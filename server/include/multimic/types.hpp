#ifndef MULTIMIC_TYPES_HPP
#define MULTIMIC_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace multimic {

constexpr uint32_t DEFAULT_CONTROL_PORT = 45454;
constexpr uint32_t DEFAULT_AUDIO_PORT   = 45455;
constexpr uint32_t DEFAULT_DISCOVERY_PORT = 45456;
constexpr uint32_t DEFAULT_WEB_PORT     = 8080;

constexpr uint32_t DEFAULT_SAMPLE_RATE  = 48000;
constexpr uint32_t DEFAULT_CHANNELS     = 1;
constexpr uint32_t DEFAULT_FRAME_MS     = 20;
constexpr uint32_t SAMPLES_PER_FRAME    = 960; // 48000 * 0.020
constexpr uint32_t DEFAULT_BITRATE      = 32000;
constexpr uint32_t DEFAULT_JITTER_MS   = 60;

// Magic bytes
constexpr uint32_t UDP_AUDIO_MAGIC     = 0x4D4D4155; // "MMAU"
constexpr uint64_t TCP_HEADER_MAGIC    = 0x4D554C54494D4943ULL; // "MULTIMIC"
constexpr uint8_t  PROTOCOL_VERSION    = 1;

enum class DeviceState {
    DISCONNECTED = 0,
    CONNECTED,
    PAIRING_REQUIRED,
    PAIRED,
    STREAMING
};

inline std::string deviceStateToString(DeviceState state) {
    switch (state) {
        case DeviceState::DISCONNECTED: return "DISCONNECTED";
        case DeviceState::CONNECTED: return "CONNECTED";
        case DeviceState::PAIRING_REQUIRED: return "PAIRING_REQUIRED";
        case DeviceState::PAIRED: return "PAIRED";
        case DeviceState::STREAMING: return "STREAMING";
        default: return "UNKNOWN";
    }
}

enum class CodecType : uint8_t {
    OPUS = 1,
    RAW_PCM = 0 // Debug mode only
};

enum class TcpMessageType : uint8_t {
    DISCOVERY_REQUEST    = 0x01,
    DISCOVERY_RESPONSE   = 0x02,
    PAIR_REQUEST         = 0x03,
    PAIR_RESPONSE        = 0x04,
    START_STREAM_REQUEST = 0x05,
    START_STREAM_RESPONSE= 0x06,
    STOP_STREAM_REQUEST  = 0x07,
    STOP_STREAM_RESPONSE = 0x08,
    HEARTBEAT_PING       = 0x09,
    HEARTBEAT_PONG       = 0x0A,
    CLI_COMMAND          = 0x0B,
    CLI_RESPONSE         = 0x0C
};

struct DeviceStats {
    uint64_t packets_received{0};
    uint64_t packets_lost{0};
    uint64_t bytes_received{0};
    double current_bitrate_kbps{0.0};
    double estimated_latency_ms{0.0};
    float rms_level{0.0f};
    float peak_level{0.0f};
};

struct DeviceInfo {
    std::string device_id;
    std::string display_name;
    std::string ip_address;
    uint16_t control_port{0};
    uint16_t audio_port{0};
    DeviceState state{DeviceState::DISCONNECTED};
    uint32_t session_id{0};
    
    // Audio control
    float volume{1.0f};
    bool is_muted{false};
    float pan{0.0f}; // -1.0 (left) to 1.0 (right)
    
    // Config
    uint32_t sample_rate{DEFAULT_SAMPLE_RATE};
    uint8_t channels{DEFAULT_CHANNELS};
    uint32_t bitrate{DEFAULT_BITRATE};
    
    // Runtime Stats
    DeviceStats stats;
    std::chrono::steady_clock::time_point last_seen;
};

struct AudioFrameHeader {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    char device_id[16];
    uint32_t session_id;
    uint64_t sequence_num;
    uint64_t timestamp_us;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t codec;
    uint16_t payload_len;
};

} // namespace multimic

#endif // MULTIMIC_TYPES_HPP
