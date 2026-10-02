#ifndef MULTIMIC_PROTOCOL_HPP
#define MULTIMIC_PROTOCOL_HPP

#include "multimic/types.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace multimic {

struct TcpMessageHeader {
    uint64_t magic{TCP_HEADER_MAGIC};
    uint8_t version{PROTOCOL_VERSION};
    TcpMessageType type;
    uint32_t payload_len{0};
};

class Protocol {
public:
    // Byte order conversions (Big Endian <-> Host)
    static uint16_t hostToNetwork16(uint16_t val);
    static uint16_t networkToHost16(uint16_t val);
    static uint32_t hostToNetwork32(uint32_t val);
    static uint32_t networkToHost32(uint32_t val);
    static uint64_t hostToNetwork64(uint64_t val);
    static uint64_t networkToHost64(uint64_t val);

    // UDP Audio Packet Serialization
    static constexpr size_t AUDIO_HEADER_SIZE = 50;
    static std::vector<uint8_t> serializeAudioPacket(
        const AudioFrameHeader& header,
        const uint8_t* payload,
        size_t payload_len);

    static bool deserializeAudioPacket(
        const uint8_t* buffer,
        size_t len,
        AudioFrameHeader& out_header,
        std::vector<uint8_t>& out_payload);

    // TCP Control Packet Serialization
    static constexpr size_t TCP_HEADER_SIZE = 14;
    static std::vector<uint8_t> serializeTcpMessage(
        TcpMessageType type,
        const std::string& json_payload);

    static bool deserializeTcpHeader(
        const uint8_t* buffer,
        size_t len,
        TcpMessageHeader& out_header);
};

} // namespace multimic

#endif // MULTIMIC_PROTOCOL_HPP
