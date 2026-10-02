#include "multimic/network.hpp"
#include "multimic/protocol.hpp"
#include "multimic/logger.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <chrono>

namespace multimic {

// UdpSocket Implementation
UdpSocket::UdpSocket() {}

UdpSocket::~UdpSocket() {
    closeSocket();
}

bool UdpSocket::bindPort(uint16_t port) {
    closeSocket();

    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        LOG_ERROR("Failed to create UDP socket");
        return false;
    }

    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind UDP socket on port " + std::to_string(port));
        closeSocket();
        return false;
    }

    return true;
}

bool UdpSocket::setNonBlocking(bool non_blocking) {
    if (fd_ < 0) return false;
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags < 0) return false;
    if (non_blocking) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;
    return fcntl(fd_, F_SETFL, flags) == 0;
}

bool UdpSocket::enableBroadcast(bool enable) {
    if (fd_ < 0) return false;
    int opt = enable ? 1 : 0;
    return setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) == 0;
}

ssize_t UdpSocket::sendTo(const std::string& ip, uint16_t port, const uint8_t* data, size_t len) {
    if (fd_ < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    return sendto(fd_, data, len, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

ssize_t UdpSocket::receiveFrom(uint8_t* buffer, size_t max_len, std::string& out_ip, uint16_t& out_port) {
    if (fd_ < 0) return -1;
    sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);

    ssize_t bytes = recvfrom(fd_, buffer, max_len, 0, reinterpret_cast<sockaddr*>(&addr), &addr_len);
    if (bytes > 0) {
        char ip_str[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
        out_ip = ip_str;
        out_port = ntohs(addr.sin_port);
    }
    return bytes;
}

void UdpSocket::closeSocket() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

// TcpServer Implementation
TcpServer::TcpServer() {}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start(uint16_t port, MessageCallback callback) {
    stop();

    port_ = port;
    callback_ = callback;

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        LOG_ERROR("Failed to create TCP listen socket");
        return false;
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind TCP server on port " + std::to_string(port_));
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    if (listen(listen_fd_, 16) < 0) {
        LOG_ERROR("Failed to listen on TCP server socket");
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&TcpServer::listenLoop, this);
    LOG_INFO("TCP Control server running on port " + std::to_string(port_));
    return true;
}

void TcpServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void TcpServer::listenLoop() {
    while (running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_fd_, &read_fds);

        timeval tv{1, 0}; // 1s timeout
        int ret = select(listen_fd_ + 1, &read_fds, nullptr, nullptr, &tv);
        if (ret <= 0 || !running_) continue;

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) continue;

        char ip_str[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        std::string client_ip = ip_str;

        LOG_DEBUG("New TCP connection from " + client_ip);

        // Handle request in worker lambda or thread
        std::thread([this, client_fd, client_ip]() {
            uint8_t buffer[4096];
            ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes >= static_cast<ssize_t>(Protocol::TCP_HEADER_SIZE)) {
                TcpMessageHeader header;
                if (Protocol::deserializeTcpHeader(buffer, bytes, header)) {
                    std::string payload;
                    if (header.payload_len > 0 && bytes >= static_cast<ssize_t>(Protocol::TCP_HEADER_SIZE + header.payload_len)) {
                        payload.assign(reinterpret_cast<char*>(buffer + Protocol::TCP_HEADER_SIZE), header.payload_len);
                    }
                    if (callback_) {
                        callback_(client_fd, client_ip, header.type, payload);
                    }
                }
            }
            close(client_fd);
        }).detach();
    }
}

bool TcpServer::sendToClient(int client_fd, TcpMessageType type, const std::string& json_payload) {
    auto packet = Protocol::serializeTcpMessage(type, json_payload);
    ssize_t sent = send(client_fd, packet.data(), packet.size(), MSG_NOSIGNAL);
    return sent == static_cast<ssize_t>(packet.size());
}

// DiscoveryBeacon Implementation
DiscoveryBeacon::DiscoveryBeacon() {}

DiscoveryBeacon::~DiscoveryBeacon() {
    stop();
}

bool DiscoveryBeacon::start(uint16_t discovery_port, uint16_t control_port, const std::string& server_name) {
    stop();

    discovery_port_ = discovery_port;
    control_port_ = control_port;
    server_name_ = server_name;

    if (!socket_.bindPort(0)) {
        return false;
    }
    socket_.enableBroadcast(true);

    running_ = true;
    beacon_thread_ = std::thread(&DiscoveryBeacon::beaconLoop, this);
    LOG_INFO("LAN Discovery Beacon started on broadcast port " + std::to_string(discovery_port_));
    return true;
}

void DiscoveryBeacon::stop() {
    running_ = false;
    socket_.closeSocket();
    if (beacon_thread_.joinable()) {
        beacon_thread_.join();
    }
}

void DiscoveryBeacon::beaconLoop() {
    std::string payload = "{\"msg\":\"MultiMic server available\",\"name\":\"" + server_name_ +
                          "\",\"control_port\":" + std::to_string(control_port_) + "}";

    while (running_) {
        socket_.sendTo("255.255.255.255", discovery_port_,
                       reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

} // namespace multimic
