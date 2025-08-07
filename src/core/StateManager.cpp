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

bool StateManager::write(const std::string &key, const StateObj &value) {
  try {
    json json_value = value.to_json();
    std::string json_string = json_value.dump();
    
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

std::pair<error, std::unique_ptr<StateObj>> StateManager::read(const std::string &key, const StateObj& template_obj) {
  try {
    RedisStorage storage;
    std::string json_string = storage.read(key);
    
    if (json_string.empty()) {
      this->log(logging::LogLevel::WARNING, "Key not found or empty: " + key);
      return std::make_pair(std::make_optional("Key not found"), nullptr);
    }
    
    // Parse JSON string back to object
    json json_value = json::parse(json_string);
    
    // Clone the template object and deserialize into it
    auto result = template_obj.clone();
    result->from_json(json_value);
    
    this->log(logging::LogLevel::INFO, "Successfully read value from key: " + key);
    return std::make_pair(std::nullopt, std::move(result));

  } catch (const json::exception& e) {
    this->log(logging::LogLevel::ERROR, "JSON deserialization error for key " + key + ": " + e.what());
    return std::make_pair(std::make_optional("JSON parsing error"), nullptr);
  } catch (const std::exception& e) {
    this->log(logging::LogLevel::ERROR, "Error reading from key " + key + ": " + e.what());
    return std::make_pair(std::make_optional("Storage error"), nullptr);
  }
}

void StateManager::subscribe(const std::string& channel, std::function<void(std::unique_ptr<StateObj>)> callback, const StateObj& template_obj) {
  try {
    RedisChannel& redis_channel = RedisChannel::getInstance();
    
    // Clone the template object once and capture by value for safety
    auto template_clone = template_obj.clone();
    
    // Wrap the user callback to handle JSON deserialization and StateObj creation
    auto wrapped_callback = [this, callback = std::move(callback), channel, template_ptr = std::shared_ptr<StateObj>(std::move(template_clone))](const std::string& message) {
      json json_value = json::parse(message);
      auto state_obj = template_ptr->clone();
      state_obj->from_json(json_value);
      callback(std::move(state_obj));
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

bool StateManager::publish(const std::string& channel, const StateObj& data) {
  try {
    json json_value = data.to_json();
    std::string json_string = json_value.dump();
    
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