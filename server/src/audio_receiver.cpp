#include "multimic/audio_receiver.hpp"
#include "multimic/protocol.hpp"
#include "multimic/logger.hpp"
#include <vector>

namespace multimic {

AudioReceiver::AudioReceiver(DeviceManager& device_manager, uint32_t jitter_ms)
    : device_manager_(device_manager), jitter_ms_(jitter_ms) {}

AudioReceiver::~AudioReceiver() {
    stop();
}

bool AudioReceiver::start(uint16_t audio_port) {
    stop();

    if (!socket_.bindPort(audio_port)) {
        LOG_ERROR("Failed to bind UDP audio receiver socket on port " + std::to_string(audio_port));
        return false;
    }

    running_ = true;
    receive_thread_ = std::thread(&AudioReceiver::receiveLoop, this);
    LOG_INFO("UDP Audio Receiver active on port " + std::to_string(audio_port));
    return true;
}

void AudioReceiver::stop() {
    running_ = false;
    socket_.closeSocket();
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
}

void AudioReceiver::receiveLoop() {
    uint8_t buffer[4096];
    std::string client_ip;
    uint16_t client_port;

    while (running_) {
        ssize_t bytes = socket_.receiveFrom(buffer, sizeof(buffer), client_ip, client_port);
        if (bytes <= 0 || !running_) continue;

        AudioFrameHeader header;
        std::vector<uint8_t> payload;

        if (!Protocol::deserializeAudioPacket(buffer, bytes, header, payload)) {
            LOG_WARN("Malformed UDP audio packet received from " + client_ip);
            continue;
        }

        std::string device_id(header.device_id);

        if (!device_manager_.validateSession(device_id, header.session_id)) {
            LOG_WARN("Rejected audio packet from unauthenticated device/session: " + device_id);
            continue;
        }

        device_manager_.recordPacketReceived(device_id, header.sequence_num, bytes);

        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (jitter_buffers_.find(device_id) == jitter_buffers_.end()) {
            jitter_buffers_[device_id] = std::make_unique<AudioJitterBuffer>(jitter_ms_);
            decoders_[device_id] = std::make_unique<AudioDecoder>(header.sample_rate, header.channels);
            decoders_[device_id]->init();
            device_manager_.setDeviceState(device_id, DeviceState::STREAMING);
        }

        jitter_buffers_[device_id]->pushPacket(header.sequence_num, header.timestamp_us, payload);
    }
}

void AudioReceiver::getDecodedFrameBuffers(
    size_t frame_size,
    std::unordered_map<std::string, std::vector<float>>& out_buffers) {

    std::lock_guard<std::mutex> lock(buffer_mutex_);
    out_buffers.clear();

    for (auto& [device_id, jb] : jitter_buffers_) {
        BufferedFrame frame;
        if (jb->popNextFrame(frame)) {
            std::vector<float> pcm(frame_size, 0.0f);
            if (frame.is_missing) {
                decoders_[device_id]->decodeMissingFrame(pcm.data(), frame_size);
            } else {
                decoders_[device_id]->decodeFrame(frame.opus_payload.data(), frame.opus_payload.size(), pcm.data(), frame_size);
            }
            out_buffers[device_id] = std::move(pcm);
        }
    }
}

} // namespace multimic
