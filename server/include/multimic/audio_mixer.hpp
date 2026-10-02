#ifndef MULTIMIC_AUDIO_MIXER_HPP
#define MULTIMIC_AUDIO_MIXER_HPP

#include "multimic/types.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>

namespace multimic {

struct MixerChannelState {
    float volume{1.0f};
    bool is_muted{false};
    float pan{0.0f};
    float current_rms{0.0f};
    float current_peak{0.0f};
};

class AudioMixer {
public:
    AudioMixer(size_t frame_size = SAMPLES_PER_FRAME, uint32_t sample_rate = DEFAULT_SAMPLE_RATE);
    ~AudioMixer() = default;

    void setChannelVolume(const std::string& device_id, float volume);
    void setChannelMute(const std::string& device_id, bool muted);
    void setChannelPan(const std::string& device_id, float pan);
    void removeChannel(const std::string& device_id);

    // Mixes multiple mono channel sample buffers into single master float output buffer
    void mixChannels(
        const std::unordered_map<std::string, const float*>& channel_buffers,
        float* out_master_pcm,
        size_t frame_size,
        std::unordered_map<std::string, std::pair<float, float>>& out_meters);

private:
    float applySoftLimiter(float sample);

    size_t frame_size_{SAMPLES_PER_FRAME};
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    mutable std::mutex mutex_;
    std::unordered_map<std::string, MixerChannelState> channels_;
};

} // namespace multimic

#endif // MULTIMIC_AUDIO_MIXER_HPP
