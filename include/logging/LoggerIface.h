#ifndef REDIS_STATE_MANAGER_LOGGER_IFACE_H
#define REDIS_STATE_MANAGER_LOGGER_IFACE_H

#include "logging/LogLevel.h"
#include <string>

namespace StateManager {
namespace logging {

class LoggerIface {
public:
    virtual ~LoggerIface() = default;
    virtual void setLevel(const std::string level) = 0;
    virtual void setLevel(const LogLevel level) = 0;
    virtual void log(const std::string& level, const std::string& message, const std::string& context = "") = 0;
    virtual void log(LogLevel level, const std::string& message, const std::string& context = "") {
        // Default implementation calls the string-based version
        log(logLevelToString(level), message, context);
    }
    
    // Convenience methods for different log levels
    virtual void trace(const std::string& message, const std::string& context = "") {
        log(LogLevel::TRACE, message, context);
    }
    
    virtual void debug(const std::string& message, const std::string& context = "") {
        log(LogLevel::DEBUG, message, context);
    }
    
    virtual void info(const std::string& message, const std::string& context = "") {
        log(LogLevel::INFO, message, context);
    }
    
    virtual void warning(const std::string& message, const std::string& context = "") {
        log(LogLevel::WARNING, message, context);
    }
    
    virtual void error(const std::string& message, const std::string& context = "") {
        log(LogLevel::ERROR, message, context);
    }
    
    virtual void critical(const std::string& message, const std::string& context = "") {
        log(LogLevel::CRITICAL, message, context);
    }
};

} // namespace logging
} // namespace StateManager

#endif // REDIS_STATE_MANAGER_LOGGER_IFACE_H