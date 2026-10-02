#include "multimic/audio_jitter_buffer.hpp"
#include "multimic/logger.hpp"

namespace multimic {

AudioJitterBuffer::AudioJitterBuffer(uint32_t target_jitter_ms)
    : target_jitter_ms_(target_jitter_ms) {}

void AudioJitterBuffer::setJitterTargetMs(uint32_t ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    target_jitter_ms_ = ms;
}

void AudioJitterBuffer::pushPacket(uint64_t sequence_num, uint64_t timestamp_us, const std::vector<uint8_t>& payload) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        expected_seq_ = sequence_num;
        initialized_ = true;
    } else if (!has_popped_ && sequence_num < expected_seq_) {
        expected_seq_ = sequence_num;
    } else if (has_popped_ && sequence_num < expected_seq_) {
        return; // Ignore late packets after popping has started
    }

    BufferedFrame frame;
    frame.sequence_num = sequence_num;
    frame.timestamp_us = timestamp_us;
    frame.opus_payload = payload;
    frame.is_missing = false;

    buffer_[sequence_num] = std::move(frame);

    if (buffer_.size() > 50) {
        auto oldest = buffer_.begin();
        expected_seq_ = oldest->first;
    }
}

bool AudioJitterBuffer::popNextFrame(BufferedFrame& out_frame) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    size_t min_frames = (target_jitter_ms_ + DEFAULT_FRAME_MS - 1) / DEFAULT_FRAME_MS;
    if (buffer_.size() < min_frames && buffer_.find(expected_seq_) == buffer_.end()) {
        return false;
    }

    has_popped_ = true;

    auto it = buffer_.find(expected_seq_);
    if (it != buffer_.end()) {
        out_frame = std::move(it->second);
        buffer_.erase(it);
        expected_seq_++;
        return true;
    }

    out_frame.sequence_num = expected_seq_;
    out_frame.timestamp_us = 0;
    out_frame.opus_payload.clear();
    out_frame.is_missing = true;
    
    expected_seq_++;
    return true;
}

void AudioJitterBuffer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
    expected_seq_ = 0;
    initialized_ = false;
    has_popped_ = false;
}


size_t AudioJitterBuffer::getQueueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size();
}

} // namespace multimic
