#include "multimic/logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace multimic {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_level_ = level;
}

void Logger::setVerbose(bool verbose) {
    setLevel(verbose ? LogLevel::DEBUG : LogLevel::INFO);
}

LogLevel Logger::getLevel() const {
    return current_level_;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < current_level_) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();

    const char* level_str = "INFO";
    const char* color_code = "\033[0m";

    switch (level) {
        case LogLevel::DEBUG:
            level_str = "DEBUG";
            color_code = "\033[36m"; // Cyan
            break;
        case LogLevel::INFO:
            level_str = "INFO";
            color_code = "\033[32m"; // Green
            break;
        case LogLevel::WARN:
            level_str = "WARN";
            color_code = "\033[33m"; // Yellow
            break;
        case LogLevel::ERROR:
            level_str = "ERROR";
            color_code = "\033[31m"; // Red
            break;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << color_code << "[" << ss.str() << "] [" << level_str << "] " 
              << message << "\033[0m\n" << std::flush;
}

void Logger::debug(const std::string& message) { log(LogLevel::DEBUG, message); }
void Logger::info(const std::string& message)  { log(LogLevel::INFO, message); }
void Logger::warn(const std::string& message)  { log(LogLevel::WARN, message); }
void Logger::error(const std::string& message) { log(LogLevel::ERROR, message); }

} // namespace multimic
