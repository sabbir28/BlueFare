#ifndef MULTIMIC_AUDIO_JITTER_BUFFER_HPP
#define MULTIMIC_AUDIO_JITTER_BUFFER_HPP

#include "multimic/types.hpp"
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>

namespace multimic {

struct BufferedFrame {
    uint64_t sequence_num{0};
    uint64_t timestamp_us{0};
    std::vector<uint8_t> opus_payload;
    bool is_missing{false}; // True if frame was lost and needs PLC
};

class AudioJitterBuffer {
public:
    explicit AudioJitterBuffer(uint32_t target_jitter_ms = DEFAULT_JITTER_MS);
    ~AudioJitterBuffer() = default;

    void setJitterTargetMs(uint32_t ms);
    void pushPacket(uint64_t sequence_num, uint64_t timestamp_us, const std::vector<uint8_t>& payload);
    bool popNextFrame(BufferedFrame& out_frame);
    
    void reset();
    size_t getQueueSize() const;

private:
    uint32_t target_jitter_ms_{DEFAULT_JITTER_MS};
    mutable std::mutex mutex_;
    std::map<uint64_t, BufferedFrame> buffer_;
    uint64_t expected_seq_{0};
    bool initialized_{false};
    bool has_popped_{false};
};


} // namespace multimic

#endif // MULTIMIC_AUDIO_JITTER_BUFFER_HPP
