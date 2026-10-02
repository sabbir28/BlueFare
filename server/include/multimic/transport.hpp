#ifndef MULTIMIC_TRANSPORT_HPP
#define MULTIMIC_TRANSPORT_HPP

#include "multimic/types.hpp"
#include <string>
#include <vector>

namespace multimic {

struct TransportStats {
    uint64_t bytes_sent{0};
    uint64_t bytes_received{0};
    uint64_t packets_sent{0};
    uint64_t packets_received{0};
};

class IAudioTransport {
public:
    virtual ~IAudioTransport() = default;
    virtual bool connect(const std::string& host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual ssize_t send(const uint8_t* data, size_t len) = 0;
    virtual ssize_t receive(uint8_t* buffer, size_t max_len) = 0;
    virtual TransportStats getStats() const = 0;
    virtual std::string getTransportType() const = 0;
};

class WifiTransport : public IAudioTransport {
public:
    WifiTransport();
    ~WifiTransport() override;

    bool connect(const std::string& host, uint16_t port) override;
    void disconnect() override;
    ssize_t send(const uint8_t* data, size_t len) override;
    ssize_t receive(uint8_t* buffer, size_t max_len) override;
    TransportStats getStats() const override { return stats_; }
    std::string getTransportType() const override { return "Wi-Fi (UDP/IP)"; }

private:
    int fd_{-1};
    std::string remote_host_;
    uint16_t remote_port_{0};
    TransportStats stats_;
};

class BluetoothTransport : public IAudioTransport {
public:
    BluetoothTransport() = default;
    ~BluetoothTransport() override = default;

    bool connect(const std::string& host, uint16_t port) override { return false; }
    void disconnect() override {}
    ssize_t send(const uint8_t* data, size_t len) override { return -1; }
    ssize_t receive(uint8_t* buffer, size_t max_len) override { return -1; }
    TransportStats getStats() const override { return {}; }
    std::string getTransportType() const override { return "Bluetooth (Stub - Wi-Fi is primary)"; }
};

} // namespace multimic

#endif // MULTIMIC_TRANSPORT_HPP
