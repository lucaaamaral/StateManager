#include "client/RedisChannel.h"
#include "client/RedisClient.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

std::shared_ptr<RedisChannel> RedisChannel::instance_ = nullptr;
std::mutex RedisChannel::instance_mutex_;

RedisChannel::RedisChannel()
    : logger_(logging::LoggerFactory::getLogger()), running_(false) {
  this->pool_.setScaleUp([](uint queueSize, uint workerCount) {
    return (queueSize > UP_THRESHOLD * workerCount &&
            workerCount < RedisChannel::MAX_WORKER_COUNT);
  });
  this->pool_.setScaleDown([](uint queueSize, uint workerCount) {
    return (queueSize < DOWN_THRESHOLD * workerCount && workerCount > 1);
  });
  this->log(logging::LogLevel::TRACE, "Initialized RedisChannel");
}

RedisChannel::~RedisChannel() {
  this->unsubscribe();
  this->log(logging::LogLevel::TRACE, "Destroyed RedisChannel");
  
  if (this->subscriptionThread_ && this->subscriptionThread_->joinable()) {
    this->subscriptionThread_->join();
    this->subscriptionThread_.reset();
  }
}

RedisChannel &RedisChannel::getInstance() {
  std::lock_guard<std::mutex> lock(instance_mutex_);
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<RedisChannel>(new RedisChannel());
  }
  return *instance_;
}

void RedisChannel::subscribe(
    const std::string &channel,
    std::function<void(const std::string &)> callback) {
  this->log(logging::LogLevel::TRACE, "Subscribing to channel: " + channel);
  std::unique_lock<std::mutex> lock(this->callbacks_mutex_);
  this->callbacks_[channel] = callback;
  lock.unlock();

  // Atomic check-and-set to prevent double initialization
  bool expected = false;
  if (!this->running_.load() && this->running_.compare_exchange_strong(expected, true)) {
    this->log(logging::LogLevel::TRACE,
              "Calling initialize subscriber: " + channel);
    if (!initializeSubscriber()) {
      // Initialization failed, reset running flag
      this->running_.store(false);
      this->log(logging::LogLevel::ERROR, "Failed to initialize subscriber for channel: " + channel);
      return;
    }
  } else if (this->running_.load()) {
    try {
      std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
      this->subscriber_->subscribe(channel);
      redisLock.unlock();
      this->log(logging::LogLevel::INFO, "Subscribed to channel: " + channel);
    } catch (const std::exception &e) {
      this->log(logging::LogLevel::ERROR,
                "Error subscribing to channel " + channel + ": " + e.what());
       this->running_.store(false);
    }
  }
  this->log(logging::LogLevel::DEBUG,
            "Added subscription to channel: " + channel);
}

void RedisChannel::unsubscribe(const std::string &channel) {
  if (channel.empty() && this->running_.load()) { // Unsubscribe from all channels
    try {
      this->running_.store(false);
      std::unique_lock<std::mutex> lock(this->callbacks_mutex_);
      for (const auto &pair : this->callbacks_) {
        std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
        this->subscriber_->unsubscribe(pair.first);
        redisLock.unlock();
        this->log(logging::LogLevel::INFO,
                  "Unsubscribed from channel: " + pair.first);
      }
      lock.unlock();
    } catch (const std::exception &e) {
      this->log(logging::LogLevel::ERROR,
                "Error unsubscribing: " + std::string(e.what()));
    }
    this->callbacks_.clear();
    this->log(logging::LogLevel::INFO, "Unsubscribed from all channels");
  } else { // Unsubscribe from specific channel
    auto it = this->callbacks_.find(channel);
    if (it != this->callbacks_.end()) {
      if (this->running_.load()) {
        std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
        this->subscriber_->unsubscribe(channel);
        redisLock.unlock();
        this->log(logging::LogLevel::INFO,
                  "Unsubscribed from channel: " + channel);
      }
      this->callbacks_.erase(it);
      this->log(logging::LogLevel::INFO,
                "Removed subscription from channel: " + channel);
    } else {
      this->log(logging::LogLevel::WARNING,
                "Subscription channel not found: " + channel);
    }
    if (this->callbacks_.empty()) {
      this->running_.store(false);
      this->log(logging::LogLevel::INFO,
                "Unsubscribed from channel (no more subscriptions): " +
                    channel);
    }
  }
}

bool RedisChannel::publish(const std::string &channel,
                           const std::string &message) {
  try {
    sw::redis::Redis &redis = RedisClient::getClient();
    std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
    long long result = redis.publish(channel, message);
    redisLock.unlock();
    this->log(logging::LogLevel::DEBUG,
            "Published message to channel " + channel +
                ", subscribers: " + std::to_string(result));
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Error publishing to channel " + channel + ": " + e.what());
    return false;
  }

  return true;
}

void RedisChannel::log(logging::LogLevel level,
                       const std::string &message) const {
  logger_->log(level, message, "RedisChannel");
}

bool RedisChannel::initializeSubscriber() {
  // Initialize this->subscriber_
  sw::redis::Redis &client = RedisClient::getClient();
  try {
    std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
    this->subscriber_ =
        std::make_unique<sw::redis::Subscriber>(client.subscriber());
    redisLock.unlock();
  } catch (const std::exception &e) {
    this->log(logging::LogLevel::ERROR,
              "Exception in subscriber creation: " + std::string(e.what()));
    return false;
  }

  // Register the on_method
  try{
    std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
    this->subscriber_->on_message([this](std::string chan, std::string msg) {
      std::unique_lock<std::mutex> lock(this->callbacks_mutex_);
      auto it = this->callbacks_.find(chan);
      if (it == this->callbacks_.end())
        return;
      this->pool_.addTask(it->second, msg);
    });
    redisLock.unlock();
  } catch (const std::exception &e) {
    this->subscriber_.reset();
    this->log(logging::LogLevel::ERROR,
              "Exception while registering on_message: " + std::string(e.what()));
    return false;
  }

  std::unique_lock<std::mutex> lock(this->callbacks_mutex_);
  for (const auto &pair : this->callbacks_) {
    std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
    this->subscriber_->subscribe(pair.first);
    redisLock.unlock();
    this->log(logging::LogLevel::DEBUG, "Subscribed to channel: " + pair.first);
  }
  lock.unlock();
  
  this->startConsumeThread();
  return true;
}

void RedisChannel::startConsumeThread() {
  if (this->subscriptionThread_ && this->subscriptionThread_->joinable()) {
    this->log(logging::LogLevel::DEBUG, "Stopping thread to consume redis messages.");
    this->subscriptionThread_->join();
    this->subscriptionThread_.reset();
  }
  // Start thread to consume redis messages
  this->log(logging::LogLevel::DEBUG, "Starting thread to consume redis messages.");
  this->running_.store(true);
  this->subscriptionThread_ = std::make_unique<std::thread>([this]() {
    while (this->running_.load()) {
      try {
        std::unique_lock<std::mutex> redisLock = RedisClient::lockClient();
        this->subscriber_->consume();
        redisLock.unlock();
      } catch (const sw::redis::TimeoutError &te) {
        this->log(logging::LogLevel::TRACE,
                  "[expected] Timeout waiting for messages: " +
                      std::string(te.what()));
      } catch (const sw::redis::Error &e) {
        this->log(logging::LogLevel::WARNING,
                  "Redis connection reset: " + std::string(e.what()));
        this->running_.store(false);
        // Consider reinitializing subscriber on connection error
        // perhaps implement a retry mechanism
        return;
      }
    }
  });
}

} // namespace StateManager
