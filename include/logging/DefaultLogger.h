#ifndef REDIS_STATE_MANAGER_DEFAULT_LOGGER_H
#define REDIS_STATE_MANAGER_DEFAULT_LOGGER_H

#include "logging/LoggerIface.h"
#include <fstream>
#include <memory>
#include <mutex>

namespace StateManager {
namespace logging {

class DefaultLogger : public LoggerIface {
public:
  DefaultLogger(LogLevel level = LogLevel::WARNING,
                const std::string &outputFile = "");
  void setLevel(const std::string level) override;
  void setLevel(const LogLevel level) override;
  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override;
  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override;
  void setOutputFile(const std::string &outputFile);

private:
  std::mutex log_mutex_;
  LogLevel level_;
  std::unique_ptr<std::ofstream> file_stream_;
  bool use_file_output_ = false;
};

} // namespace logging
} // namespace StateManager

#endif // REDIS_STATE_MANAGER_DEFAULT_LOGGER_H
