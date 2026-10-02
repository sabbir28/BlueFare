#include "multimic/device_manager.hpp"
#include <iostream>
#include <cassert>

using namespace multimic;

void test_device_manager_pairing_and_volume() {
    DeviceManager dm;
    dm.setPairingRequired(true);
    dm.setFixedPairingCode("483921");

    auto dev = dm.addOrUpdateDevice("phone01", "Sabbir Phone", "192.168.1.50");
    assert(dev.state == DeviceState::PAIRING_REQUIRED);

    uint32_t session_id = 0;
    bool invalid_pin_res = dm.verifyPairing("phone01", "000000", session_id);
    assert(!invalid_pin_res);

    bool valid_pin_res = dm.verifyPairing("phone01", "483921", session_id);
    assert(valid_pin_res);
    assert(session_id > 0);

    assert(dm.validateSession("phone01", session_id));

    dm.setDeviceVolume("phone01", 0.75f);
    DeviceInfo info;
    assert(dm.getDevice("phone01", info));
    assert(info.volume == 0.75f);

    std::cout << "[PASS] test_device_manager_pairing_and_volume\n";
}

int main() {
    std::cout << "Running Device Manager Unit Tests...\n";
    test_device_manager_pairing_and_volume();
    std::cout << "All Device Manager Tests Passed!\n";
    return 0;
}
