#include "multimic/server.hpp"
#include "multimic/logger.hpp"
#include <iostream>
#include <string>
#include <csignal>

static std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received. Stopping BlueFare MultiMic Server...\n";
        g_running = false;
    }
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n"
              << "Options:\n"
              << "  --port <PORT>        Control port (default: 45454)\n"
              << "  --audio-port <PORT>  UDP Audio port (default: 45455)\n"
              << "  --web-port <PORT>    Web Dashboard port (default: 8080)\n"
              << "  --config <FILE>      Configuration JSON file path\n"
              << "  --output <DRIVER>    Audio output driver: pipewire, alsa, dummy\n"
              << "  --record <WAV_FILE>  Record mixed audio to WAV file\n"
              << "  --code <PIN>         Set 6-digit pairing code\n"
              << "  --verbose            Enable verbose debug logging\n"
              << "  --help               Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    multimic::ServerConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--port" && i + 1 < argc) {
            config.control_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--audio-port" && i + 1 < argc) {
            config.audio_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--web-port" && i + 1 < argc) {
            config.web_port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--config" && i + 1 < argc) {
            config.loadFromFile(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            config.output_driver = argv[++i];
        } else if (arg == "--record" && i + 1 < argc) {
            config.record_path = argv[++i];
        } else if (arg == "--code" && i + 1 < argc) {
            config.fixed_pairing_code = argv[++i];
        } else if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
        }
    }

    multimic::MultiMicServer server(config);

    if (!server.init()) {
        std::cerr << "Failed to initialize server.\n";
        return 1;
    }

    if (!server.start()) {
        std::cerr << "Failed to start server.\n";
        return 1;
    }

    std::cout << "MultiMic Server is running. Press Ctrl+C to stop.\n";

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    server.stop();
    std::cout << "Server stopped successfully.\n";
    return 0;
}
