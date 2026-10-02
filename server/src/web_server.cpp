#include "multimic/web_server.hpp"
#include "multimic/logger.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <cstring>

namespace multimic {

WebServer::WebServer(DeviceManager& device_manager)
    : device_manager_(device_manager) {}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start(uint16_t port) {
    stop();
    port_ = port;

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return false;

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Failed to bind Web Server on port " + std::to_string(port_));
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    if (listen(listen_fd_, 16) < 0) {
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&WebServer::listenLoop, this);
    LOG_INFO("Web Dashboard server active at http://127.0.0.1:" + std::to_string(port_));
    return true;
}

void WebServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebServer::listenLoop() {
    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listen_fd_, &fds);
        timeval tv{1, 0};

        int ret = select(listen_fd_ + 1, &fds, nullptr, nullptr, &tv);
        if (ret <= 0 || !running_) continue;

        int client_fd = accept(listen_fd_, nullptr, nullptr);
        if (client_fd >= 0) {
            std::thread(&WebServer::handleClient, this, client_fd).detach();
        }
    }
}

void WebServer::handleClient(int client_fd) {
    char buffer[4096];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string request(buffer);

        if (request.find("GET /api/status") == 0) {
            std::string json = generateJsonStatus();
            std::stringstream ss;
            ss << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: "
               << json.size() << "\r\n\r\n" << json;
            std::string resp = ss.str();
            send(client_fd, resp.data(), resp.size(), 0);
        } else if (request.find("POST /api/volume") == 0) {
            // e.g. /api/volume?id=phone01&vol=0.8
            size_t id_pos = request.find("id=");
            size_t vol_pos = request.find("vol=");
            if (id_pos != std::string::npos && vol_pos != std::string::npos) {
                std::string id = request.substr(id_pos + 3, request.find('&', id_pos) - (id_pos + 3));
                float vol = std::stof(request.substr(vol_pos + 4, request.find(' ', vol_pos) - (vol_pos + 4)));
                device_manager_.setDeviceVolume(id, vol);
            }
            std::string json = "{\"status\":\"ok\"}";
            std::stringstream ss;
            ss << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << json.size() << "\r\n\r\n" << json;
            std::string resp = ss.str();
            send(client_fd, resp.data(), resp.size(), 0);
        } else if (request.find("POST /api/mute") == 0) {
            size_t id_pos = request.find("id=");
            size_t state_pos = request.find("state=");
            if (id_pos != std::string::npos && state_pos != std::string::npos) {
                std::string id = request.substr(id_pos + 3, request.find('&', id_pos) - (id_pos + 3));
                bool mute = (request.substr(state_pos + 6, request.find(' ', state_pos) - (state_pos + 6)) == "true");
                device_manager_.setDeviceMute(id, mute);
            }
            std::string json = "{\"status\":\"ok\"}";
            std::stringstream ss;
            ss << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << json.size() << "\r\n\r\n" << json;
            std::string resp = ss.str();
            send(client_fd, resp.data(), resp.size(), 0);
        } else {
            std::string html = generateHtmlDashboard();
            std::stringstream ss;
            ss << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << html.size() << "\r\n\r\n" << html;
            std::string resp = ss.str();
            send(client_fd, resp.data(), resp.size(), 0);
        }
    }
    close(client_fd);
}

std::string WebServer::generateJsonStatus() const {
    auto devices = device_manager_.getAllDevices();
    std::stringstream ss;
    ss << "{\"pairing_code\":\"" << device_manager_.getPairingCode() << "\",\"devices\":[";
    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& d = devices[i];
        ss << "{\"id\":\"" << d.device_id << "\","
           << "\"name\":\"" << d.display_name << "\","
           << "\"ip\":\"" << d.ip_address << "\","
           << "\"state\":\"" << deviceStateToString(d.state) << "\","
           << "\"volume\":" << d.volume << ","
           << "\"muted\":" << (d.is_muted ? "true" : "false") << ","
           << "\"rms\":" << d.stats.rms_level << ","
           << "\"peak\":" << d.stats.peak_level << ","
           << "\"packets_received\":" << d.stats.packets_received << ","
           << "\"packets_lost\":" << d.stats.packets_lost << "}";
        if (i + 1 < devices.size()) ss << ",";
    }
    ss << "]}";
    return ss.str();
}

std::string WebServer::generateHtmlDashboard() const {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>BlueFare MultiMic Server Dashboard</title>
<style>
  :root { --bg: #0f172a; --card: #1e293b; --accent: #38bdf8; --text: #f8fafc; --muted: #94a3b8; }
  body { font-family: system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 2rem; }
  .container { max-width: 900px; margin: 0 auto; }
  header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #334155; padding-bottom: 1rem; margin-bottom: 2rem; }
  h1 { margin: 0; color: var(--accent); font-size: 1.8rem; }
  .pin-badge { background: #0284c7; padding: 0.4rem 0.8rem; borderRadius: 6px; font-weight: bold; font-family: monospace; }
  .device-card { background: var(--card); border-radius: 12px; padding: 1.5rem; margin-bottom: 1rem; border: 1px solid #334155; display: grid; grid-template-columns: 1fr 120px 180px 100px; align-items: center; gap: 1rem; }
  .meter-bar { background: #334155; height: 16px; border-radius: 8px; overflow: hidden; position: relative; }
  .meter-fill { background: linear-gradient(90deg, #4ade80, #facc15, #f87171); height: 100%; width: 0%; transition: width 0.1s ease; }
  button { background: #ef4444; color: white; border: none; padding: 0.5rem 1rem; border-radius: 6px; cursor: pointer; font-weight: bold; }
  button.unmute { background: #22c55e; }
  input[type=range] { width: 100%; }
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>🎙️ BlueFare MultiMic Dashboard</h1>
    <div>Pairing PIN: <span class="pin-badge" id="pinCode">------</span></div>
  </header>
  <div id="deviceList">Loading connected Android devices...</div>
</div>
<script>
async function update() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();
    document.getElementById('pinCode').innerText = data.pairing_code;
    const container = document.getElementById('deviceList');
    if (data.devices.length === 0) {
      container.innerHTML = '<div style="color:var(--muted);text-align:center;padding:3rem;">No Android microphones connected. Enter PIN on your phone to connect.</div>';
      return;
    }
    container.innerHTML = data.devices.map(d => `
      <div class="device-card">
        <div>
          <strong style="font-size:1.1rem">${d.name}</strong> (${d.ip})<br>
          <small style="color:var(--muted)">State: ${d.state} | Pkts: ${d.packets_received} | Lost: ${d.packets_lost}</small>
        </div>
        <div class="meter-bar"><div class="meter-fill" style="width:${Math.min(100, Math.round(d.rms * 300))}%"></div></div>
        <div>
          Volume: <span id="vol-val-${d.id}">${Math.round(d.volume * 100)}%</span>
          <input type="range" min="0" max="2" step="0.05" value="${d.volume}" onchange="setVolume('${d.id}', this.value)">
        </div>
        <div>
          <button class="${d.muted ? 'unmute' : ''}" onclick="toggleMute('${d.id}', ${!d.muted})">${d.muted ? 'Unmute' : 'Mute'}</button>
        </div>
      </div>
    `).join('');
  } catch(e) {}
}
async function setVolume(id, vol) { await fetch(`/api/volume?id=${id}&vol=${vol}`, {method:'POST'}); update(); }
async function toggleMute(id, state) { await fetch(`/api/mute?id=${id}&state=${state}`, {method:'POST'}); update(); }
setInterval(update, 500); update();
</script>
</body>
</html>)HTML";
}

} // namespace multimic
