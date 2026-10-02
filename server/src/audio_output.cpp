#include "multimic/audio_output.hpp"
#include "multimic/logger.hpp"
#include "third_party/alsa/asoundlib.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <dlfcn.h>

namespace multimic {

// WavWriter Implementation
WavWriter::WavWriter() {}

WavWriter::~WavWriter() {
    close();
}

bool WavWriter::open(const std::string& path, uint32_t sample_rate, uint8_t channels) {
    close();
    path_ = path;
    sample_rate_ = sample_rate;
    channels_ = channels;
    total_samples_ = 0;

    file_.open(path, std::ios::binary);
    if (!file_.is_open()) {
        LOG_ERROR("Failed to open WAV output file: " + path);
        return false;
    }

    writeHeader();
    LOG_INFO("WAV Recorder active: " + path + " (" + std::to_string(sample_rate) + " Hz, " + std::to_string(channels) + " ch)");
    return true;
}

void WavWriter::writeHeader() {
    uint32_t byte_rate = sample_rate_ * channels_ * sizeof(int16_t);
    uint16_t block_align = channels_ * sizeof(int16_t);
    uint16_t bits_per_sample = 16;
    uint32_t data_size = 0;
    uint32_t chunk_size = 36 + data_size;

    file_.write("RIFF", 4);
    file_.write(reinterpret_cast<const char*>(&chunk_size), 4);
    file_.write("WAVE", 4);

    file_.write("fmt ", 4);
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1; // PCM
    uint16_t num_channels = channels_;

    file_.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
    file_.write(reinterpret_cast<const char*>(&audio_format), 2);
    file_.write(reinterpret_cast<const char*>(&num_channels), 2);
    file_.write(reinterpret_cast<const char*>(&sample_rate_), 4);
    file_.write(reinterpret_cast<const char*>(&byte_rate), 4);
    file_.write(reinterpret_cast<const char*>(&block_align), 2);
    file_.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

    file_.write("data", 4);
    file_.write(reinterpret_cast<const char*>(&data_size), 4);
}

void WavWriter::writeFloatSamples(const float* samples, size_t count) {
    if (!file_.is_open()) return;

    std::vector<int16_t> pcm16(count);
    for (size_t i = 0; i < count; ++i) {
        float sample = std::clamp(samples[i], -1.0f, 1.0f);
        pcm16[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    file_.write(reinterpret_cast<const char*>(pcm16.data()), count * sizeof(int16_t));
    total_samples_ += static_cast<uint32_t>(count);
}

void WavWriter::updateHeaderSize() {
    if (!file_.is_open()) return;

    uint32_t data_size = total_samples_ * sizeof(int16_t);
    uint32_t chunk_size = 36 + data_size;

    file_.seekp(4, std::ios::beg);
    file_.write(reinterpret_cast<const char*>(&chunk_size), 4);

    file_.seekp(40, std::ios::beg);
    file_.write(reinterpret_cast<const char*>(&data_size), 4);
}

void WavWriter::close() {
    if (file_.is_open()) {
        updateHeaderSize();
        file_.close();
        LOG_INFO("WAV file closed: " + path_);
    }
}

// ALSA Output Dynamic Wrapper
typedef int (*snd_pcm_open_t)(snd_pcm_t **, const char *, snd_pcm_stream_t, int);
typedef int (*snd_pcm_close_t)(snd_pcm_t *);
typedef int (*snd_pcm_hw_params_malloc_t)(snd_pcm_hw_params_t **);
typedef void (*snd_pcm_hw_params_free_t)(snd_pcm_hw_params_t *);
typedef int (*snd_pcm_hw_params_any_t)(snd_pcm_t *, snd_pcm_hw_params_t *);
typedef int (*snd_pcm_hw_params_set_access_t)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t);
typedef int (*snd_pcm_hw_params_set_format_t)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
typedef int (*snd_pcm_hw_params_set_rate_near_t)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
typedef int (*snd_pcm_hw_params_set_channels_t)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int);
typedef int (*snd_pcm_hw_params_t_func)(snd_pcm_t *, snd_pcm_hw_params_t *);
typedef int (*snd_pcm_prepare_t)(snd_pcm_t *);
typedef snd_pcm_sframes_t (*snd_pcm_writei_t)(snd_pcm_t *, const void *, snd_pcm_uframes_t);
typedef int (*snd_pcm_recover_t)(snd_pcm_t *, int, int);

struct AlsaApi {
    void* handle{nullptr};
    snd_pcm_open_t pcm_open{nullptr};
    snd_pcm_close_t pcm_close{nullptr};
    snd_pcm_hw_params_malloc_t hw_params_malloc{nullptr};
    snd_pcm_hw_params_free_t hw_params_free{nullptr};
    snd_pcm_hw_params_any_t hw_params_any{nullptr};
    snd_pcm_hw_params_set_access_t hw_params_set_access{nullptr};
    snd_pcm_hw_params_set_format_t hw_params_set_format{nullptr};
    snd_pcm_hw_params_set_rate_near_t hw_params_set_rate_near{nullptr};
    snd_pcm_hw_params_set_channels_t hw_params_set_channels{nullptr};
    snd_pcm_hw_params_t_func hw_params{nullptr};
    snd_pcm_prepare_t prepare{nullptr};
    snd_pcm_writei_t writei{nullptr};
    snd_pcm_recover_t recover{nullptr};

    static AlsaApi& get() {
        static AlsaApi api;
        return api;
    }

    AlsaApi() {
        handle = dlopen("libasound.so.2", RTLD_LAZY);
        if (handle) {
            pcm_open = (snd_pcm_open_t)dlsym(handle, "snd_pcm_open");
            pcm_close = (snd_pcm_close_t)dlsym(handle, "snd_pcm_close");
            hw_params_malloc = (snd_pcm_hw_params_malloc_t)dlsym(handle, "snd_pcm_hw_params_malloc");
            hw_params_free = (snd_pcm_hw_params_free_t)dlsym(handle, "snd_pcm_hw_params_free");
            hw_params_any = (snd_pcm_hw_params_any_t)dlsym(handle, "snd_pcm_hw_params_any");
            hw_params_set_access = (snd_pcm_hw_params_set_access_t)dlsym(handle, "snd_pcm_hw_params_set_access");
            hw_params_set_format = (snd_pcm_hw_params_set_format_t)dlsym(handle, "snd_pcm_hw_params_set_format");
            hw_params_set_rate_near = (snd_pcm_hw_params_set_rate_near_t)dlsym(handle, "snd_pcm_hw_params_set_rate_near");
            hw_params_set_channels = (snd_pcm_hw_params_set_channels_t)dlsym(handle, "snd_pcm_hw_params_set_channels");
            hw_params = (snd_pcm_hw_params_t_func)dlsym(handle, "snd_pcm_hw_params");
            prepare = (snd_pcm_prepare_t)dlsym(handle, "snd_pcm_prepare");
            writei = (snd_pcm_writei_t)dlsym(handle, "snd_pcm_writei");
            recover = (snd_pcm_recover_t)dlsym(handle, "snd_pcm_recover");
        }
    }

    bool isValid() const { return handle && pcm_open && writei; }
};

AlsaOutput::AlsaOutput(const std::string& device_name)
    : device_name_(device_name) {}

AlsaOutput::~AlsaOutput() {
    close();
}

bool AlsaOutput::open(uint32_t sample_rate, uint8_t channels) {
    close();

    auto& alsa = AlsaApi::get();
    if (!alsa.isValid()) {
        LOG_WARN("ALSA dynamic library (libasound.so.2) unavailable. Fallback to dummy output.");
        return false;
    }

    int err = alsa.pcm_open(&pcm_handle_, device_name_.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        LOG_WARN("Could not open ALSA device: " + device_name_);
        return false;
    }

    snd_pcm_hw_params_t *params = nullptr;
    alsa.hw_params_malloc(&params);
    alsa.hw_params_any(pcm_handle_, params);
    alsa.hw_params_set_access(pcm_handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    alsa.hw_params_set_format(pcm_handle_, params, SND_PCM_FORMAT_S16_LE);
    unsigned int rate = sample_rate;
    alsa.hw_params_set_rate_near(pcm_handle_, params, &rate, nullptr);
    alsa.hw_params_set_channels(pcm_handle_, params, channels);
    alsa.hw_params(pcm_handle_, params);
    alsa.hw_params_free(params);
    alsa.prepare(pcm_handle_);

    LOG_INFO("ALSA Output initialized (" + device_name_ + " @ " + std::to_string(rate) + " Hz)");
    return true;
}

void AlsaOutput::writeSamples(const float* pcm, size_t frame_size) {
    if (!pcm_handle_) return;

    auto& alsa = AlsaApi::get();
    convert_buffer_.resize(frame_size);
    for (size_t i = 0; i < frame_size; ++i) {
        float sample = std::clamp(pcm[i], -1.0f, 1.0f);
        convert_buffer_[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    snd_pcm_sframes_t frames = alsa.writei(pcm_handle_, convert_buffer_.data(), frame_size);
    if (frames < 0) {
        alsa.recover(pcm_handle_, static_cast<int>(frames), 1);
    }
}

void AlsaOutput::close() {
    if (pcm_handle_) {
        auto& alsa = AlsaApi::get();
        if (alsa.pcm_close) alsa.pcm_close(pcm_handle_);
        pcm_handle_ = nullptr;
    }
}

// PipeWire Output Implementation
PipeWireOutput::PipeWireOutput() {
    alsa_fallback_ = std::make_unique<AlsaOutput>("pipewire");
}

PipeWireOutput::~PipeWireOutput() {
    close();
}

bool PipeWireOutput::open(uint32_t sample_rate, uint8_t channels) {
    if (alsa_fallback_ && alsa_fallback_->open(sample_rate, channels)) {
        LOG_INFO("PipeWire Audio Output connected via PipeWire-ALSA plugin.");
        return true;
    }
    LOG_WARN("PipeWire plugin default connection unavailable. System falling back.");
    return false;
}

void PipeWireOutput::writeSamples(const float* pcm, size_t frame_size) {
    if (alsa_fallback_) alsa_fallback_->writeSamples(pcm, frame_size);
}

void PipeWireOutput::close() {
    if (alsa_fallback_) alsa_fallback_->close();
}

std::unique_ptr<IAudioOutput> createAudioOutputDriver(const std::string& driver_type) {
    if (driver_type == "pipewire") {
        auto pw = std::make_unique<PipeWireOutput>();
        if (pw->open(DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS)) return pw;
    }
    
    if (driver_type == "alsa" || driver_type == "pipewire") {
        auto alsa = std::make_unique<AlsaOutput>("default");
        if (alsa->open(DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS)) return alsa;
    }

    LOG_INFO("Using Dummy (Null) Audio Output driver.");
    return std::make_unique<DummyOutput>();
}

} // namespace multimic
