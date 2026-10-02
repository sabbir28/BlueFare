#include "multimic/audio_mixer.hpp"
#include <cmath>
#include <algorithm>

namespace multimic {

AudioMixer::AudioMixer(size_t frame_size, uint32_t sample_rate)
    : frame_size_(frame_size), sample_rate_(sample_rate) {}

void AudioMixer::setChannelVolume(const std::string& device_id, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    channels_[device_id].volume = std::clamp(volume, 0.0f, 2.0f);
}

void AudioMixer::setChannelMute(const std::string& device_id, bool muted) {
    std::lock_guard<std::mutex> lock(mutex_);
    channels_[device_id].is_muted = muted;
}

void AudioMixer::setChannelPan(const std::string& device_id, float pan) {
    std::lock_guard<std::mutex> lock(mutex_);
    channels_[device_id].pan = std::clamp(pan, -1.0f, 1.0f);
}

void AudioMixer::removeChannel(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    channels_.erase(device_id);
}

float AudioMixer::applySoftLimiter(float sample) {
    // Soft knee limiter via hyperbolic tangent to avoid harsh digital clipping (> +1.0 or < -1.0)
    if (std::abs(sample) <= 0.8f) {
        return sample;
    }
    return std::tanh(sample);
}

void AudioMixer::mixChannels(
    const std::unordered_map<std::string, const float*>& channel_buffers,
    float* out_master_pcm,
    size_t frame_size,
    std::unordered_map<std::string, std::pair<float, float>>& out_meters) {

    std::lock_guard<std::mutex> lock(mutex_);

    // Clear output buffer
    std::fill(out_master_pcm, out_master_pcm + frame_size, 0.0f);

    size_t active_channels = 0;

    for (const auto& [device_id, pcm_ptr] : channel_buffers) {
        if (!pcm_ptr) continue;

        auto& state = channels_[device_id];
        if (state.is_muted) {
            out_meters[device_id] = {0.0f, 0.0f};
            continue;
        }

        float sum_sq = 0.0f;
        float peak = 0.0f;
        float vol = state.volume;

        for (size_t i = 0; i < frame_size; ++i) {
            float sample = pcm_ptr[i] * vol;
            float abs_sample = std::abs(sample);
            if (abs_sample > peak) peak = abs_sample;
            sum_sq += sample * sample;

            out_master_pcm[i] += sample;
        }

        float rms = std::sqrt(sum_sq / static_cast<float>(frame_size));
        out_meters[device_id] = {rms, peak};
        active_channels++;
    }

    // Apply headroom normalization & soft limiting to master mix
    float gain_norm = 1.0f;
    if (active_channels > 1) {
        gain_norm = 1.0f / std::sqrt(static_cast<float>(active_channels));
    }

    for (size_t i = 0; i < frame_size; ++i) {
        out_master_pcm[i] = applySoftLimiter(out_master_pcm[i] * gain_norm);
    }
}

} // namespace multimic
