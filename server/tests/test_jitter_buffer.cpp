#include "multimic/audio_jitter_buffer.hpp"
#include <iostream>
#include <cassert>

using namespace multimic;

void test_jitter_buffer_ordering() {
    AudioJitterBuffer jb(40); // 40ms target

    std::vector<uint8_t> payload1 = {1, 2, 3};
    std::vector<uint8_t> payload2 = {4, 5, 6};
    std::vector<uint8_t> payload3 = {7, 8, 9};

    // Push out of order: 2, 1, 3
    jb.pushPacket(2, 20000, payload2);
    jb.pushPacket(1, 10000, payload1);
    jb.pushPacket(3, 30000, payload3);

    BufferedFrame f1, f2, f3;
    assert(jb.popNextFrame(f1));
    assert(f1.sequence_num == 1);
    assert(f1.opus_payload == payload1);

    assert(jb.popNextFrame(f2));
    assert(f2.sequence_num == 2);
    assert(f2.opus_payload == payload2);

    assert(jb.popNextFrame(f3));
    assert(f3.sequence_num == 3);
    assert(f3.opus_payload == payload3);

    std::cout << "[PASS] test_jitter_buffer_ordering\n";
}

void test_jitter_buffer_plc() {
    AudioJitterBuffer jb(20);

    jb.pushPacket(1, 10000, {10, 20});
    // Skip seq 2 (simulated packet loss)
    jb.pushPacket(3, 30000, {30, 40});

    BufferedFrame f1, f2, f3;
    assert(jb.popNextFrame(f1));
    assert(f1.sequence_num == 1);
    assert(!f1.is_missing);

    assert(jb.popNextFrame(f2));
    assert(f2.sequence_num == 2);
    assert(f2.is_missing); // Triggers PLC

    assert(jb.popNextFrame(f3));
    assert(f3.sequence_num == 3);
    assert(!f3.is_missing);

    std::cout << "[PASS] test_jitter_buffer_plc\n";
}

int main() {
    std::cout << "Running Jitter Buffer Unit Tests...\n";
    test_jitter_buffer_ordering();
    test_jitter_buffer_plc();
    std::cout << "All Jitter Buffer Tests Passed!\n";
    return 0;
}
