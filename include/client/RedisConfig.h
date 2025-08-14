#pragma once

#include <string>

namespace StateManager {

// Configuration structure for Redis connection
struct RedisConfig {
  std::string host = "localhost";
  int port = 6379;
  std::string password = "";
  int database = 0;
  int timeout_ms = 1000;

  RedisConfig() = default;
  RedisConfig(const std::string &host, int port) : host(host), port(port) {}
};

} // namespace StateManager