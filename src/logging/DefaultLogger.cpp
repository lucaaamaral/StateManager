#include "logging/DefaultLogger.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace StateManager {
namespace logging {

DefaultLogger::DefaultLogger(LogLevel level, const std::string &outputFile)
    : level_(level), use_file_output_(!outputFile.empty()) {
  if (!outputFile.empty()) {
    this->setOutputFile(outputFile);
  }
  this->trace("Starting DefaultLogger", "DefaultLogger");
}

void DefaultLogger::setLevel(const LogLevel level) {
  this->trace("Setting DefaultLogger level to " + logLevelToString(level),
              "DefaultLogger");
  std::lock_guard<std::mutex> lock(this->log_mutex_);
  this->level_ = level;
};

void DefaultLogger::setLevel(const std::string level) {
  this->setLevel(stringToLogLevel(level));
};

void DefaultLogger::setOutputFile(const std::string &outputFile) {
  std::lock_guard<std::mutex> lock(this->log_mutex_);
  this->file_stream_ =
      std::make_unique<std::ofstream>(outputFile, std::ios::app);
  if (this->file_stream_->is_open()) {
    this->use_file_output_ = true;
    // Check file size for basic log rotation
    this->file_stream_->seekp(0, std::ios::end);
    std::streampos size = this->file_stream_->tellp();
    if (size > 1024 * 1024) { // Rotate if larger than 1MB
      this->file_stream_->close();
      std::string backupFile = outputFile + ".bak";
      rename(outputFile.c_str(), backupFile.c_str());
      this->file_stream_ =
          std::make_unique<std::ofstream>(outputFile, std::ios::app);
      if (!this->file_stream_->is_open()) {
        this->use_file_output_ = false;
        std::cerr << "Failed to open log file after rotation: " << outputFile
                  << std::endl;
      }
    }
  } else {
    this->use_file_output_ = false;
    std::cerr << "Failed to open log file: " << outputFile << std::endl;
  }
}

void DefaultLogger::log(const std::string &level, const std::string &message,
                        const std::string &context) {
  std::lock_guard<std::mutex> lock(this->log_mutex_);
  // Get current time
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) %
                1000;

  std::stringstream timestamp;
  timestamp << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
  timestamp << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

  std::stringstream log_entry;
  log_entry << "[" << timestamp.str() << "] [" << level << "]";

  // Add context if provided
  if (!context.empty()) {
    log_entry << " [" << context << "]";
  }

  log_entry << " " << message << std::endl;

  if (this->use_file_output_ && this->file_stream_ &&
      this->file_stream_->is_open()) {
    *(this->file_stream_) << log_entry.str();
    this->file_stream_->flush();
  } else {
    std::cout << log_entry.str();
  }
}

void DefaultLogger::log(LogLevel level, const std::string &message,
                        const std::string &context) {
  if (static_cast<int>(level) < static_cast<int>(this->level_)) {
    return;
  }
  this->log(logLevelToString(level), message, context);
}

} // namespace logging
} // namespace StateManager