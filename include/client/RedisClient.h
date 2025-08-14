#pragma once

#include <memory>
#include <mutex>

#include <sw/redis++/redis++.h>

#include "client/ClientIface.h"
#include "client/RedisConfig.h"
#include "logging/LoggerIface.h"

namespace StateManager {

class RedisClient : public ClientIface {
private:
  static std::shared_ptr<RedisClient> instance_;
  static std::mutex instance_mutex_;

  std::unique_ptr<sw::redis::Redis> redis_;
  sw::redis::ConnectionOptions config_;
  mutable std::mutex redis_mutex_;
  std::shared_ptr<logging::LoggerIface> logger_;

  RedisClient();
  bool isConnected() const override;
  void connect() override;
  static sw::redis::Redis &getClient();
  static std::unique_lock<std::mutex> lockClient();

public:
  friend class RedisChannel;
  friend class RedisStorage;
  friend class RedisClientTest;

  static void setConfig(const RedisConfig &config);
  ~RedisClient() override;
};

} // namespace StateManager