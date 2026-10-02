#ifndef MULTIMIC_CONFIG_HPP
#define MULTIMIC_CONFIG_HPP

#include "multimic/types.hpp"
#include <string>

namespace multimic {

struct ServerConfig {
    // Server ports
    uint16_t control_port{DEFAULT_CONTROL_PORT};
    uint16_t audio_port{DEFAULT_AUDIO_PORT};
    uint16_t discovery_port{DEFAULT_DISCOVERY_PORT};
    uint16_t web_port{DEFAULT_WEB_PORT};

    // Audio parameters
    uint32_t sample_rate{DEFAULT_SAMPLE_RATE};
    uint8_t channels{DEFAULT_CHANNELS};
    uint32_t frame_ms{DEFAULT_FRAME_MS};
    uint32_t jitter_buffer_ms{DEFAULT_JITTER_MS};
    std::string output_driver{"pipewire"}; // "pipewire", "alsa", "dummy"
    std::string record_path{""};

    // Opus configuration
    uint32_t opus_bitrate{DEFAULT_BITRATE};

    // Security & Pairing
    bool pairing_required{true};
    std::string fixed_pairing_code{""}; // Empty means generate random 6-digit pin

    // Logging & CLI overrides
    bool verbose{false};

    // Methods
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;
    std::string toJsonString() const;
};

} // namespace multimic

#endif // MULTIMIC_CONFIG_HPP
