#include "multimic/device_manager.hpp"
#include "multimic/logger.hpp"
#include <random>

namespace multimic {

void DeviceManager::setPairingRequired(bool required) {
    std::lock_guard<std::mutex> lock(mutex_);
    pairing_required_ = required;
}

void DeviceManager::setFixedPairingCode(const std::string& code) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!code.empty()) {
        pairing_code_ = code;
    }
}

std::string DeviceManager::getPairingCode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pairing_code_;
}

uint32_t DeviceManager::generateSessionId() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(100000, 999999);
    return dist(rng);
}

DeviceInfo DeviceManager::addOrUpdateDevice(const std::string& device_id, const std::string& name, const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it != devices_.end()) {
        it->second.display_name = name;
        it->second.ip_address = ip;
        it->second.last_seen = std::chrono::steady_clock::now();
        return it->second;
    }

    DeviceInfo info;
    info.device_id = device_id;
    info.display_name = name;
    info.ip_address = ip;
    info.state = pairing_required_ ? DeviceState::PAIRING_REQUIRED : DeviceState::PAIRED;
    info.session_id = pairing_required_ ? 0 : generateSessionId();
    info.last_seen = std::chrono::steady_clock::now();

    devices_[device_id] = info;
    LOG_INFO("Device registered: " + name + " (" + device_id + ") at " + ip);
    return info;
}

bool DeviceManager::setDeviceState(const std::string& device_id, DeviceState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return false;
    it->second.state = state;
    it->second.last_seen = std::chrono::steady_clock::now();
    LOG_INFO("Device state changed: " + device_id + " -> " + deviceStateToString(state));
    return true;
}

bool DeviceManager::setDeviceVolume(const std::string& device_id, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return false;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 2.0f) volume = 2.0f; // Max 200% gain
    it->second.volume = volume;
    LOG_INFO("Device volume updated: " + device_id + " -> " + std::to_string(volume));
    return true;
}

bool DeviceManager::setDeviceMute(const std::string& device_id, bool muted) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return false;
    it->second.is_muted = muted;
    LOG_INFO("Device mute state: " + device_id + " -> " + (muted ? "MUTED" : "UNMUTED"));
    return true;
}

bool DeviceManager::verifyPairing(const std::string& device_id, const std::string& pin, uint32_t& out_session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return false;

    if (pairing_required_ && pin != pairing_code_) {
        LOG_WARN("Pairing failed for device " + device_id + " (Invalid PIN)");
        return false;
    }

    it->second.session_id = generateSessionId();
    it->second.state = DeviceState::PAIRED;
    it->second.last_seen = std::chrono::steady_clock::now();
    out_session_id = it->second.session_id;

    LOG_INFO("Device paired successfully: " + device_id + " (Session ID: " + std::to_string(out_session_id) + ")");
    return true;
}

bool DeviceManager::validateSession(const std::string& device_id, uint32_t session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return false;
    if (pairing_required_ && (it->second.session_id == 0 || it->second.session_id != session_id)) {
        return false;
    }
    it->second.last_seen = std::chrono::steady_clock::now();
    return true;
}

void DeviceManager::recordPacketReceived(const std::string& device_id, uint64_t seq_num, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return;

    it->second.stats.packets_received++;
    it->second.stats.bytes_received += bytes;
    it->second.last_seen = std::chrono::steady_clock::now();

    auto seq_it = last_sequence_nums_.find(device_id);
    if (seq_it != last_sequence_nums_.end()) {
        uint64_t last_seq = seq_it->second;
        if (seq_num > last_seq + 1) {
            it->second.stats.packets_lost += (seq_num - last_seq - 1);
        }
        seq_it->second = std::max(last_seq, seq_num);
    } else {
        last_sequence_nums_[device_id] = seq_num;
    }
}

void DeviceManager::updateMeters(const std::string& device_id, float rms, float peak) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return;
    
    // Smoothing (exponential moving average)
    constexpr float alpha = 0.3f;
    it->second.stats.rms_level = alpha * rms + (1.0f - alpha) * it->second.stats.rms_level;
    it->second.stats.peak_level = alpha * peak + (1.0f - alpha) * it->second.stats.peak_level;
}

std::vector<DeviceInfo> DeviceManager::getAllDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DeviceInfo> result;
    for (const auto& [id, info] : devices_) {
        result.push_back(info);
    }
    return result;
}

bool DeviceManager::getDevice(const std::string& device_id, DeviceInfo& out_info) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = devices_.find(device_id);
    if (it == devices_.end()) return false;
    out_info = it->second;
    return true;
}

void DeviceManager::removeInactiveDevices(std::chrono::seconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    for (auto it = devices_.begin(); it != devices_.end(); ) {
        if (now - it->second.last_seen > timeout) {
            LOG_INFO("Device disconnected due to timeout: " + it->first);
            last_sequence_nums_.erase(it->first);
            it = devices_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace multimic
