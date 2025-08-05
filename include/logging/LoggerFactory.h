#ifndef REDIS_STATE_MANAGER_LOGGER_FACTORY_H
#define REDIS_STATE_MANAGER_LOGGER_FACTORY_H

#include "logging/LoggerIface.h"
#include <memory>
#include <mutex>

namespace StateManager {
namespace logging {

class LoggerFactory {
private:
    static std::shared_ptr<LoggerIface> global_logger_;
    static std::mutex logger_mutex_;
public:
    LoggerFactory();
    static std::shared_ptr<LoggerIface> getLogger();
    static void setLevel(const std::string level);
    static void setLevel(const LogLevel level);
    static void setLogger(std::shared_ptr<LoggerIface> logger);
};

} // namespace logging
} // namespace StateManager

#endif // REDIS_STATE_MANAGER_LOGGER_FACTORY_H
