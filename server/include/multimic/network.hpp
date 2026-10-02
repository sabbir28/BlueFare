#ifndef MULTIMIC_NETWORK_HPP
#define MULTIMIC_NETWORK_HPP

#include "multimic/types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <netinet/in.h>

namespace multimic {

class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    bool bindPort(uint16_t port);
    bool setNonBlocking(bool non_blocking);
    bool enableBroadcast(bool enable);
    
    ssize_t sendTo(const std::string& ip, uint16_t port, const uint8_t* data, size_t len);
    ssize_t receiveFrom(uint8_t* buffer, size_t max_len, std::string& out_ip, uint16_t& out_port);
    
    int getFd() const { return fd_; }
    void closeSocket();

private:
    int fd_{-1};
};

class TcpServer {
public:
    using MessageCallback = std::function<void(int client_fd, const std::string& client_ip, TcpMessageType type, const std::string& payload)>;

    TcpServer();
    ~TcpServer();

    bool start(uint16_t port, MessageCallback callback);
    void stop();

    bool sendToClient(int client_fd, TcpMessageType type, const std::string& json_payload);

private:
    void listenLoop();

    int listen_fd_{-1};
    uint16_t port_{0};
    MessageCallback callback_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    std::mutex client_mutex_;
};

class DiscoveryBeacon {
public:
    DiscoveryBeacon();
    ~DiscoveryBeacon();

    bool start(uint16_t discovery_port, uint16_t control_port, const std::string& server_name);
    void stop();

private:
    void beaconLoop();

    UdpSocket socket_;
    uint16_t discovery_port_{DEFAULT_DISCOVERY_PORT};
    uint16_t control_port_{DEFAULT_CONTROL_PORT};
    std::string server_name_;
    std::atomic<bool> running_{false};
    std::thread beacon_thread_;
};

} // namespace multimic

#endif // MULTIMIC_NETWORK_HPP
