#ifndef MULTIMIC_SERVER_HPP
#define MULTIMIC_SERVER_HPP

#include "multimic/config.hpp"
#include "multimic/device_manager.hpp"
#include "multimic/network.hpp"
#include "multimic/audio_receiver.hpp"
#include "multimic/audio_mixer.hpp"
#include "multimic/audio_output.hpp"
#include "multimic/web_server.hpp"
#include <memory>
#include <atomic>
#include <thread>

namespace multimic {

class MultiMicServer {
public:
    MultiMicServer(const ServerConfig& config);
    ~MultiMicServer();

    bool init();
    bool start();
    void stop();

    DeviceManager& getDeviceManager() { return device_manager_; }

private:
    void handleTcpControlMessage(int client_fd, const std::string& client_ip, TcpMessageType type, const std::string& payload);
    void mixingLoop();

    ServerConfig config_;
    DeviceManager device_manager_;
    TcpServer tcp_server_;
    DiscoveryBeacon beacon_;
    AudioReceiver audio_receiver_;
    AudioMixer audio_mixer_;
    std::unique_ptr<IAudioOutput> audio_output_;
    std::unique_ptr<WavWriter> wav_writer_;
    std::unique_ptr<WebServer> web_server_;

    std::atomic<bool> running_{false};
    std::thread mixer_thread_;
};

} // namespace multimic

#endif // MULTIMIC_SERVER_HPP
