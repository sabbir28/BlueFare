#include "multimic/protocol.hpp"
#include <cstring>
#include <arpa/inet.h>

namespace multimic {

uint16_t Protocol::hostToNetwork16(uint16_t val) { return htons(val); }
uint16_t Protocol::networkToHost16(uint16_t val) { return ntohs(val); }
uint32_t Protocol::hostToNetwork32(uint32_t val) { return htonl(val); }
uint32_t Protocol::networkToHost32(uint32_t val) { return ntohl(val); }

uint64_t Protocol::hostToNetwork64(uint64_t val) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t)htonl(val & 0xFFFFFFFF)) << 32) | htonl(val >> 32);
#else
    return val;
#endif
}

uint64_t Protocol::networkToHost64(uint64_t val) {
    return hostToNetwork64(val);
}

std::vector<uint8_t> Protocol::serializeAudioPacket(
    const AudioFrameHeader& header,
    const uint8_t* payload,
    size_t payload_len) {

    std::vector<uint8_t> packet(AUDIO_HEADER_SIZE + payload_len);
    uint8_t* ptr = packet.data();

    uint32_t magic_net = hostToNetwork32(header.magic);
    std::memcpy(ptr, &magic_net, 4); ptr += 4;
    
    *ptr = header.version; ptr += 1;
    *ptr = header.type; ptr += 1;
    
    std::memset(ptr, 0, 16);
    std::strncpy(reinterpret_cast<char*>(ptr), header.device_id, 15);
    ptr += 16;
    
    uint32_t session_net = hostToNetwork32(header.session_id);
    std::memcpy(ptr, &session_net, 4); ptr += 4;
    
    uint64_t seq_net = hostToNetwork64(header.sequence_num);
    std::memcpy(ptr, &seq_net, 8); ptr += 8;
    
    uint64_t ts_net = hostToNetwork64(header.timestamp_us);
    std::memcpy(ptr, &ts_net, 8); ptr += 8;
    
    uint32_t rate_net = hostToNetwork32(header.sample_rate);
    std::memcpy(ptr, &rate_net, 4); ptr += 4;
    
    *ptr = header.channels; ptr += 1;
    *ptr = header.codec; ptr += 1;
    
    uint16_t len_net = hostToNetwork16(static_cast<uint16_t>(payload_len));
    std::memcpy(ptr, &len_net, 2); ptr += 2;
    
    if (payload && payload_len > 0) {
        std::memcpy(ptr, payload, payload_len);
    }

    return packet;
}

bool Protocol::deserializeAudioPacket(
    const uint8_t* buffer,
    size_t len,
    AudioFrameHeader& out_header,
    std::vector<uint8_t>& out_payload) {

    if (!buffer || len < AUDIO_HEADER_SIZE) {
        return false;
    }

    const uint8_t* ptr = buffer;

    uint32_t magic_net;
    std::memcpy(&magic_net, ptr, 4); ptr += 4;
    out_header.magic = networkToHost32(magic_net);
    if (out_header.magic != UDP_AUDIO_MAGIC) {
        return false;
    }

    out_header.version = *ptr; ptr += 1;
    out_header.type = *ptr; ptr += 1;

    std::memset(out_header.device_id, 0, 16);
    std::memcpy(out_header.device_id, ptr, 16);
    out_header.device_id[15] = '\0';
    ptr += 16;

    uint32_t session_net;
    std::memcpy(&session_net, ptr, 4); ptr += 4;
    out_header.session_id = networkToHost32(session_net);

    uint64_t seq_net;
    std::memcpy(&seq_net, ptr, 8); ptr += 8;
    out_header.sequence_num = networkToHost64(seq_net);

    uint64_t ts_net;
    std::memcpy(&ts_net, ptr, 8); ptr += 8;
    out_header.timestamp_us = networkToHost64(ts_net);

    uint32_t rate_net;
    std::memcpy(&rate_net, ptr, 4); ptr += 4;
    out_header.sample_rate = networkToHost32(rate_net);

    out_header.channels = *ptr; ptr += 1;
    out_header.codec = *ptr; ptr += 1;

    uint16_t len_net;
    std::memcpy(&len_net, ptr, 2); ptr += 2;
    out_header.payload_len = networkToHost16(len_net);

    if (len < AUDIO_HEADER_SIZE + out_header.payload_len) {
        return false; // Truncated payload
    }

    out_payload.assign(ptr, ptr + out_header.payload_len);
    return true;
}

std::vector<uint8_t> Protocol::serializeTcpMessage(
    TcpMessageType type,
    const std::string& json_payload) {

    uint32_t payload_len = static_cast<uint32_t>(json_payload.size());
    std::vector<uint8_t> packet(TCP_HEADER_SIZE + payload_len);
    uint8_t* ptr = packet.data();

    uint64_t magic_net = hostToNetwork64(TCP_HEADER_MAGIC);
    std::memcpy(ptr, &magic_net, 8); ptr += 8;

    *ptr = PROTOCOL_VERSION; ptr += 1;
    *ptr = static_cast<uint8_t>(type); ptr += 1;

    uint32_t len_net = hostToNetwork32(payload_len);
    std::memcpy(ptr, &len_net, 4); ptr += 4;

    if (!json_payload.empty()) {
        std::memcpy(ptr, json_payload.data(), payload_len);
    }

    return packet;
}

bool Protocol::deserializeTcpHeader(
    const uint8_t* buffer,
    size_t len,
    TcpMessageHeader& out_header) {

    if (!buffer || len < TCP_HEADER_SIZE) {
        return false;
    }

    const uint8_t* ptr = buffer;

    uint64_t magic_net;
    std::memcpy(&magic_net, ptr, 8); ptr += 8;
    out_header.magic = networkToHost64(magic_net);
    if (out_header.magic != TCP_HEADER_MAGIC) {
        return false;
    }

    out_header.version = *ptr; ptr += 1;
    out_header.type = static_cast<TcpMessageType>(*ptr); ptr += 1;

    uint32_t len_net;
    std::memcpy(&len_net, ptr, 4); ptr += 4;
    out_header.payload_len = networkToHost32(len_net);

    return true;
}

} // namespace multimic
