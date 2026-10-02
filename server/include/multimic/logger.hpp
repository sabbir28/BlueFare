#ifndef MULTIMIC_LOGGER_HPP
#define MULTIMIC_LOGGER_HPP

#include <string>
#include <mutex>

namespace multimic {

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static Logger& getInstance();

    void setLevel(LogLevel level);
    void setVerbose(bool verbose);
    LogLevel getLevel() const;

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel current_level_{LogLevel::INFO};
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) multimic::Logger::getInstance().debug(msg)
#define LOG_INFO(msg)  multimic::Logger::getInstance().info(msg)
#define LOG_WARN(msg)  multimic::Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) multimic::Logger::getInstance().error(msg)

} // namespace multimic

#endif // MULTIMIC_LOGGER_HPP
