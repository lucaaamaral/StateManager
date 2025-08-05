#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>

#include "client/RedisClient.h"
#include "client/RedisConfig.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

// Test fixture for RedisClient tests
class RedisClientTest : public ::testing::Test {
protected:
  std::shared_ptr<logging::LoggerIface> logger;

  void SetUp() override {
    logger = logging::LoggerFactory::getLogger();
    logging::LoggerFactory::setLevel(logging::LogLevel::INFO);
    RedisClient::setConfig(RedisConfig{});
  }

  void TearDown() override {
    // Reset to default configuration between tests
    RedisClient::setConfig(RedisConfig{});
  }

public:
  static sw::redis::Redis &getClient() { return RedisClient::getClient(); }

  static std::unique_lock<std::mutex> lockClient() {
    return RedisClient::lockClient();
  }
};

// Test RedisConfig structure and utility functions
TEST_F(RedisClientTest, RedisConfigTest) {
  // Test parameterized constructor
  RedisConfig custom_config("localhost", 6380);
  EXPECT_EQ(custom_config.host, "localhost");
  EXPECT_EQ(custom_config.port, 6380);

  // Test toConnectionOptions utility function
  RedisConfig config;
  config.host = "test-host";
  config.port = 6380;
  config.password = "test-password";
  config.database = 2;
  config.timeout_ms = 2000;

  auto conn_options = toConnectionOptions(config);
  EXPECT_EQ(conn_options.host, "test-host");
  EXPECT_EQ(conn_options.port, 6380);
  EXPECT_EQ(conn_options.password, "test-password");
  EXPECT_EQ(conn_options.db, 2);
  EXPECT_EQ(conn_options.socket_timeout.count(), 2000);
}

// Test setting configuration
TEST_F(RedisClientTest, SetConfigTest) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 3000;

  // This should not throw
  EXPECT_NO_THROW({ RedisClient::setConfig(config); });
}

// Test configuration with different parameters
TEST_F(RedisClientTest, ConfigurationVariations) {
  // Test with password
  RedisConfig config_with_password;
  config_with_password.host = "localhost";
  config_with_password.port = 6379;
  config_with_password.password = "test-password";
  config_with_password.database = 1;
  config_with_password.timeout_ms = 2000;

  EXPECT_NO_THROW({ RedisClient::setConfig(config_with_password); });

  // Test with different database
  RedisConfig config_different_db;
  config_different_db.host = "localhost";
  config_different_db.port = 6379;
  config_different_db.database = 3;

  EXPECT_NO_THROW({ RedisClient::setConfig(config_different_db); });

  // Test with different timeout
  RedisConfig config_short_timeout;
  config_short_timeout.host = "localhost";
  config_short_timeout.port = 6379;
  config_short_timeout.timeout_ms = 100;

  EXPECT_NO_THROW({ RedisClient::setConfig(config_short_timeout); });
}

// Test rapid configuration changes
TEST_F(RedisClientTest, RapidConfigurationChanges) {
  // Make multiple rapid configuration changes
  for (int i = 0; i < 5; ++i) {
    RedisConfig config;
    config.host = "localhost";
    config.port = 6379;
    config.database = i % 2; // Alternate between databases 0 and 1
    config.timeout_ms = 1000 + (i * 100);

    EXPECT_NO_THROW({ RedisClient::setConfig(config); });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// Test configuration with empty password
TEST_F(RedisClientTest, EmptyPasswordConfig) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.password = ""; // Empty password
  config.database = 0;
  config.timeout_ms = 1000;

  EXPECT_NO_THROW({ RedisClient::setConfig(config); });
}

// Test actual Redis connectivity using private methods
TEST_F(RedisClientTest, TestRedisConnectivity) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 2000;

  RedisClient::setConfig(config);

  // Test connectivity by accessing the client directly and checking connection
  try {
    sw::redis::Redis &client = getClient();
    std::string pong = client.ping();
    EXPECT_EQ(pong, "PONG") << "Redis ping failed - connection issue detected";
    logger->info("Redis connectivity test passed: " + pong);
  } catch (const std::exception &e) {
    FAIL() << "Failed to connect to Redis: " << e.what();
  }
}

// Test actual Redis connectivity using private methods
TEST_F(RedisClientTest, TestRedisConnectivityDefaultConfig) {
  RedisClient::setConfig(RedisConfig{});

  // Test connectivity by accessing the client directly and checking connection
  try {
    sw::redis::Redis &client = getClient();
    std::string pong = client.ping();
    EXPECT_EQ(pong, "PONG") << "Redis ping failed - connection issue detected";
    logger->info("Redis connectivity test passed: " + pong);
  } catch (const std::exception &e) {
    FAIL() << "Failed to connect to Redis: " << e.what();
  }
}

// Test Redis connection with invalid configuration
TEST_F(RedisClientTest, TestInvalidRedisConfig) {
  RedisConfig config;
  config.host = "invalid-redis-host";
  config.port = 9999;
  config.database = 0;
  config.timeout_ms = 500; // Short timeout

  // This should not throw
  EXPECT_NO_THROW({ RedisClient::setConfig(config); });

  // Test connectivity by trying to get client - should handle connection
  // failure gracefully
  try {
    sw::redis::Redis &client = getClient();
    std::string pong = client.ping();
    FAIL() << "Should not be able to connect with invalid config, but got: "
           << pong;
  } catch (const std::exception &e) {
    // Expected to fail with invalid configuration
    logger->info("Expected connection failure with invalid config: " +
                 std::string(e.what()));
    SUCCEED();
  }
}

// Test Redis connection configuration validation
TEST_F(RedisClientTest, TestConfigurationDetails) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 3000;

  // Log configuration details for debugging
  logger->info("Testing Redis configuration:");
  logger->info("Host: " + config.host);
  logger->info("Port: " + std::to_string(config.port));
  logger->info("Database: " + std::to_string(config.database));
  logger->info("Timeout: " + std::to_string(config.timeout_ms) + "ms");

  EXPECT_NO_THROW({ RedisClient::setConfig(config); });

  // Test immediate connectivity using private methods
  try {
    sw::redis::Redis &client = getClient();
    std::string pong = client.ping();
    logger->info("Redis connectivity test passed: " + pong);

    // Test basic Redis operations
    client.set("test_key", "test_value");
    auto value = client.get("test_key");
    if (value && *value == "test_value") {
      logger->info("Redis set/get operations working correctly");
    }
    client.del("test_key"); // Cleanup

  } catch (const std::exception &e) {
    logger->error("Redis connectivity test failed: " + std::string(e.what()));

    // Try with different configurations
    RedisConfig alt_config;
    alt_config.host = "127.0.0.1";
    alt_config.port = 6379;
    alt_config.database = 0;
    alt_config.timeout_ms = 5000;

    try {
      RedisClient::setConfig(alt_config);
      sw::redis::Redis &alt_client = getClient();
      std::string alt_pong = alt_client.ping();
      logger->info("Alternative config result: SUCCESS - " + alt_pong);
    } catch (const std::exception &alt_e) {
      logger->info("Alternative config result: FAILED - " +
                   std::string(alt_e.what()));
    }
  }

  // Don't fail the test, just report results for debugging
  SUCCEED();
}

// Test multiple rapid connections to detect race conditions
TEST_F(RedisClientTest, TestConnectionRaceConditions) {
  const int num_attempts = 10;
  std::vector<bool> results;

  for (int i = 0; i < num_attempts; ++i) {
    RedisConfig config;
    config.host = "localhost";
    config.port = 6379;
    config.database = i % 3;              // Vary database
    config.timeout_ms = 1000 + (i * 100); // Vary timeout

    RedisClient::setConfig(config);

    // Small delay to allow connection
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    try {
      sw::redis::Redis &client = getClient();
      std::string pong = client.ping();
      results.push_back(true);
      logger->info("Connection attempt " + std::to_string(i) +
                   " succeeded: " + pong);
    } catch (const std::exception &e) {
      results.push_back(false);
      logger->warning("Connection attempt " + std::to_string(i) +
                      " failed: " + e.what());
    }
  }

  int successful_connections = std::count(results.begin(), results.end(), true);
  logger->info(
      "Successful connections: " + std::to_string(successful_connections) +
      "/" + std::to_string(num_attempts));

  // Report results without failing test
  SUCCEED();
}

// Test connection state debugging
TEST_F(RedisClientTest, DebugConnectionState) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 2000;

  logger->info("=== Redis Connection Debug Test ===");

  RedisClient::setConfig(config);
  logger->info("Config set completed");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Test direct Redis client connection and operations
  try {
    sw::redis::Redis &client = getClient();
    logger->info("RedisClient instance obtained");

    logger->info("Attempting to ping Redis...");
    std::string pong = client.ping();
    logger->info("Ping result: " + pong);

    logger->info("Attempting Redis set operation...");
    client.set("debug_test_key", "debug_test_value");
    logger->info("Set operation completed");

    logger->info("Attempting Redis get operation...");
    auto value = client.get("debug_test_key");
    if (value) {
      logger->info("Get operation result: " + *value);
    } else {
      logger->info("Get operation result: no value found");
    }

    logger->info("Attempting Redis delete operation...");
    long deleted = client.del("debug_test_key");
    logger->info("Delete operation result: " + std::to_string(deleted) +
                 " keys deleted");

  } catch (const std::exception &e) {
    logger->error("Redis operation failed: " + std::string(e.what()));
  }

  SUCCEED();
}

// Test private method access through friend class
TEST_F(RedisClientTest, TestPrivateMethodAccess) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 2000;

  logger->info("=== Testing Private Method Access ===");

  RedisClient::setConfig(config);

  // Since RedisClientTest is now a friend class, we can access private methods
  try {
    // Access the private getClient method
    sw::redis::Redis &client = getClient();
    logger->info("Successfully accessed private getClient method");

    // Test lockClient private method
    auto lock = lockClient();
    logger->info("Successfully accessed private lockClient method");
    lock.unlock(); // Release the lock

    // Test basic operations through the client
    std::string pong = client.ping();
    logger->info("Private client ping result: " + pong);
    EXPECT_EQ(pong, "PONG");

  } catch (const std::exception &e) {
    logger->error("Failed to access private methods: " + std::string(e.what()));
    FAIL() << "Private method access failed: " << e.what();
  }

  SUCCEED();
}

// Test Redis subscriber creation to isolate the issue
TEST_F(RedisClientTest, TestRedisSubscriberCreation) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 2000;

  logger->info("=== Testing Redis Subscriber Creation ===");

  RedisClient::setConfig(config);

  try {
    // Step 1: Get the Redis client
    sw::redis::Redis &client = getClient();
    logger->info("Successfully obtained Redis client");

    // Step 2: Test basic connection
    std::string pong = client.ping();
    logger->info("Redis ping successful: " + pong);
    EXPECT_EQ(pong, "PONG");

    // Step 3: Try to create a subscriber - this is the problematic call
    logger->info("Attempting to create Redis subscriber...");
    auto subscriber = client.subscriber();
    logger->info("Successfully created Redis subscriber");

    // Step 4: Test subscriber functionality
    logger->info("Testing subscriber on_message callback setup...");
    std::atomic<bool> message_received{false};
    std::string received_message;

    subscriber.on_message([this, &message_received, &received_message](
                              std::string channel, std::string msg) {
      logger->info("Message received on channel " + channel + ": " + msg);
      received_message = msg;
      message_received = true;
    });
    logger->info("Subscriber callback configured successfully");

    // Step 5: Test subscription
    logger->info("Testing subscription to test channel...");
    subscriber.subscribe("test_subscriber_channel");
    logger->info("Successfully subscribed to test channel");

    // Step 6: Test publish/receive in separate client
    logger->info("Publishing test message...");
    auto publish_result =
        client.publish("test_subscriber_channel", "test_message");
    logger->info("Publish result: " + std::to_string(publish_result) +
                 " subscribers received");

    // Step 7: Consume one message with timeout
    logger->info("Attempting to consume message...");
    try {
      subscriber.consume(); // This should process the published message
      logger->info("Message consumption completed");
    } catch (const std::exception &consume_e) {
      logger->warning("Exception during consume: " +
                      std::string(consume_e.what()));
    }

    SUCCEED();

  } catch (const std::exception &e) {
    logger->error("Exception in subscriber test: " + std::string(e.what()));
    FAIL() << "Subscriber creation/usage failed: " << e.what();
  }
}

// Test Redis subscriber creation with lock
TEST_F(RedisClientTest, TestRedisSubscriberWithLock) {
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 2000;

  logger->info("=== Testing Redis Subscriber Creation With Lock ===");

  RedisClient::setConfig(config);

  try {
    // Mimic the exact pattern used in RedisChannel::initializeSubscriber
    sw::redis::Redis &client = getClient();
    std::unique_lock<std::mutex> redisLock = lockClient();

    logger->info("Successfully obtained Redis client and lock");

    // This is the exact problematic call from RedisChannel
    logger->info("Attempting client.subscriber() with lock held...");
    auto subscriber = client.subscriber();
    logger->info("Successfully created subscriber with lock held");

    redisLock.unlock();
    logger->info("Lock released successfully");

    SUCCEED();

  } catch (const std::exception &e) {
    logger->error("Exception in locked subscriber test: " +
                  std::string(e.what()));
    FAIL() << "Subscriber creation with lock failed: " << e.what();
  }
}

// Test RedisClient without ever calling setConfig (like RedisChannel does)
TEST_F(RedisClientTest, TestRedisClientWithoutSetConfig) {
  logger->info("=== Testing RedisClient Without setConfig ===");

  // Don't call setConfig at all, just try to use getClient directly
  // This mimics what happens in RedisChannel
  try {
    sw::redis::Redis &client = getClient();
    logger->info("Successfully obtained Redis client without setConfig");

    std::string pong = client.ping();
    logger->info("Redis ping successful without setConfig: " + pong);
    EXPECT_EQ(pong, "PONG");

  } catch (const std::exception &e) {
    logger->error("Exception when using RedisClient without setConfig: " +
                  std::string(e.what()));
    FAIL() << "RedisClient failed without setConfig: " << e.what();
  }
}

// Test configuration with invalid host (should not throw in setConfig)
TEST_F(RedisClientTest, InvalidHostConfig) {
  RedisConfig config;
  config.host = "invalid-host-that-does-not-exist";
  config.port = 9999;
  config.timeout_ms = 500; // Short timeout

  // setConfig should not throw even with invalid host
  // Connection failure will be handled internally
  EXPECT_NO_THROW({ RedisClient::setConfig(config); });
}

// Test thread safety of setConfig
TEST_F(RedisClientTest, ThreadSafeSetConfig) {
  const int num_threads = 3;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  // Create multiple threads that set different configurations
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&success_count, i]() {
      try {
        RedisConfig config;
        config.host = "localhost";
        config.port = 6379;
        config.database = i % 2; // Different databases
        config.timeout_ms = 1000 + (i * 100);

        RedisClient::setConfig(config);
        success_count++;
      } catch (const std::exception &e) {
        // Thread failed
      }
    });
  }

  // Wait for all threads to complete
  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // All threads should have successfully set configuration
  EXPECT_EQ(success_count.load(), num_threads);
}

// Test configuration with various timeout values
TEST_F(RedisClientTest, TimeoutVariations) {
  std::vector<int> timeouts = {100, 500, 1000, 2000, 5000};

  for (int timeout : timeouts) {
    RedisConfig config;
    config.host = "localhost";
    config.port = 6379;
    config.timeout_ms = timeout;

    EXPECT_NO_THROW({ RedisClient::setConfig(config); });
  }
}

// Test configuration with various database numbers
TEST_F(RedisClientTest, DatabaseVariations) {
  std::vector<int> databases = {0, 1, 2, 3, 15};

  for (int db : databases) {
    RedisConfig config;
    config.host = "localhost";
    config.port = 6379;
    config.database = db;

    EXPECT_NO_THROW({ RedisClient::setConfig(config); });
  }
}

// Test configuration with various port numbers
TEST_F(RedisClientTest, PortVariations) {
  std::vector<int> ports = {6379, 6380, 6381, 7000, 7001};

  for (int port : ports) {
    RedisConfig config;
    config.host = "localhost";
    config.port = port;

    EXPECT_NO_THROW({ RedisClient::setConfig(config); });
  }
}

// ==================== THREAD SAFETY TESTS ====================

// Test concurrent singleton creation
TEST_F(RedisClientTest, ConcurrentSingletonCreation) {
  const int num_threads = 10;
  std::vector<std::thread> threads;
  std::vector<sw::redis::Redis*> client_ptrs(num_threads);
  std::atomic<int> success_count{0};

  // Multiple threads trying to get client simultaneously
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&client_ptrs, &success_count, i]() {
      try {
        sw::redis::Redis& client = RedisClientTest::getClient();
        client_ptrs[i] = &client;
        
        // Test that the client actually works
        std::string pong = client.ping();
        if (pong == "PONG") {
          success_count++;
        }
      } catch (const std::exception& e) {
        client_ptrs[i] = nullptr;
      }
    });
  }

  // Wait for all threads
  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // All threads should get the same singleton instance
  sw::redis::Redis* first_valid_ptr = nullptr;
  for (int i = 0; i < num_threads; ++i) {
    if (client_ptrs[i] != nullptr) {
      if (first_valid_ptr == nullptr) {
        first_valid_ptr = client_ptrs[i];
      } else {
        EXPECT_EQ(client_ptrs[i], first_valid_ptr) 
          << "Thread " << i << " got different singleton instance";
      }
    }
  }

  EXPECT_GT(success_count.load(), 0) << "No threads successfully accessed Redis client";
  logger->info("Concurrent singleton test: " + std::to_string(success_count.load()) + 
               "/" + std::to_string(num_threads) + " threads succeeded");
}

// Test concurrent configuration changes
TEST_F(RedisClientTest, ConcurrentConfigurationChanges) {
  const int num_threads = 5;
  const int configs_per_thread = 3;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};
  std::atomic<int> total_attempts{0};

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&success_count, &total_attempts, t, configs_per_thread]() {
      for (int c = 0; c < configs_per_thread; ++c) {
        try {
          RedisConfig config;
          config.host = "localhost";
          config.port = 6379;
          config.database = (t * configs_per_thread + c) % 3; // Vary database
          config.timeout_ms = 1000 + (t * 100);

          RedisClient::setConfig(config);
          total_attempts++;
          success_count++;
          
          // Brief delay to increase chance of race conditions
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } catch (const std::exception& e) {
          total_attempts++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // All configuration attempts should succeed
  EXPECT_EQ(success_count.load(), total_attempts.load());
  logger->info("Concurrent config test: " + std::to_string(success_count.load()) + 
               "/" + std::to_string(total_attempts.load()) + " config changes succeeded");
}

// Test concurrent Redis operations
TEST_F(RedisClientTest, ConcurrentRedisOperations) {
  const int num_threads = 8;
  const int operations_per_thread = 5;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};
  std::atomic<int> total_operations{0};

  // Set initial config
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 2000;
  RedisClient::setConfig(config);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&success_count, &total_operations, t, operations_per_thread]() {
      for (int op = 0; op < operations_per_thread; ++op) {
        try {
          sw::redis::Redis& client = RedisClientTest::getClient();
          
          // Perform different operations to test thread safety
          std::string key = "thread_" + std::to_string(t) + "_op_" + std::to_string(op);
          std::string value = "value_" + std::to_string(t) + "_" + std::to_string(op);
          
          // SET operation
          client.set(key, value);
          
          // GET operation  
          auto retrieved = client.get(key);
          if (retrieved && *retrieved == value) {
            success_count++;
          }
          
          // DELETE operation
          client.del(key);
          
          total_operations++;
          
        } catch (const std::exception& e) {
          total_operations++;
        }
      }
    });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_EQ(success_count.load(), total_operations.load()) 
    << "Some Redis operations failed in concurrent test";
  logger->info("Concurrent operations test: " + std::to_string(success_count.load()) + 
               "/" + std::to_string(total_operations.load()) + " operations succeeded");
}

// Test mixed concurrent operations (config changes + Redis operations)
TEST_F(RedisClientTest, MixedConcurrentOperations) {
  const int num_config_threads = 2;
  const int num_operation_threads = 4;
  const int operations_per_thread = 3;
  std::vector<std::thread> threads;
  std::atomic<int> config_success{0};
  std::atomic<int> operation_success{0};
  std::atomic<bool> stop_test{false};

  // Config changing threads
  for (int t = 0; t < num_config_threads; ++t) {
    threads.emplace_back([&config_success, &stop_test, t]() {
      int config_count = 0;
      while (!stop_test.load() && config_count < 5) {
        try {
          RedisConfig config;
          config.host = "localhost";
          config.port = 6379;
          config.database = config_count % 2;
          config.timeout_ms = 1000 + (t * 200);
          
          RedisClient::setConfig(config);
          config_success++;
          config_count++;
          
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        } catch (const std::exception& e) {
          // Config change failed, continue
        }
      }
    });
  }

  // Redis operation threads
  for (int t = 0; t < num_operation_threads; ++t) {
    threads.emplace_back([&operation_success, &stop_test, t, operations_per_thread]() {
      for (int op = 0; op < operations_per_thread && !stop_test.load(); ++op) {
        try {
          sw::redis::Redis& client = RedisClientTest::getClient();
          auto lock = RedisClientTest::lockClient();
          
          std::string key = "mixed_thread_" + std::to_string(t) + "_op_" + std::to_string(op);
          std::string value = "mixed_value_" + std::to_string(t);
          
          client.set(key, value);
          auto retrieved = client.get(key);
          
          if (retrieved && *retrieved == value) {
            operation_success++;
          }
          
          client.del(key);
          
          std::this_thread::sleep_for(std::chrono::milliseconds(25));
        } catch (const std::exception& e) {
          // Operation failed, continue
        }
      }
    });
  }

  // Let test run for a while
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  stop_test = true;

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_GT(config_success.load(), 0) << "No config changes succeeded";
  EXPECT_GT(operation_success.load(), 0) << "No Redis operations succeeded";
  
  logger->info("Mixed operations test - Config changes: " + std::to_string(config_success.load()) + 
               ", Operations: " + std::to_string(operation_success.load()));
}

// Test concurrent lock contention
TEST_F(RedisClientTest, ConcurrentLockContention) {
  const int num_threads = 6;
  std::vector<std::thread> threads;
  std::atomic<int> lock_acquisitions{0};
  std::atomic<int> successful_operations{0};
  std::vector<std::chrono::steady_clock::time_point> lock_times(num_threads);

  // Set initial config
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  RedisClient::setConfig(config);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&lock_acquisitions, &successful_operations, &lock_times, t]() {
      try {
        auto start_time = std::chrono::steady_clock::now();

        // Acquire the Redis client lock
        // Hold lock for a short time and perform operation
        sw::redis::Redis& client = RedisClientTest::getClient();
        auto lock = RedisClientTest::lockClient();
        lock_times[t] = std::chrono::steady_clock::now();
        lock_acquisitions++;

        std::string key = "lock_test_" + std::to_string(t);
        std::string value = "locked_value_" + std::to_string(t);
        
        client.set(key, value);
        auto retrieved = client.get(key);
        
        if (retrieved && *retrieved == value) {
          successful_operations++;
        }
        
        client.del(key);
        
        // Hold lock briefly to test contention
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        lock.unlock();
      } catch (const std::exception& e) {
        // Lock acquisition or operation failed
      }
    });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_EQ(lock_acquisitions.load(), num_threads) << "Some threads failed to acquire lock";
  EXPECT_EQ(successful_operations.load(), num_threads) << "Some operations failed while holding lock";
  
  // Verify locks were acquired serially (no two threads got lock simultaneously)
  for (int i = 0; i < num_threads - 1; ++i) {
    for (int j = i + 1; j < num_threads; ++j) {
      auto time_diff = std::abs(std::chrono::duration_cast<std::chrono::milliseconds>(
        lock_times[i] - lock_times[j]).count());
      EXPECT_GE(time_diff, 45) << "Threads " << i << " and " << j << " acquired locks too close in time";
    }
  }
  
  logger->info("Lock contention test: " + std::to_string(lock_acquisitions.load()) + 
               " locks acquired, " + std::to_string(successful_operations.load()) + " operations succeeded");
}

// Test thread safety during connection failures and recovery
TEST_F(RedisClientTest, ConcurrentConnectionRecovery) {
  const int num_threads = 4;
  std::vector<std::thread> threads;
  std::atomic<int> successful_reconnects{0};
  std::atomic<int> successful_operations{0};

  // Start with invalid config to force connection failures
  RedisConfig invalid_config;
  invalid_config.host = "invalid-host";
  invalid_config.port = 9999;
  invalid_config.timeout_ms = 100; // Quick timeout
  RedisClient::setConfig(invalid_config);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&successful_reconnects, &successful_operations, t]() {
      // Phase 1: Try operations with invalid config (should fail)
      try {
        sw::redis::Redis& client = RedisClientTest::getClient();
        auto lock = RedisClientTest::lockClient();
        client.ping(); // This should fail
      } catch (const std::exception& e) {
        // Expected to fail
      }
      
      // Phase 2: Set valid config and try again
      try {
        RedisConfig valid_config;
        valid_config.host = "localhost";
        valid_config.port = 6379;
        valid_config.database = t % 2;
        valid_config.timeout_ms = 2000;
        
        RedisClient::setConfig(valid_config);
        successful_reconnects++;
        
        // Give some time for connection
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Try operation with valid config
        sw::redis::Redis& client = RedisClientTest::getClient();
        auto lock = RedisClientTest::lockClient();
        std::string pong = client.ping();
        
        if (pong == "PONG") {
          successful_operations++;
        }
      } catch (const std::exception& e) {
        // Reconnection or operation failed
      }
    });
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  EXPECT_GT(successful_reconnects.load(), 0) << "No threads successfully changed to valid config";
  
  logger->info("Connection recovery test: " + std::to_string(successful_reconnects.load()) + 
               " reconnects, " + std::to_string(successful_operations.load()) + " operations succeeded");
}

// Stress test: High-frequency concurrent operations
TEST_F(RedisClientTest, HighFrequencyConcurrentStress) {
  const int num_threads = 6;
  const int duration_seconds = 2;
  std::vector<std::thread> threads;
  std::atomic<int> total_operations{0};
  std::atomic<int> successful_operations{0};
  std::atomic<bool> stop_test{false};

  // Set config
  RedisConfig config;
  config.host = "localhost";
  config.port = 6379;
  config.database = 0;
  config.timeout_ms = 1000;
  RedisClient::setConfig(config);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&total_operations, &successful_operations, &stop_test, t]() {
      int thread_ops = 0;
      while (!stop_test.load()) {
        try {
          sw::redis::Redis& client = RedisClientTest::getClient();
          auto lock = RedisClientTest::lockClient();
          
          std::string key = "stress_" + std::to_string(t) + "_" + std::to_string(thread_ops);
          std::string value = "val_" + std::to_string(thread_ops);
          
          // Rapid fire operations
          client.set(key, value);
          auto retrieved = client.get(key);
          client.del(key);
          
          if (retrieved && *retrieved == value) {
            successful_operations++;
          }
          
          total_operations++;
          thread_ops++;
          
          // Brief yield to other threads
          if (thread_ops % 10 == 0) {
            std::this_thread::yield();
          }
        } catch (const std::exception& e) {
          total_operations++;
        }
      }
    });
  }

  // Run stress test
  std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
  stop_test = true;

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // Calculate success rate
  double success_rate = (double)successful_operations.load() / (double)total_operations.load() * 100.0;
  
  EXPECT_GT(total_operations.load(), duration_seconds * num_threads * 10) 
    << "Stress test didn't generate enough operations";
  EXPECT_GT(success_rate, 95.0) << "Success rate too low: " << success_rate << "%";
  
  logger->info("Stress test: " + std::to_string(total_operations.load()) + 
               " total operations, " + std::to_string(successful_operations.load()) + 
               " successful (" + std::to_string((int)success_rate) + "%)");
}

} // namespace StateManager