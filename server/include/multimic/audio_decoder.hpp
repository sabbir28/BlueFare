#ifndef MULTIMIC_AUDIO_DECODER_HPP
#define MULTIMIC_AUDIO_DECODER_HPP

#include "multimic/types.hpp"
#include <vector>
#include <cstdint>

struct OpusDecoder; // Forward declaration

namespace multimic {

class AudioDecoder {
public:
    AudioDecoder(uint32_t sample_rate = DEFAULT_SAMPLE_RATE, uint8_t channels = DEFAULT_CHANNELS);
    ~AudioDecoder();

    bool init();
    bool decodeFrame(const uint8_t* payload, size_t len, float* out_pcm, size_t frame_size);
    bool decodeMissingFrame(float* out_pcm, size_t frame_size);

private:
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    uint8_t channels_{DEFAULT_CHANNELS};
    OpusDecoder* decoder_{nullptr};
};

} // namespace multimic

#endif // MULTIMIC_AUDIO_DECODER_HPP
