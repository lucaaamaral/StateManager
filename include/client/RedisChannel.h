#include <atomic>
#ifndef REDIS_CHANNEL_H
#define REDIS_CHANNEL_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <sw/redis++/redis++.h>
#include <thread>
#include <unordered_map>
#include <vector>

#include "logging/LoggerIface.h"
#include "thread/ThreadPool.h"

namespace StateManager {

class RedisChannel {
private:
  static std::shared_ptr<RedisChannel> instance_;
  static std::mutex instance_mutex_;

  std::shared_ptr<logging::LoggerIface> logger_;
  ThreadPool pool_;

  std::unordered_map<std::string, std::function<void(const std::string &)>>
      callbacks_;
  std::unique_ptr<std::thread> subscriptionThread_;
  std::atomic<bool> running_;
  std::mutex callbacks_mutex_;
  std::unique_ptr<sw::redis::Subscriber> subscriber_;

  RedisChannel();
  void log(logging::LogLevel level, const std::string &message) const;
  bool
  initializeSubscriber(); // Method to initialize subscriber and consume thread
  void startConsumeThread();

public:
  ~RedisChannel();

  static constexpr int MAX_WORKER_COUNT = 5;
  static constexpr int UP_THRESHOLD = 10;
  static constexpr int DOWN_THRESHOLD = 3;

  static RedisChannel &getInstance();
  void subscribe(const std::string &channel,
                 std::function<void(const std::string &)> callback);
  void unsubscribe(const std::string &channel = "");
  bool publish(const std::string &channel, const std::string &message);
};

} // namespace StateManager

#endif // REDIS_CHANNEL_H