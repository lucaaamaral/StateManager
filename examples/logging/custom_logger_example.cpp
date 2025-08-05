#include "Logging.h"
#include "core/StateManager.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>

// Example of a custom logger that writes to a file
class FileLogger : public StateManager::logging::LoggerIface {
private:
  std::ofstream log_file_;
  std::string prefix_;
  std::mutex log_mutex_;

public:
  FileLogger(const std::string &filename, const std::string &prefix = "")
      : prefix_(prefix) {
    log_file_.open(filename, std::ios::out | std::ios::app);
    if (!log_file_.is_open()) {
      throw std::runtime_error("Failed to open log file: " + filename);
    }
  }

  ~FileLogger() override {
    if (log_file_.is_open()) {
      log_file_.close();
    }
  }

  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (log_file_.is_open()) {
      // Get current time
      auto now = std::chrono::system_clock::now();
      auto now_time_t = std::chrono::system_clock::to_time_t(now);
      auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    1000;

      std::stringstream timestamp;
      timestamp << std::put_time(std::localtime(&now_time_t),
                                 "%Y-%m-%d %H:%M:%S");
      timestamp << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

      log_file_ << "[" << timestamp.str() << "] [" << level << "]";

      // Add context if provided
      if (!context.empty()) {
        log_file_ << " [" << context << "]";
      }

      log_file_ << " " << prefix_ << message << std::endl;
    }
  }

  // Override the enum-based log method for better performance
  void log(StateManager::logging::LogLevel level, const std::string &message,
           const std::string &context = "") override {
    log(StateManager::logging::logLevelToString(level), message, context);
  }
};

// Example of a custom logger that sends logs to multiple destinations
class MultiLogger : public StateManager::logging::LoggerIface {
private:
  std::vector<std::shared_ptr<StateManager::logging::LoggerIface>> loggers_;
  std::mutex loggers_mutex_;

public:
  void addLogger(std::shared_ptr<StateManager::logging::LoggerIface> logger) {
    if (logger) {
      std::lock_guard<std::mutex> lock(loggers_mutex_);
      loggers_.push_back(logger);
    }
  }

  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    for (auto &logger : loggers_) {
      logger->log(level, message, context);
    }
  }

  void log(StateManager::logging::LogLevel level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(loggers_mutex_);
    for (auto &logger : loggers_) {
      logger->log(level, message, context);
    }
  }
};

int main() {
  try {
    // Example 1: Using the default logger
    std::cout << "Example 1: Using the default logger" << std::endl;
    {
      // The default logger will be used automatically
      StateManager::RedisConfig config;
      config.host = "localhost";
      config.port = 6379;

      auto stateManager = StateManager::StateManager::createWithDefaults();
      // Use the state manager...
    }

    // Example 2: Using a custom file logger
    std::cout << "\nExample 2: Using a custom file logger" << std::endl;
    {
      // Create a custom file logger
      auto fileLogger = std::make_shared<FileLogger>("redis_state_manager.log",
                                                     "[CustomLogger] ");

      // Set it as the global logger
      StateManager::logging::LoggerFactory::setLogger(fileLogger);

      StateManager::RedisConfig config;
      config.host = "localhost";
      config.port = 6379;

      auto stateManager = StateManager::StateManager::createWithDefaults();
      // Use the state manager...
    }

    // Example 3: Using a multi-destination logger
    std::cout << "\nExample 3: Using a multi-destination logger" << std::endl;
    {
      // Create a multi-destination logger
      auto multiLogger = std::make_shared<MultiLogger>();

      // Add a console logger (the default logger)
      multiLogger->addLogger(StateManager::logging::LoggerFactory::getLogger());

      // Add a file logger
      multiLogger->addLogger(
          std::make_shared<FileLogger>("redis_state_manager_multi.log"));

      // Set it as the global logger
      StateManager::logging::LoggerFactory::setLogger(multiLogger);

      StateManager::RedisConfig config;
      config.host = "localhost";
      config.port = 6379;

      auto stateManager = StateManager::StateManager::createWithDefaults();
      // Use the state manager...
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}