#include "multimic/audio_mixer.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace multimic;

void test_mixer_channel_summing_and_limiting() {
    AudioMixer mixer(960, 48000);

    std::vector<float> ch1(960, 0.5f);
    std::vector<float> ch2(960, 0.4f);

    std::unordered_map<std::string, const float*> inputs = {
        {"phone01", ch1.data()},
        {"phone02", ch2.data()}
    };

    std::vector<float> master(960, 0.0f);
    std::unordered_map<std::string, std::pair<float, float>> meters;

    mixer.mixChannels(inputs, master.data(), 960, meters);

    assert(meters.find("phone01") != meters.end());
    assert(meters.find("phone02") != meters.end());

    // Verify output samples are within [-1.0, 1.0] soft-limited bounds
    for (float sample : master) {
        assert(sample >= -1.0f && sample <= 1.0f);
    }

    std::cout << "[PASS] test_mixer_channel_summing_and_limiting\n";
}

void test_mixer_mute_volume() {
    AudioMixer mixer(960, 48000);

    std::vector<float> ch1(960, 0.5f);
    mixer.setChannelMute("phone01", true);

    std::unordered_map<std::string, const float*> inputs = {
        {"phone01", ch1.data()}
    };

    std::vector<float> master(960, 0.0f);
    std::unordered_map<std::string, std::pair<float, float>> meters;

    mixer.mixChannels(inputs, master.data(), 960, meters);

    for (float sample : master) {
        assert(sample == 0.0f); // Muted channel produces silence
    }

    std::cout << "[PASS] test_mixer_mute_volume\n";
}

int main() {
    std::cout << "Running Audio Mixer Unit Tests...\n";
    test_mixer_channel_summing_and_limiting();
    test_mixer_mute_volume();
    std::cout << "All Audio Mixer Tests Passed!\n";
    return 0;
}
