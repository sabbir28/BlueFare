#ifndef MULTIMIC_DEVICE_MANAGER_HPP
#define MULTIMIC_DEVICE_MANAGER_HPP

#include "multimic/types.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace multimic {

class DeviceManager {
public:
    DeviceManager() = default;
    ~DeviceManager() = default;

    void setPairingRequired(bool required);
    void setFixedPairingCode(const std::string& code);
    std::string getPairingCode() const;

    DeviceInfo addOrUpdateDevice(const std::string& device_id, const std::string& name, const std::string& ip);
    bool setDeviceState(const std::string& device_id, DeviceState state);
    bool setDeviceVolume(const std::string& device_id, float volume);
    bool setDeviceMute(const std::string& device_id, bool muted);
    
    bool verifyPairing(const std::string& device_id, const std::string& pin, uint32_t& out_session_id);
    bool validateSession(const std::string& device_id, uint32_t session_id);
    
    void recordPacketReceived(const std::string& device_id, uint64_t seq_num, size_t bytes);
    void updateMeters(const std::string& device_id, float rms, float peak);

    std::vector<DeviceInfo> getAllDevices() const;
    bool getDevice(const std::string& device_id, DeviceInfo& out_info) const;
    
    void removeInactiveDevices(std::chrono::seconds timeout);

private:
    uint32_t generateSessionId();

    bool pairing_required_{true};
    std::string pairing_code_{"483921"};
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DeviceInfo> devices_;
    std::unordered_map<std::string, uint64_t> last_sequence_nums_;
};

} // namespace multimic

#endif // MULTIMIC_DEVICE_MANAGER_HPP
