#ifndef MULTIMIC_AUDIO_RECEIVER_HPP
#define MULTIMIC_AUDIO_RECEIVER_HPP

#include "multimic/types.hpp"
#include "multimic/network.hpp"
#include "multimic/device_manager.hpp"
#include "multimic/audio_jitter_buffer.hpp"
#include "multimic/audio_decoder.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace multimic {

class AudioReceiver {
public:
    AudioReceiver(DeviceManager& device_manager, uint32_t jitter_ms = DEFAULT_JITTER_MS);
    ~AudioReceiver();

    bool start(uint16_t audio_port);
    void stop();

    // Fetches decoded PCM samples per device for the current frame
    void getDecodedFrameBuffers(
        size_t frame_size,
        std::unordered_map<std::string, std::vector<float>>& out_buffers);

private:
    void receiveLoop();

    DeviceManager& device_manager_;
    uint32_t jitter_ms_{DEFAULT_JITTER_MS};
    UdpSocket socket_;
    std::atomic<bool> running_{false};
    std::thread receive_thread_;

    mutable std::mutex buffer_mutex_;
    std::unordered_map<std::string, std::unique_ptr<AudioJitterBuffer>> jitter_buffers_;
    std::unordered_map<std::string, std::unique_ptr<AudioDecoder>> decoders_;
};

} // namespace multimic

#endif // MULTIMIC_AUDIO_RECEIVER_HPP
