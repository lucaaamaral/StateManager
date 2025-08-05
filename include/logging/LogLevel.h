#ifndef REDIS_STATE_MANAGER_LOG_LEVEL_H
#define REDIS_STATE_MANAGER_LOG_LEVEL_H

#include <string>

namespace StateManager {
namespace logging {

// Log levels for more structured logging
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// Convert LogLevel enum to string
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// Convert string to LogLevel enum (case insensitive)
inline LogLevel stringToLogLevel(const std::string& level) {
    if (level == "TRACE") return LogLevel::TRACE;
    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO") return LogLevel::INFO;
    if (level == "WARNING") return LogLevel::WARNING;
    if (level == "ERROR") return LogLevel::ERROR;
    if (level == "CRITICAL") return LogLevel::CRITICAL;
    return LogLevel::ERROR;
}

} // namespace logging
} // namespace StateManager

#endif // REDIS_STATE_MANAGER_LOG_LEVEL_H