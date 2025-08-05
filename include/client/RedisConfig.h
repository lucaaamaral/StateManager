#pragma once

#include <string>
#include <sw/redis++/redis++.h>

namespace StateManager {

// Configuration structure for Redis connection
struct RedisConfig {
    std::string host = "localhost";
    int port = 6379;
    std::string password = "";
    int database = 0;
    int timeout_ms = 1000;
    
    RedisConfig() = default;
    RedisConfig(const std::string& host, int port) : host(host), port(port) {}
};

// Utility function to convert RedisConfig to ConnectionOptions
inline sw::redis::ConnectionOptions toConnectionOptions(const RedisConfig& config) {
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

} // namespace StateManager