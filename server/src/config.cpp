#include "multimic/config.hpp"
#include "multimic/logger.hpp"
#include <fstream>
#include <sstream>
#include <regex>

namespace multimic {

static std::string extractJsonValue(const std::string& json, const std::string& key) {
    std::regex regex_str("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(json, match, regex_str)) {
        return match[1].str();
    }
    std::regex regex_num("\"" + key + "\"\\s*:\\s*([0-9]+)");
    if (std::regex_search(json, match, regex_num)) {
        return match[1].str();
    }
    std::regex regex_bool("\"" + key + "\"\\s*:\\s*(true|false)");
    if (std::regex_search(json, match, regex_bool)) {
        return match[1].str();
    }
    return "";
}

bool ServerConfig::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("Could not open config file: " + path + ". Using default settings.");
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    std::string val;
    if (!(val = extractJsonValue(json, "control_port")).empty()) control_port = static_cast<uint16_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "audio_port")).empty()) audio_port = static_cast<uint16_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "discovery_port")).empty()) discovery_port = static_cast<uint16_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "web_port")).empty()) web_port = static_cast<uint16_t>(std::stoi(val));

    if (!(val = extractJsonValue(json, "sample_rate")).empty()) sample_rate = static_cast<uint32_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "channels")).empty()) channels = static_cast<uint8_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "frame_ms")).empty()) frame_ms = static_cast<uint32_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "jitter_buffer_ms")).empty()) jitter_buffer_ms = static_cast<uint32_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "output_driver")).empty()) output_driver = val;
    if (!(val = extractJsonValue(json, "record_path")).empty()) record_path = val;

    if (!(val = extractJsonValue(json, "bitrate")).empty()) opus_bitrate = static_cast<uint32_t>(std::stoi(val));
    if (!(val = extractJsonValue(json, "pairing_required")).empty()) pairing_required = (val == "true");
    if (!(val = extractJsonValue(json, "pairing_code")).empty()) fixed_pairing_code = val;

    LOG_INFO("Loaded configuration from: " + path);
    return true;
}

std::string ServerConfig::toJsonString() const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"server\": {\n"
       << "    \"control_port\": " << control_port << ",\n"
       << "    \"audio_port\": " << audio_port << ",\n"
       << "    \"discovery_port\": " << discovery_port << ",\n"
       << "    \"web_port\": " << web_port << "\n"
       << "  },\n"
       << "  \"audio\": {\n"
       << "    \"sample_rate\": " << sample_rate << ",\n"
       << "    \"channels\": " << static_cast<int>(channels) << ",\n"
       << "    \"frame_ms\": " << frame_ms << ",\n"
       << "    \"jitter_buffer_ms\": " << jitter_buffer_ms << ",\n"
       << "    \"output_driver\": \"" << output_driver << "\",\n"
       << "    \"record_path\": \"" << record_path << "\"\n"
       << "  },\n"
       << "  \"opus\": {\n"
       << "    \"bitrate\": " << opus_bitrate << "\n"
       << "  },\n"
       << "  \"security\": {\n"
       << "    \"pairing_required\": " << (pairing_required ? "true" : "false") << ",\n"
       << "    \"pairing_code\": \"" << fixed_pairing_code << "\"\n"
       << "  }\n"
       << "}\n";
    return ss.str();
}

bool ServerConfig::saveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file for writing config: " + path);
        return false;
    }
    file << toJsonString();
    return true;
}

} // namespace multimic
