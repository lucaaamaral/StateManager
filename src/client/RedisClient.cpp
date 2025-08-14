#include <chrono>
#include <sstream>
#include <stdexcept>
#include <thread>

#include <sw/redis++/redis++.h>

#include "client/RedisClient.h"
#include "client/RedisConfig.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

// Utility function to convert RedisConfig to ConnectionOptions
sw::redis::ConnectionOptions toConnectionOptions(const RedisConfig &config) {
  sw::redis::ConnectionOptions options;
  options.host = config.host;
  options.port = config.port;
  if (!config.password.empty()) {
    options.password = config.password;
  }
  options.db = config.database;
  options.socket_timeout = std::chrono::milliseconds(config.timeout_ms);
  return options;
}

std::shared_ptr<RedisClient> RedisClient::instance_ = nullptr;
std::mutex RedisClient::instance_mutex_;

RedisClient::RedisClient()
    : redis_(nullptr),
      config_(StateManager::toConnectionOptions(RedisConfig())),
      logger_(logging::LoggerFactory::getLogger()) {}

void RedisClient::setConfig(const RedisConfig &config) {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<RedisClient>(new RedisClient());
  }
  instance_->config_ = StateManager::toConnectionOptions(config);
  instance_->connect();
  instance_->logger_->log(logging::LogLevel::DEBUG,
                          "Configuration updated for RedisClient",
                          "RedisClient");
}

sw::redis::Redis &RedisClient::getClient() {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<RedisClient>(new RedisClient());
  }
  if (!instance_->isConnected()) {
    instance_->logger_->log(
        logging::LogLevel::DEBUG,
        "Redis instance not connected, trying a connection.", "RedisClient");
    instance_->connect();
  }
  instance_->logger_->log(logging::LogLevel::DEBUG,
                          "Redis instance connected, returning redis.",
                          "RedisClient");
  if (instance_->redis_ == nullptr) {
    instance_->logger_->log(logging::LogLevel::DEBUG,
                            "Redis instance null pointer.", "RedisClient");
  }
  return *instance_->redis_;
}

std::unique_lock<std::mutex> RedisClient::lockClient() {
  std::lock_guard<std::mutex> instance_lock(instance_mutex_);
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<RedisClient>(new RedisClient());
  }
  return std::unique_lock<std::mutex>(instance_->redis_mutex_);
}

bool RedisClient::isConnected() const {
  std::lock_guard<std::mutex> lock(this->redis_mutex_);
  if (!this->redis_) {
    return false;
  }
  try {
    this->redis_->ping();
    this->logger_->log(logging::LogLevel::DEBUG, "Successful ping to redis.",
                       "RedisClient");
    return true;
  } catch (const std::exception &e) {
    this->logger_->log(logging::LogLevel::DEBUG, "Failed ping to redis.",
                       "RedisClient");
    return false;
  }
}

void RedisClient::connect() {
  std::lock_guard<std::mutex> lock(this->redis_mutex_);
  this->logger_->log(logging::LogLevel::INFO,
                     "Attempting RedisClient reconnection with configs: " +
                         this->config_._server_info(),
                     "RedisClient");
  const int max_retries = 5;
  const std::chrono::milliseconds base_delay(1000); // Base delay of 1 second
  const std::chrono::milliseconds max_delay(40000); // Cap delay at 40 seconds

  for (int attempt = 1; attempt <= max_retries; ++attempt) {
    try {
      this->redis_ = std::make_unique<sw::redis::Redis>(this->config_);
      this->logger_->log(logging::LogLevel::INFO,
                         "RedisClient connected successfully", "RedisClient");
      return;
    } catch (const std::exception &e) {
      std::ostringstream oss;
      oss << "Failed to connect RedisClient on attempt " << attempt << " of "
          << max_retries << ": " << e.what();
      this->logger_->log(logging::LogLevel::DEBUG, oss.str(), "RedisClient");
      if (attempt < max_retries) {
        // Delay with exponential backoff
        auto delay = std::min(base_delay * (1L << (attempt - 1)), max_delay);
        std::ostringstream delay_oss;
        delay_oss << "Waiting " << delay.count() << "ms before next attempt";
        this->logger_->log(logging::LogLevel::DEBUG, delay_oss.str(),
                           "RedisClient");
        std::this_thread::sleep_for(delay);
      }
    }
  }

  this->logger_->log(logging::LogLevel::ERROR,
                     "Failed to connect RedisClient after " +
                         std::to_string(max_retries) + " attempts",
                     "RedisClient");
}

RedisClient::~RedisClient() {
  this->logger_->log(logging::LogLevel::INFO, "RedisClient destroyed",
                     "RedisClient");
}

} // namespace StateManager