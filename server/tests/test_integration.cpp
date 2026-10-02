#include "multimic/server.hpp"
#include "multimic/protocol.hpp"
#include "third_party/opus/opus.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <fstream>
#include <cmath>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


using namespace multimic;

void run_simulated_client_test() {
    ServerConfig config;
    config.control_port = 46454;
    config.audio_port = 46455;
    config.web_port = 48080;
    config.pairing_required = false; // Allow instant streaming for automated test
    config.output_driver = "dummy";
    config.record_path = "test_integration_output.wav";
    config.verbose = false;

    MultiMicServer server(config);
    assert(server.init());
    assert(server.start());

    // Register simulated device
    server.getDeviceManager().addOrUpdateDevice("test_phone_01", "Simulated Phone", "127.0.0.1");

    int opus_err = 0;
    OpusEncoder* encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_AUDIO, &opus_err);
    assert(opus_err == OPUS_OK && encoder != nullptr);

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(udp_fd >= 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(46455);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Generate 1 second of 440 Hz Sine wave PCM audio (50 frames of 20ms = 960 samples/frame)
    constexpr size_t frame_samples = 960;
    std::vector<int16_t> pcm_signal(frame_samples);

    uint8_t opus_payload[512];

    for (uint64_t seq = 1; seq <= 50; ++seq) {
        for (size_t i = 0; i < frame_samples; ++i) {
            double t = (static_cast<double>((seq - 1) * frame_samples + i)) / 48000.0;
            pcm_signal[i] = static_cast<int16_t>(16384.0 * std::sin(2.0 * M_PI * 440.0 * t));
        }

        opus_int32 encoded_bytes = opus_encode(encoder, pcm_signal.data(), frame_samples, opus_payload, sizeof(opus_payload));
        assert(encoded_bytes > 0);

        AudioFrameHeader header;
        header.magic = UDP_AUDIO_MAGIC;
        header.version = PROTOCOL_VERSION;
        header.type = 1;
        std::strncpy(header.device_id, "test_phone_01", 15);
        header.session_id = 0;
        header.sequence_num = seq;
        header.timestamp_us = seq * 20000;
        header.sample_rate = 48000;
        header.channels = 1;
        header.codec = static_cast<uint8_t>(CodecType::OPUS);

        auto packet = Protocol::serializeAudioPacket(header, opus_payload, encoded_bytes);
        sendto(udp_fd, packet.data(), packet.size(), 0, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Allow server 500ms to process and write WAV
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    close(udp_fd);
    opus_encoder_destroy(encoder);
    server.stop();

    // Verify written WAV file
    std::ifstream wav_file("test_integration_output.wav", std::ios::binary);
    assert(wav_file.is_open());
    wav_file.seekg(0, std::ios::end);
    size_t wav_size = wav_file.tellg();
    assert(wav_size > 44); // Header size + audio data

    std::cout << "[PASS] Integration Test Passed! (WAV recording generated: " << wav_size << " bytes)\n";
}

int main() {
    std::cout << "Running Integration End-to-End Test...\n";
    run_simulated_client_test();
    return 0;
}
