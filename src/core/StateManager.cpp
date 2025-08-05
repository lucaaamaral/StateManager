#include <iostream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "client/RedisClient.h"
#include "client/RedisChannel.h"
#include "client/RedisStorage.h"
#include "core/StateManager.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

using json = nlohmann::json;
using error = std::optional<std::string>;

StateManager::StateManager(const RedisConfig &config)
    : logger_(logging::LoggerFactory::getLogger()), config_(config) {
  RedisClient::setConfig(config);
  this->log(logging::LogLevel::INFO, "StateManager initialized successfully");
}

StateManager::~StateManager() {
  this->log(logging::LogLevel::INFO, "StateManager destroyed");
}

void StateManager::setConfig(const RedisConfig& config) {
  RedisClient::setConfig(config);
}

bool StateManager::write(const std::string &key, const json &value) {
  try {
    std::string json_string = value.dump();
    
    // Use RedisStorage to write the JSON string
    RedisStorage storage;
    bool success = storage.write(key, json_string);
    
    if (success) {
      this->log(logging::LogLevel::INFO, "Successfully wrote value to key: " + key);
    } else {
      this->log(logging::LogLevel::ERROR, "Failed to write value to key: " + key);
    }
    
    return success;
  } catch (const json::exception& e) {
    this->log(logging::LogLevel::ERROR, "JSON serialization error for key " + key + ": " + e.what());
    return false;
  } catch (const std::exception& e) {
    this->log(logging::LogLevel::ERROR, "Error writing to key " + key + ": " + e.what());
    return false;
  }
}

bool StateManager::erase(const std::string &key) {
  try {
    RedisStorage storage;
    bool success = storage.erase(key);
    
    if (success) {
      this->log(logging::LogLevel::INFO, "Successfully erased key: " + key);
    } else {
      this->log(logging::LogLevel::ERROR, "Failed to erase key: " + key);
    }
    
    return success;
  } catch (const std::exception& e) {
    this->log(logging::LogLevel::ERROR, "Error erasing key " + key + ": " + e.what());
    return false;
  }
}

std::pair<error, json> StateManager::read(const std::string &key) {
  try {
    RedisStorage storage;
    std::string json_string = storage.read(key);
    
    if (json_string.empty()) {
      this->log(logging::LogLevel::WARNING, "Key not found or empty: " + key);
      return std::make_pair(std::make_optional("Key not found"), json{});
    }
    
    // Parse JSON string back to object
    json json_value = json::parse(json_string);
    
    this->log(logging::LogLevel::INFO, "Successfully read value from key: " + key);
    return std::make_pair(std::nullopt, json_value);

  } catch (const json::exception& e) {
    this->log(logging::LogLevel::ERROR, "JSON deserialization error for key " + key + ": " + e.what());
    return std::make_pair(std::make_optional("JSON parsing error"), json{});
  } catch (const std::exception& e) {
    this->log(logging::LogLevel::ERROR, "Error reading from key " + key + ": " + e.what());
    return std::make_pair(std::make_optional("Storage error"), json{});
  }
}

void StateManager::subscribe(const std::string& channel, std::function<void(const json&)> callback) {
  try {
    RedisChannel& redis_channel = RedisChannel::getInstance();
    
    // Wrap the user callback to handle JSON deserialization
    auto wrapped_callback = [this, callback, channel](const std::string& message) {
      json json_value = json::parse(message);
      callback(json_value);
      this->log(logging::LogLevel::DEBUG, "Successfully processed message on channel: " + channel);
    };
    
    redis_channel.subscribe(channel, wrapped_callback);
    this->log(logging::LogLevel::INFO, "Successfully subscribed to channel: " + channel);
    
  } catch (const std::exception& e) {
    this->log(logging::LogLevel::ERROR, "Error subscribing to channel " + channel + ": " + e.what());
  }
}

void StateManager::unsubscribe(const std::string& channel) {
  RedisChannel& redis_channel = RedisChannel::getInstance();
  redis_channel.unsubscribe(channel);
  this->log(logging::LogLevel::INFO, "Successfully unsubscribed from channel: " + channel);
}

bool StateManager::publish(const std::string& channel, const json& data) {
  try {
    std::string json_string = data.dump();
    
    RedisChannel& redis_channel = RedisChannel::getInstance();
    bool success = redis_channel.publish(channel, json_string);
    
    if (success) {
      this->log(logging::LogLevel::INFO, "Successfully published to channel: " + channel);
    } else {
      this->log(logging::LogLevel::ERROR, "Failed to publish to channel: " + channel);
    }
    
    return success;
  } catch (const json::exception& e) {
    this->log(logging::LogLevel::ERROR, "JSON serialization error for channel " + channel + ": " + e.what());
    return false;
  } catch (const std::exception& e) {
    this->log(logging::LogLevel::ERROR, "Error publishing to channel " + channel + ": " + e.what());
    return false;
  }
}

void StateManager::log(logging::LogLevel level,
                       const std::string &message) const {
  this->logger_->log(level, message, "StateManager]");
}

} // namespace StateManager