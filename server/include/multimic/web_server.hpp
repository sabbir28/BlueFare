#ifndef MULTIMIC_WEB_SERVER_HPP
#define MULTIMIC_WEB_SERVER_HPP

#include "multimic/types.hpp"
#include "multimic/device_manager.hpp"
#include <string>
#include <thread>
#include <atomic>

namespace multimic {

class WebServer {
public:
    WebServer(DeviceManager& device_manager);
    ~WebServer();

    bool start(uint16_t port);
    void stop();

private:
    void listenLoop();
    void handleClient(int client_fd);
    std::string generateHtmlDashboard() const;
    std::string generateJsonStatus() const;

    DeviceManager& device_manager_;
    uint16_t port_{DEFAULT_WEB_PORT};
    int listen_fd_{-1};
    std::atomic<bool> running_{false};
    std::thread server_thread_;
};

} // namespace multimic

#endif // MULTIMIC_WEB_SERVER_HPP
