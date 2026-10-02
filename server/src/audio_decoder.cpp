#include "multimic/audio_decoder.hpp"
#include "multimic/logger.hpp"
#include "third_party/opus/opus.h"
#include <cstring>

namespace multimic {

AudioDecoder::AudioDecoder(uint32_t sample_rate, uint8_t channels)
    : sample_rate_(sample_rate), channels_(channels) {}

AudioDecoder::~AudioDecoder() {
    if (decoder_) {
        opus_decoder_destroy(decoder_);
        decoder_ = nullptr;
    }
}

bool AudioDecoder::init() {
    int error = 0;
    decoder_ = opus_decoder_create(sample_rate_, channels_, &error);
    if (error != OPUS_OK || !decoder_) {
        LOG_ERROR("Failed to create Opus decoder: " + std::string(opus_strerror(error)));
        return false;
    }
    LOG_INFO("Opus Decoder initialized (" + std::to_string(sample_rate_) + " Hz, " + std::to_string(channels_) + " ch)");
    return true;
}

bool AudioDecoder::decodeFrame(const uint8_t* payload, size_t len, float* out_pcm, size_t frame_size) {
    if (!decoder_) return false;

    int samples = opus_decode_float(decoder_, payload, static_cast<opus_int32>(len), out_pcm, static_cast<int>(frame_size), 0);
    if (samples < 0) {
        LOG_WARN("Opus decode error: " + std::string(opus_strerror(samples)));
        std::memset(out_pcm, 0, frame_size * channels_ * sizeof(float));
        return false;
    }
    return true;
}

bool AudioDecoder::decodeMissingFrame(float* out_pcm, size_t frame_size) {
    if (!decoder_) return false;

    // Opus Packet Loss Concealment (PLC): pass NULL payload and decode_fec = 1
    int samples = opus_decode_float(decoder_, nullptr, 0, out_pcm, static_cast<int>(frame_size), 1);
    if (samples < 0) {
        std::memset(out_pcm, 0, frame_size * channels_ * sizeof(float));
        return false;
    }
    return true;
}

} // namespace multimic
