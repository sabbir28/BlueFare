#ifndef MULTIMIC_AUDIO_OUTPUT_HPP
#define MULTIMIC_AUDIO_OUTPUT_HPP

#include "multimic/types.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

struct _snd_pcm;

namespace multimic {

class WavWriter {
public:
    WavWriter();
    ~WavWriter();

    bool open(const std::string& path, uint32_t sample_rate = DEFAULT_SAMPLE_RATE, uint8_t channels = DEFAULT_CHANNELS);
    void writeFloatSamples(const float* samples, size_t count);
    void close();

private:
    void writeHeader();
    void updateHeaderSize();

    std::ofstream file_;
    uint32_t sample_rate_{DEFAULT_SAMPLE_RATE};
    uint8_t channels_{DEFAULT_CHANNELS};
    uint32_t total_samples_{0};
    std::string path_;
};

class IAudioOutput {
public:
    virtual ~IAudioOutput() = default;
    virtual bool open(uint32_t sample_rate, uint8_t channels) = 0;
    virtual void writeSamples(const float* pcm, size_t frame_size) = 0;
    virtual void close() = 0;
    virtual std::string getDriverName() const = 0;
};

class AlsaOutput : public IAudioOutput {
public:
    AlsaOutput(const std::string& device_name = "default");
    ~AlsaOutput() override;

    bool open(uint32_t sample_rate, uint8_t channels) override;
    void writeSamples(const float* pcm, size_t frame_size) override;
    void close() override;
    std::string getDriverName() const override { return "ALSA (" + device_name_ + ")"; }

private:
    std::string device_name_;
    _snd_pcm* pcm_handle_{nullptr};
    std::vector<int16_t> convert_buffer_;
};

class PipeWireOutput : public IAudioOutput {
public:
    PipeWireOutput();
    ~PipeWireOutput() override;

    bool open(uint32_t sample_rate, uint8_t channels) override;
    void writeSamples(const float* pcm, size_t frame_size) override;
    void close() override;
    std::string getDriverName() const override { return "PipeWire (ALSA-emulation plugin)"; }

private:
    std::unique_ptr<AlsaOutput> alsa_fallback_;
};

class DummyOutput : public IAudioOutput {
public:
    DummyOutput() = default;
    ~DummyOutput() override = default;

    bool open(uint32_t sample_rate, uint8_t channels) override { return true; }
    void writeSamples(const float* pcm, size_t frame_size) override {}
    void close() override {}
    std::string getDriverName() const override { return "Dummy Output"; }
};

std::unique_ptr<IAudioOutput> createAudioOutputDriver(const std::string& driver_type);

} // namespace multimic

#endif // MULTIMIC_AUDIO_OUTPUT_HPP
