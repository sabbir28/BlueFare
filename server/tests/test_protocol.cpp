#include "multimic/protocol.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace multimic;

void test_audio_packet_serialization() {
    AudioFrameHeader header;
    header.magic = UDP_AUDIO_MAGIC;
    header.version = PROTOCOL_VERSION;
    header.type = 1;
    std::strncpy(header.device_id, "phone01_test", 15);
    header.session_id = 987654321;
    header.sequence_num = 123456789012345ULL;
    header.timestamp_us = 1600000000123456ULL;
    header.sample_rate = 48000;
    header.channels = 1;
    header.codec = static_cast<uint8_t>(CodecType::OPUS);

    std::vector<uint8_t> dummy_payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03};

    auto packet = Protocol::serializeAudioPacket(header, dummy_payload.data(), dummy_payload.size());
    assert(packet.size() == Protocol::AUDIO_HEADER_SIZE + dummy_payload.size());

    AudioFrameHeader unpacked_header;
    std::vector<uint8_t> unpacked_payload;
    bool success = Protocol::deserializeAudioPacket(
        packet.data(), packet.size(), unpacked_header, unpacked_payload);

    assert(success);
    assert(unpacked_header.magic == UDP_AUDIO_MAGIC);
    assert(unpacked_header.version == PROTOCOL_VERSION);
    assert(std::string(unpacked_header.device_id) == "phone01_test");
    assert(unpacked_header.session_id == 987654321);
    assert(unpacked_header.sequence_num == 123456789012345ULL);
    assert(unpacked_header.timestamp_us == 1600000000123456ULL);
    assert(unpacked_header.sample_rate == 48000);
    assert(unpacked_header.channels == 1);
    assert(unpacked_header.codec == static_cast<uint8_t>(CodecType::OPUS));
    assert(unpacked_payload == dummy_payload);

    std::cout << "[PASS] test_audio_packet_serialization\n";
}

void test_tcp_message_serialization() {
    std::string json_data = "{\"device_id\":\"phone01\",\"pin\":\"483921\"}";
    auto packet = Protocol::serializeTcpMessage(TcpMessageType::PAIR_REQUEST, json_data);
    assert(packet.size() == Protocol::TCP_HEADER_SIZE + json_data.size());

    TcpMessageHeader header;
    bool success = Protocol::deserializeTcpHeader(packet.data(), packet.size(), header);

    assert(success);
    assert(header.magic == TCP_HEADER_MAGIC);
    assert(header.version == PROTOCOL_VERSION);
    assert(header.type == TcpMessageType::PAIR_REQUEST);
    assert(header.payload_len == json_data.size());

    std::string payload(reinterpret_cast<const char*>(packet.data() + Protocol::TCP_HEADER_SIZE), header.payload_len);
    assert(payload == json_data);

    std::cout << "[PASS] test_tcp_message_serialization\n";
}

int main() {
    std::cout << "Running Protocol Unit Tests...\n";
    test_audio_packet_serialization();
    test_tcp_message_serialization();
    std::cout << "All Protocol Tests Passed!\n";
    return 0;
}
