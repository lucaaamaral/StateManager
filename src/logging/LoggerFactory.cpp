#include "logging/LoggerFactory.h"
#include "logging/DefaultLogger.h"

namespace StateManager {
namespace logging {

// Initialize static members of LoggerFactory
std::shared_ptr<LoggerIface> LoggerFactory::global_logger_ = nullptr;
std::mutex LoggerFactory::logger_mutex_;

std::shared_ptr<LoggerIface> LoggerFactory::getLogger() {
  std::lock_guard<std::mutex> lock(logger_mutex_);
  if (!global_logger_) {
    global_logger_ = std::make_shared<DefaultLogger>();
  }
  return global_logger_;
}

void LoggerFactory::setLevel(const LogLevel level) {
  std::shared_ptr<LoggerIface> logger = getLogger();
  std::lock_guard<std::mutex> lock(logger_mutex_);
  logger->setLevel(level);
};

void LoggerFactory::setLevel(const std::string level) {
  setLevel(stringToLogLevel(level));
};

void LoggerFactory::setLogger(std::shared_ptr<LoggerIface> logger) {
  std::lock_guard<std::mutex> lock(logger_mutex_);
  if (logger) {
    global_logger_ = logger;
  }
}

} // namespace logging
} // namespace StateManager