#include "multimic/transport.hpp"
#include "multimic/logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace multimic {

WifiTransport::WifiTransport() {}

WifiTransport::~WifiTransport() {
    disconnect();
}

bool WifiTransport::connect(const std::string& host, uint16_t port) {
    disconnect();

    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) return false;

    remote_host_ = host;
    remote_port_ = port;

    LOG_INFO("WifiTransport initialized target " + host + ":" + std::to_string(port));
    return true;
}

void WifiTransport::disconnect() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

ssize_t WifiTransport::send(const uint8_t* data, size_t len) {
    if (fd_ < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remote_port_);
    inet_pton(AF_INET, remote_host_.c_str(), &addr.sin_addr);

    ssize_t sent = sendto(fd_, data, len, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (sent > 0) {
        stats_.bytes_sent += sent;
        stats_.packets_sent++;
    }
    return sent;
}

ssize_t WifiTransport::receive(uint8_t* buffer, size_t max_len) {
    if (fd_ < 0) return -1;

    ssize_t recvd = recv(fd_, buffer, max_len, 0);
    if (recvd > 0) {
        stats_.bytes_received += recvd;
        stats_.packets_received++;
    }
    return recvd;
}

} // namespace multimic
