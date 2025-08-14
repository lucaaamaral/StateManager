#include "logging/DefaultLogger.h"
#include "logging/LoggerFactory.h"
#include "logging/LoggerIface.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

using namespace StateManager::logging;

// Example 1: Advanced File Logger with rotation capabilities
class RotatingFileLogger : public LoggerIface {
private:
  std::string filename_base_;
  std::string current_filename_;
  std::ofstream log_file_;
  std::mutex log_mutex_;
  size_t max_file_size_; // in bytes
  size_t current_size_;
  int max_files_;
  int current_file_index_;
  LogLevel current_level_;

public:
  RotatingFileLogger(const std::string &filename_base,
                     size_t max_file_size = 1024 * 1024, // 1MB default
                     int max_files = 5)
      : filename_base_(filename_base), max_file_size_(max_file_size),
        current_size_(0), max_files_(max_files), current_file_index_(0),
        current_level_(LogLevel::INFO) {

    // Create the initial log file
    rotateLogFile();
  }

  ~RotatingFileLogger() override {
    if (log_file_.is_open()) {
      log_file_.close();
    }
  }

  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(log_mutex_);

    // Check if we need to rotate the log file
    if (current_size_ >= max_file_size_) {
      rotateLogFile();
    }

    if (log_file_.is_open()) {
      // Format the log message
      std::string formatted_message = formatLogMessage(level, message, context);

      // Write to file
      log_file_ << formatted_message << std::endl;

      // Update the current size
      current_size_ += formatted_message.size() + 1; // +1 for newline

      // Ensure it's written immediately
      log_file_.flush();
    }
  }

  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override {
    log(logLevelToString(level), message, context);
  }

  void setLevel(const std::string level) override {
    current_level_ = stringToLogLevel(level);
  }

  void setLevel(const LogLevel level) override { current_level_ = level; }

private:
  void rotateLogFile() {
    // Close the current file if open
    if (log_file_.is_open()) {
      log_file_.close();
    }

    // Update the file index
    current_file_index_ = (current_file_index_ + 1) % max_files_;

    // Create the new filename
    std::stringstream ss;
    ss << filename_base_ << "." << current_file_index_ << ".log";
    current_filename_ = ss.str();

    // Open the new file
    log_file_.open(current_filename_, std::ios::out | std::ios::trunc);
    if (!log_file_.is_open()) {
      std::cerr << "Failed to open log file: " << current_filename_
                << std::endl;
    }

    // Reset the current size
    current_size_ = 0;

    // Log a rotation message
    std::string rotation_message =
        formatLogMessage("INFO", "Log file rotated", "LogSystem");
    log_file_ << rotation_message << std::endl;
    current_size_ += rotation_message.size() + 1;
  }

  std::string formatLogMessage(const std::string &level,
                               const std::string &message,
                               const std::string &context) {
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

    std::stringstream ss;
    ss << "[" << timestamp.str() << "] [" << level << "]";

    // Add context if provided
    if (!context.empty()) {
      ss << " [" << context << "]";
    }

    ss << " " << message;
    return ss.str();
  }
};

// Example 2: Filtered Logger that only logs messages at or above a certain
// level
class FilteredLogger : public LoggerIface {
private:
  LogLevel min_level_;
  std::shared_ptr<LoggerIface> base_logger_;
  LogLevel current_level_;

public:
  FilteredLogger(LogLevel min_level, std::shared_ptr<LoggerIface> base_logger)
      : min_level_(min_level), base_logger_(std::move(base_logger)),
        current_level_(LogLevel::INFO) {}

  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    // Convert string level to enum
    LogLevel log_level = stringToLogLevel(level);

    // Only log if the level is at or above the minimum level
    if (log_level >= min_level_) {
      base_logger_->log(level, message, context);
    }
  }

  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override {
    if (level >= min_level_) {
      base_logger_->log(level, message, context);
    }
  }

  void setLevel(const std::string level) override {
    current_level_ = stringToLogLevel(level);
    base_logger_->setLevel(level);
  }

  void setLevel(const LogLevel level) override {
    current_level_ = level;
    base_logger_->setLevel(level);
  }
};

// Example 3: Asynchronous Logger that logs in a background thread
class AsyncLogger : public LoggerIface {
private:
  struct LogMessage {
    LogLevel level;
    std::string message;
    std::string context;
    bool is_string_level;
    std::string string_level;
  };

  std::shared_ptr<LoggerIface> base_logger_;
  std::vector<LogMessage> message_queue_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  std::thread worker_thread_;
  bool stop_thread_;
  size_t max_queue_size_;
  LogLevel current_level_;

public:
  AsyncLogger(std::shared_ptr<LoggerIface> base_logger,
              size_t max_queue_size = 1000)
      : base_logger_(std::move(base_logger)), stop_thread_(false),
        max_queue_size_(max_queue_size), current_level_(LogLevel::INFO) {

    // Start the worker thread
    worker_thread_ = std::thread(&AsyncLogger::processLogs, this);
  }

  ~AsyncLogger() override {
    // Stop the worker thread
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      stop_thread_ = true;
    }

    // Notify the worker thread
    cv_.notify_one();

    // Wait for the worker thread to finish
    if (worker_thread_.joinable()) {
      worker_thread_.join();
    }
  }

  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    // Check if the queue is full
    if (message_queue_.size() >= max_queue_size_) {
      // Drop the oldest message
      message_queue_.erase(message_queue_.begin());
    }

    // Add the message to the queue
    LogMessage log_message;
    log_message.is_string_level = true;
    log_message.string_level = level;
    log_message.message = message;
    log_message.context = context;

    message_queue_.push_back(log_message);

    // Notify the worker thread
    cv_.notify_one();
  }

  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(queue_mutex_);

    // Check if the queue is full
    if (message_queue_.size() >= max_queue_size_) {
      // Drop the oldest message
      message_queue_.erase(message_queue_.begin());
    }

    // Add the message to the queue
    LogMessage log_message;
    log_message.is_string_level = false;
    log_message.level = level;
    log_message.message = message;
    log_message.context = context;

    message_queue_.push_back(log_message);

    // Notify the worker thread
    cv_.notify_one();
  }

  void setLevel(const std::string level) override {
    current_level_ = stringToLogLevel(level);
    base_logger_->setLevel(level);
  }

  void setLevel(const LogLevel level) override {
    current_level_ = level;
    base_logger_->setLevel(level);
  }

private:
  void processLogs() {
    while (true) {
      std::vector<LogMessage> messages_to_process;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Wait for messages or stop signal
        cv_.wait(lock,
                 [this]() { return !message_queue_.empty() || stop_thread_; });

        // Check if we should stop
        if (stop_thread_ && message_queue_.empty()) {
          break;
        }

        // Move all messages to a local vector for processing
        messages_to_process = std::move(message_queue_);
        message_queue_.clear();
      }

      // Process all messages
      for (const auto &message : messages_to_process) {
        if (message.is_string_level) {
          base_logger_->log(message.string_level, message.message,
                            message.context);
        } else {
          base_logger_->log(message.level, message.message, message.context);
        }
      }
    }
  }
};

// Example 4: Performance Measuring Logger
class PerformanceLogger : public LoggerIface {
private:
  std::shared_ptr<LoggerIface> base_logger_;
  std::atomic<uint64_t> log_count_{0};
  std::chrono::steady_clock::time_point start_time_;
  LogLevel current_level_;

public:
  PerformanceLogger(std::shared_ptr<LoggerIface> base_logger)
      : base_logger_(std::move(base_logger)),
        start_time_(std::chrono::steady_clock::now()),
        current_level_(LogLevel::INFO) {}

  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    // Increment the log count
    log_count_++;

    // Forward to the base logger
    base_logger_->log(level, message, context);
  }

  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override {
    // Increment the log count
    log_count_++;

    // Forward to the base logger
    base_logger_->log(level, message, context);
  }

  // Get performance statistics
  void printStatistics() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time_)
            .count();

    if (elapsed > 0) {
      double logs_per_second = static_cast<double>(log_count_) / elapsed;

      std::stringstream ss;
      ss << "Logging statistics: " << log_count_ << " logs in " << elapsed
         << " seconds (" << logs_per_second << " logs/sec)";

      base_logger_->info(ss.str(), "PerformanceLogger");
    }
  }

  // Reset statistics
  void resetStatistics() {
    log_count_ = 0;
    start_time_ = std::chrono::steady_clock::now();
  }

  void setLevel(const std::string level) override {
    current_level_ = stringToLogLevel(level);
    base_logger_->setLevel(level);
  }

  void setLevel(const LogLevel level) override {
    current_level_ = level;
    base_logger_->setLevel(level);
  }
};

// Main function to demonstrate the advanced logging examples
int main() {
  try {
    std::cout << "Advanced Logging Examples" << std::endl;
    std::cout << "=======================" << std::endl;

    // Example 1: Rotating File Logger
    std::cout << "\nExample 1: Rotating File Logger" << std::endl;
    {
      // Create a rotating file logger with small max size for demonstration
      auto rotatingLogger = std::make_shared<RotatingFileLogger>(
          "app_log", 1024, 3); // 1KB max size, 3 files max

      // Set it as the global logger
      LoggerFactory::setLogger(rotatingLogger);

      // Generate some logs to demonstrate rotation
      auto logger = LoggerFactory::getLogger();
      for (int i = 0; i < 100; ++i) {
        std::string message = "Log message " + std::to_string(i) +
                              " with some additional text to make it longer";
        logger->info(message, "RotationDemo");
      }

      std::cout << "Created rotating log files: app_log.0.log, app_log.1.log, "
                   "app_log.2.log"
                << std::endl;
    }

    // Example 2: Filtered Logger
    std::cout << "\nExample 2: Filtered Logger" << std::endl;
    {
      // Create a default logger
      auto baseLogger = std::make_shared<DefaultLogger>();

      // Create a filtered logger that only logs WARNING and above
      auto filteredLogger =
          std::make_shared<FilteredLogger>(LogLevel::WARNING, baseLogger);

      // Set it as the global logger
      LoggerFactory::setLogger(filteredLogger);

      // Get the logger and log some messages
      auto logger = LoggerFactory::getLogger();

      std::cout << "The following logs should NOT appear (below WARNING level):"
                << std::endl;
      logger->trace("This is a trace message");
      logger->debug("This is a debug message");
      logger->info("This is an info message");

      std::cout
          << "\nThe following logs SHOULD appear (WARNING level and above):"
          << std::endl;
      logger->warning("This is a warning message");
      logger->error("This is an error message");
      logger->critical("This is a critical message");
    }

    // Example 3: Asynchronous Logger
    std::cout << "\nExample 3: Asynchronous Logger" << std::endl;
    {
      // Create a default logger
      auto baseLogger = std::make_shared<DefaultLogger>();

      // Create an asynchronous logger
      auto asyncLogger = std::make_shared<AsyncLogger>(baseLogger);

      // Set it as the global logger
      LoggerFactory::setLogger(asyncLogger);

      // Get the logger
      auto logger = LoggerFactory::getLogger();

      std::cout << "Logging 1000 messages asynchronously..." << std::endl;

      // Log a large number of messages
      auto start = std::chrono::steady_clock::now();

      for (int i = 0; i < 1000; ++i) {
        logger->info("Async log message " + std::to_string(i), "AsyncDemo");
      }

      auto end = std::chrono::steady_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();

      std::cout << "Queued 1000 messages in " << duration << "ms" << std::endl;
      std::cout << "Waiting for background processing to complete..."
                << std::endl;

      // Sleep to allow the async logger to process messages
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Example 4: Performance Measuring Logger
    std::cout << "\nExample 4: Performance Measuring Logger" << std::endl;
    {
      // Create a default logger
      auto baseLogger = std::make_shared<DefaultLogger>();

      // Create a performance logger
      auto perfLogger = std::make_shared<PerformanceLogger>(baseLogger);

      // Set it as the global logger
      LoggerFactory::setLogger(perfLogger);

      // Get the logger
      auto logger = LoggerFactory::getLogger();

      std::cout << "Logging messages and measuring performance..." << std::endl;

      // Log messages in a loop
      for (int i = 0; i < 1000; ++i) {
        logger->info("Performance test message " + std::to_string(i),
                     "PerfTest");
      }

      // Print statistics
      perfLogger->printStatistics();
    }

    // Example 5: Thread-safe Singleton demonstration
    std::cout << "\nExample 5: Thread-safe Singleton demonstration"
              << std::endl;
    {
      // Reset to default logger
      LoggerFactory::setLogger(std::make_shared<DefaultLogger>());

      // Create multiple threads that use the logger
      const int num_threads = 5;
      std::vector<std::thread> threads;

      std::cout << "Creating " << num_threads
                << " threads that log simultaneously..." << std::endl;

      for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
          // Each thread gets the logger instance
          auto logger = LoggerFactory::getLogger();

          // Log some messages
          for (int j = 0; j < 10; ++j) {
            std::stringstream ss;
            ss << "Thread " << i << " message " << j;
            logger->info(ss.str(), "ThreadDemo");

            // Sleep a bit to interleave logs
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
          }
        });
      }

      // Wait for all threads to complete
      for (auto &thread : threads) {
        thread.join();
      }

      std::cout << "All threads completed successfully" << std::endl;
    }

    std::cout << "\nAll examples completed successfully!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}