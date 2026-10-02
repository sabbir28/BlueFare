#include "multimic/server.hpp"
#include "multimic/logger.hpp"
#include "multimic/protocol.hpp"
#include <sstream>
#include <chrono>

namespace multimic {

MultiMicServer::MultiMicServer(const ServerConfig& config)
    : config_(config),
      audio_receiver_(device_manager_, config.jitter_buffer_ms),
      audio_mixer_(SAMPLES_PER_FRAME, config.sample_rate) {}

MultiMicServer::~MultiMicServer() {
    stop();
}

bool MultiMicServer::init() {
    Logger::getInstance().setVerbose(config_.verbose);

    LOG_INFO("==================================================");
    LOG_INFO("Initializing BlueFare MultiMic Server");
    LOG_INFO("Control Port: " + std::to_string(config_.control_port));
    LOG_INFO("Audio Port:   " + std::to_string(config_.audio_port));
    LOG_INFO("Web Port:     " + std::to_string(config_.web_port));
    LOG_INFO("Sample Rate:  " + std::to_string(config_.sample_rate) + " Hz");
    LOG_INFO("==================================================");

    device_manager_.setPairingRequired(config_.pairing_required);
    if (!config_.fixed_pairing_code.empty()) {
        device_manager_.setFixedPairingCode(config_.fixed_pairing_code);
    }

    LOG_INFO("System Pairing PIN: " + device_manager_.getPairingCode());

    // Audio Output Initialization
    audio_output_ = createAudioOutputDriver(config_.output_driver);
    if (!audio_output_) {
        LOG_ERROR("Failed to create audio output driver");
        return false;
    }
    LOG_INFO("Audio Output Driver: " + audio_output_->getDriverName());

    // WAV Recording Initialization
    if (!config_.record_path.empty()) {
        wav_writer_ = std::make_unique<WavWriter>();
        if (!wav_writer_->open(config_.record_path, config_.sample_rate, config_.channels)) {
            LOG_WARN("Could not open WAV recording path: " + config_.record_path);
        }
    }

    // Web Dashboard Server
    web_server_ = std::make_unique<WebServer>(device_manager_);

    return true;
}

bool MultiMicServer::start() {
    stop();
    running_ = true;

    // Start TCP Control Server
    if (!tcp_server_.start(config_.control_port,
        [this](int fd, const std::string& ip, TcpMessageType type, const std::string& payload) {
            handleTcpControlMessage(fd, ip, type, payload);
        })) {
        return false;
    }

    // Start UDP Audio Receiver
    if (!audio_receiver_.start(config_.audio_port)) {
        tcp_server_.stop();
        return false;
    }

    // Start LAN Discovery Beacon
    beacon_.start(config_.discovery_port, config_.control_port, "MultiMic-Linux");

    // Start Web Server
    web_server_->start(config_.web_port);

    // Start Realtime Mixer thread
    mixer_thread_ = std::thread(&MultiMicServer::mixingLoop, this);

    LOG_INFO("MultiMic Server fully operational!");
    return true;
}

void MultiMicServer::stop() {
    running_ = false;
    beacon_.stop();
    tcp_server_.stop();
    audio_receiver_.stop();
    if (web_server_) web_server_->stop();

    if (mixer_thread_.joinable()) {
        mixer_thread_.join();
    }

    if (wav_writer_) {
        wav_writer_->close();
    }
    if (audio_output_) {
        audio_output_->close();
    }
}

void MultiMicServer::handleTcpControlMessage(
    int client_fd, const std::string& client_ip, TcpMessageType type, const std::string& payload) {

    LOG_DEBUG("Control Message type " + std::to_string(static_cast<int>(type)) + " from " + client_ip);

    if (type == TcpMessageType::DISCOVERY_REQUEST) {
        std::string json = "{\"server_name\":\"MultiMic-Linux\",\"control_port\":" +
                           std::to_string(config_.control_port) + ",\"audio_port\":" +
                           std::to_string(config_.audio_port) + ",\"status\":\"AVAILABLE\"}";
        tcp_server_.sendToClient(client_fd, TcpMessageType::DISCOVERY_RESPONSE, json);
    }
    else if (type == TcpMessageType::PAIR_REQUEST) {
        // payload: {"device_id":"phone01","name":"Sabbir Phone","pin":"483921"}
        std::string device_id = "phone01";
        std::string name = "Android Phone";
        std::string pin = "";

        size_t id_pos = payload.find("\"device_id\":\"");
        if (id_pos != std::string::npos) {
            size_t end = payload.find("\"", id_pos + 13);
            device_id = payload.substr(id_pos + 13, end - (id_pos + 13));
        }
        size_t name_pos = payload.find("\"name\":\"");
        if (name_pos != std::string::npos) {
            size_t end = payload.find("\"", name_pos + 8);
            name = payload.substr(name_pos + 8, end - (name_pos + 8));
        }
        size_t pin_pos = payload.find("\"pin\":\"");
        if (pin_pos != std::string::npos) {
            size_t end = payload.find("\"", pin_pos + 7);
            pin = payload.substr(pin_pos + 7, end - (pin_pos + 7));
        }

        device_manager_.addOrUpdateDevice(device_id, name, client_ip);

        uint32_t session_id = 0;
        if (device_manager_.verifyPairing(device_id, pin, session_id)) {
            std::string resp = "{\"status\":\"OK\",\"session_id\":" + std::to_string(session_id) + ",\"audio_port\":" + std::to_string(config_.audio_port) + "}";
            tcp_server_.sendToClient(client_fd, TcpMessageType::PAIR_RESPONSE, resp);
        } else {
            std::string resp = "{\"status\":\"ERROR\",\"message\":\"Invalid PIN pairing code\"}";
            tcp_server_.sendToClient(client_fd, TcpMessageType::PAIR_RESPONSE, resp);
        }
    }
    else if (type == TcpMessageType::START_STREAM_REQUEST) {
        std::string resp = "{\"status\":\"OK\",\"audio_port\":" + std::to_string(config_.audio_port) + "}";
        tcp_server_.sendToClient(client_fd, TcpMessageType::START_STREAM_RESPONSE, resp);
    }
    else if (type == TcpMessageType::STOP_STREAM_REQUEST) {
        std::string resp = "{\"status\":\"OK\"}";
        tcp_server_.sendToClient(client_fd, TcpMessageType::STOP_STREAM_RESPONSE, resp);
    }
    else if (type == TcpMessageType::CLI_COMMAND) {
        // CLI request handling
        std::string response_json;
        if (payload.find("devices") != std::string::npos) {
            auto devices = device_manager_.getAllDevices();
            std::stringstream ss;
            ss << "{\"devices\":[";
            for (size_t i = 0; i < devices.size(); ++i) {
                const auto& d = devices[i];
                ss << "{\"id\":\"" << d.device_id << "\",\"name\":\"" << d.display_name << "\",\"ip\":\"" << d.ip_address
                   << "\",\"state\":\"" << deviceStateToString(d.state) << "\",\"volume\":" << d.volume
                   << ",\"muted\":" << (d.is_muted ? "true" : "false") << "}";
                if (i + 1 < devices.size()) ss << ",";
            }
            ss << "]}";
            response_json = ss.str();
        } else if (payload.find("volume") != std::string::npos) {
            // e.g. {"cmd":"volume","device_id":"phone01","vol":0.75}
            size_t id_pos = payload.find("\"device_id\":\"");
            size_t vol_pos = payload.find("\"vol\":");
            if (id_pos != std::string::npos && vol_pos != std::string::npos) {
                std::string dev_id = payload.substr(id_pos + 13, payload.find("\"", id_pos + 13) - (id_pos + 13));
                float vol = std::stof(payload.substr(vol_pos + 6));
                device_manager_.setDeviceVolume(dev_id, vol);
                response_json = "{\"status\":\"OK\"}";
            } else {
                response_json = "{\"status\":\"ERROR\",\"message\":\"Invalid arguments\"}";
            }
        } else if (payload.find("mute") != std::string::npos) {
            size_t id_pos = payload.find("\"device_id\":\"");
            if (id_pos != std::string::npos) {
                std::string dev_id = payload.substr(id_pos + 13, payload.find("\"", id_pos + 13) - (id_pos + 13));
                bool mute = (payload.find("\"unmute\"") == std::string::npos);
                device_manager_.setDeviceMute(dev_id, mute);
                response_json = "{\"status\":\"OK\"}";
            } else {
                response_json = "{\"status\":\"ERROR\",\"message\":\"Invalid device ID\"}";
            }
        } else {
            response_json = "{\"status\":\"OK\"}";
        }
        tcp_server_.sendToClient(client_fd, TcpMessageType::CLI_RESPONSE, response_json);
    }
}

void MultiMicServer::mixingLoop() {
    constexpr size_t frame_size = SAMPLES_PER_FRAME;
    std::vector<float> master_mix(frame_size, 0.0f);
    std::unordered_map<std::string, std::vector<float>> pcm_frames;
    std::unordered_map<std::string, const float*> channel_ptrs;
    std::unordered_map<std::string, std::pair<float, float>> meters;

    auto frame_duration = std::chrono::milliseconds(DEFAULT_FRAME_MS);

    while (running_) {
        auto start_time = std::chrono::steady_clock::now();

        audio_receiver_.getDecodedFrameBuffers(frame_size, pcm_frames);

        channel_ptrs.clear();
        for (const auto& [id, buffer] : pcm_frames) {
            channel_ptrs[id] = buffer.data();
        }

        audio_mixer_.mixChannels(channel_ptrs, master_mix.data(), frame_size, meters);

        // Update device audio level meters
        for (const auto& [id, meter] : meters) {
            device_manager_.updateMeters(id, meter.first, meter.second);
        }

        // Output to PipeWire/ALSA soundcard
        if (audio_output_) {
            audio_output_->writeSamples(master_mix.data(), frame_size);
        }

        // Output to WAV Recorder
        if (wav_writer_) {
            wav_writer_->writeFloatSamples(master_mix.data(), frame_size);
        }

        // Precise frame pacing (20ms)
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }
}

} // namespace multimic
