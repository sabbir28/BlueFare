#include "multimic/protocol.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace multimic;

bool sendCommand(const std::string& host, uint16_t port, const std::string& cmd_json, std::string& out_response) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return false;
    }

    auto packet = Protocol::serializeTcpMessage(TcpMessageType::CLI_COMMAND, cmd_json);
    send(fd, packet.data(), packet.size(), 0);

    uint8_t buffer[4096];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
    if (bytes >= static_cast<ssize_t>(Protocol::TCP_HEADER_SIZE)) {
        TcpMessageHeader header;
        if (Protocol::deserializeTcpHeader(buffer, bytes, header)) {
            if (header.payload_len > 0 && bytes >= static_cast<ssize_t>(Protocol::TCP_HEADER_SIZE + header.payload_len)) {
                out_response.assign(reinterpret_cast<char*>(buffer + Protocol::TCP_HEADER_SIZE), header.payload_len);
            }
        }
    }

    close(fd);
    return true;
}

void printHelp() {
    std::cout << "multimic-cli - BlueFare MultiMic Command Line Interface\n"
              << "Usage: multimic-cli <command> [args] [--host <IP>] [--port <PORT>]\n\n"
              << "Commands:\n"
              << "  devices               List all connected Android microphone devices\n"
              << "  status                Show server status and active streams\n"
              << "  mute <DEVICE_ID>      Mute specified device\n"
              << "  unmute <DEVICE_ID>    Unmute specified device\n"
              << "  volume <DEVICE_ID> <VOL>  Set device volume (0.0 to 2.0, e.g. 0.75)\n"
              << "  start <DEVICE_ID>     Start streaming for device\n"
              << "  stop <DEVICE_ID>      Stop streaming for device\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string command = argv[1];
    if (command == "--help" || command == "-h") {
        printHelp();
        return 0;
    }

    std::string host = "127.0.0.1";
    uint16_t port = DEFAULT_CONTROL_PORT;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) host = argv[++i];
        else if (arg == "--port" && i + 1 < argc) port = static_cast<uint16_t>(std::stoi(argv[++i]));
    }

    std::string cmd_json;
    if (command == "devices" || command == "status") {
        cmd_json = "{\"cmd\":\"devices\"}";
    } else if (command == "mute" && argc >= 3) {
        cmd_json = "{\"cmd\":\"mute\",\"device_id\":\"" + std::string(argv[2]) + "\"}";
    } else if (command == "unmute" && argc >= 3) {
        cmd_json = "{\"cmd\":\"unmute\",\"device_id\":\"" + std::string(argv[2]) + "\"}";
    } else if (command == "volume" && argc >= 4) {
        cmd_json = "{\"cmd\":\"volume\",\"device_id\":\"" + std::string(argv[2]) + "\",\"vol\":" + std::string(argv[3]) + "}";
    } else if (command == "start" && argc >= 3) {
        cmd_json = "{\"cmd\":\"start\",\"device_id\":\"" + std::string(argv[2]) + "\"}";
    } else if (command == "stop" && argc >= 3) {
        cmd_json = "{\"cmd\":\"stop\",\"device_id\":\"" + std::string(argv[2]) + "\"}";
    } else {
        printHelp();
        return 1;
    }

    std::string response;
    if (!sendCommand(host, port, cmd_json, response)) {
        std::cerr << "Error: Could not connect to multimic-server at " << host << ":" << port << "\n";
        return 1;
    }

    std::cout << response << "\n";
    return 0;
}
