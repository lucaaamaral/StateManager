#include "client/RedisStorage.h"
#include "client/RedisClient.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

RedisStorage::RedisStorage() : logger_(logging::LoggerFactory::getLogger()) {
  this->log(logging::LogLevel::TRACE, "Initialized RedisStorage");
}

RedisStorage::~RedisStorage() {
  this->log(logging::LogLevel::INFO, "Destroyed RedisStorage");
}

bool RedisStorage::tryLock(const std::string &key) const {
  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    bool acquired = redis.setnx("lock:" + key, "locked");
    if (acquired) {
      this->log(logging::LogLevel::DEBUG, "Lock acquired for key: " + key);
    } else {
      this->log(logging::LogLevel::DEBUG,
                "Failed to acquire lock for key: " + key);
    }
    return acquired;
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error trying to lock key " + key + ": " + e.what());
    return false;
  }
}

void RedisStorage::unlock(const std::string &key) const {
  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    redis.del("lock:" + key);
    this->log(logging::LogLevel::DEBUG, "Lock released for key: " + key);
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error releasing lock for key " + key + ": " + e.what());
  }
}

bool RedisStorage::isLocked(const std::string &key) const {
  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    return redis.exists("lock:" + key);
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error checking lock status for key " + key + ": " + e.what());
    return false;
  }
}

bool RedisStorage::write(const std::string &key,
                         const std::string &value) const {
  if (!this->tryLock(key)) {
    this->log(logging::LogLevel::DEBUG,
              "Cannot write to key " + key + ": Lock not acquired");
    return false;
  }
  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    redis.set(key, value);
    this->log(logging::LogLevel::INFO,
              "Successfully wrote value to key: " + key);
    this->unlock(key);
    return true;
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error writing to key " + key + ": " + e.what());
    this->unlock(key);
    return false;
  }
}

std::string RedisStorage::read(const std::string &key) const {
  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    std::optional<std::string> value = redis.get(key);
    if (value) {
      this->log(logging::LogLevel::INFO,
                "Successfully read value from key: " + key);
      return *value;
    } else {
      this->log(logging::LogLevel::WARNING,
                "Key " + key + " does not exist for read");
      return "";
    }
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error reading from key " + key + ": " + e.what());
    return "";
  }
}

bool RedisStorage::erase(const std::string &key) const {
  if (!this->tryLock(key)) {
    this->log(logging::LogLevel::DEBUG,
              "Cannot erase key " + key + ": Lock not acquired");
    return false;
  }

  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    long deleted = redis.del(key);
    this->unlock(key);

    if (deleted > 0) {
      this->log(logging::LogLevel::INFO, "Successfully erased key: " + key);
      return true;
    } else {
      this->log(logging::LogLevel::WARNING,
                "Key " + key + " does not exist for erase");
      return false;
    }
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error erasing key " + key + ": " + e.what());
    this->unlock(key);
    return false;
  }
}

bool RedisStorage::update(const std::string &key,
                          const std::string &newValue) const {
  if (!this->tryLock(key)) {
    this->log(logging::LogLevel::DEBUG,
              "Cannot update key " + key + ": Lock not acquired");
    return false;
  }

  sw::redis::Redis &redis = RedisClient::getClient();
  try {
    redis.set(key, newValue);
    this->log(logging::LogLevel::INFO, "Successfully updated key: " + key);
    this->unlock(key);
    return true;
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error updating key " + key + ": " + e.what());
    this->unlock(key);
    return false;
  }
}

void RedisStorage::log(logging::LogLevel level,
                       const std::string &message) const {
  this->logger_->log(level, message, "RedisStorage");
}

} // namespace StateManager