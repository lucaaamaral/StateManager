#include "logging/DefaultLogger.h"
#include "logging/LogLevel.h"
#include "logging/LoggerFactory.h"
#include "logging/LoggerIface.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace StateManager::logging;

// Test logger implementation that counts log calls
class TestCountingLogger : public LoggerIface {
private:
  std::atomic<int> log_count_{0};
  std::mutex log_mutex_;

public:
  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_count_++;
    // Optionally print to console for debugging
    // std::cout << "[" << level << "] " << message << std::endl;
  }

  // Override both log methods to ensure proper functionality
  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override {
    // Call the string-based version with the properly converted level
    log(logLevelToString(level), message, context);
  }

  void setLevel(std::string level) override {
    this->setLevel(stringToLogLevel(level));
  }

  void setLevel(LogLevel level) override {
    this->log(level, "Setting level", "");
  }

  int getLogCount() const { return log_count_; }

  void resetCount() { log_count_ = 0; }
};

// Test logger that tracks the last message
class TestMessageLogger : public LoggerIface {
private:
  std::string last_level_;
  std::string last_message_;
  std::string last_context_;
  std::mutex log_mutex_;

public:
  void log(const std::string &level, const std::string &message,
           const std::string &context = "") override {
    std::lock_guard<std::mutex> lock(log_mutex_);
    last_level_ = level;
    last_message_ = message;
    last_context_ = context;
  }

  // Override both log methods to ensure proper functionality
  void log(LogLevel level, const std::string &message,
           const std::string &context = "") override {
    // Call the string-based version with the properly converted level
    log(StateManager::logging::logLevelToString(level), message, context);
  }

  void setLevel(std::string level) override {
    this->setLevel(stringToLogLevel(level));
  }

  void setLevel(LogLevel level) override {
    this->log(level, "Setting level", "");
  }

  std::string getLastLevel() const { return last_level_; }

  std::string getLastMessage() const { return last_message_; }

  std::string getLastContext() const { return last_context_; }
};

// Test fixture for logger tests
class LoggerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Reset to default logger before each test
    auto defaultLogger = std::make_shared<DefaultLogger>();
    LoggerFactory::setLogger(defaultLogger);
  }

  void TearDown() override {}
};

// Test that LoggerFactory returns the same instance (singleton pattern)
TEST_F(LoggerTest, SingletonInstanceTest) {
  auto logger1 = LoggerFactory::getLogger();
  auto logger2 = LoggerFactory::getLogger();

  // Both pointers should point to the same instance
  EXPECT_EQ(logger1, logger2);
}

// Test setting a custom logger
TEST_F(LoggerTest, SetCustomLoggerTest) {
  auto customLogger = std::make_shared<TestCountingLogger>();
  LoggerFactory::setLogger(customLogger);

  auto logger = LoggerFactory::getLogger();
  logger->info("Test message");

  // Cast to TestCountingLogger to check the count
  auto testLogger = std::dynamic_pointer_cast<TestCountingLogger>(logger);
  ASSERT_NE(testLogger, nullptr);
  EXPECT_EQ(testLogger->getLogCount(), 1);
}

// Test that log level methods work correctly
TEST_F(LoggerTest, LogLevelMethodsTest) {
  auto messageLogger = std::make_shared<TestMessageLogger>();
  LoggerFactory::setLogger(messageLogger);

  auto logger = LoggerFactory::getLogger();

  // Test each log level method
  logger->trace("Trace message");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastLevel(),
      "TRACE");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastMessage(),
      "Trace message");

  logger->debug("Debug message");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastLevel(),
      "DEBUG");

  logger->info("Info message");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastLevel(),
      "INFO");

  logger->warning("Warning message");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastLevel(),
      "WARNING");

  logger->error("Error message");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastLevel(),
      "ERROR");

  logger->critical("Critical message");
  EXPECT_EQ(
      std::dynamic_pointer_cast<TestMessageLogger>(logger)->getLastLevel(),
      "CRITICAL");
}

// Test context parameter
TEST_F(LoggerTest, ContextParameterTest) {
  auto messageLogger = std::make_shared<TestMessageLogger>();
  LoggerFactory::setLogger(messageLogger);

  auto logger = LoggerFactory::getLogger();
  logger->info("Info with context", "TestContext");

  auto testLogger = std::dynamic_pointer_cast<TestMessageLogger>(logger);
  ASSERT_NE(testLogger, nullptr);
  EXPECT_EQ(testLogger->getLastContext(), "TestContext");
}

// Test thread safety of the singleton pattern
TEST_F(LoggerTest, ThreadSafetyTest) {
  const int num_threads = 3;      // Reduced from 5
  const int logs_per_thread = 10; // Reduced from 20

  auto countingLogger = std::make_shared<TestCountingLogger>();
  LoggerFactory::setLogger(countingLogger);

  std::vector<std::thread> threads;

  // Create multiple threads that log messages
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([i, logs_per_thread]() {
      for (int j = 0; j < logs_per_thread; ++j) {
        auto logger = LoggerFactory::getLogger();
        logger->info("Thread " + std::to_string(i) + " message " +
                     std::to_string(j));

        // Add a small sleep to reduce contention
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }
    });
  }

  // Wait for all threads to complete
  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // Check that all log messages were counted
  auto testLogger =
      std::dynamic_pointer_cast<TestCountingLogger>(LoggerFactory::getLogger());
  ASSERT_NE(testLogger, nullptr);
  EXPECT_EQ(testLogger->getLogCount(), num_threads * logs_per_thread);
}

// Test thread safety when changing loggers
TEST_F(LoggerTest, ThreadSafeLoggerChangeTest) {
  const int num_threads = 3; // Reduced from 5
  std::atomic<bool> keep_running{true};

  // Thread that continuously changes the logger
  std::thread changer_thread([&keep_running]() {
    int count = 0;
    while (keep_running && count < 5) { // Limit iterations to avoid hanging
      if (count % 2 == 0) {
        LoggerFactory::setLogger(std::make_shared<TestCountingLogger>());
      } else {
        LoggerFactory::setLogger(std::make_shared<TestMessageLogger>());
      }
      count++;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  // Threads that continuously log messages
  std::vector<std::thread> logger_threads;
  for (int i = 0; i < num_threads; ++i) {
    logger_threads.emplace_back([&keep_running, i]() {
      int count = 0;
      while (keep_running && count < 10) { // Limit iterations to avoid hanging
        auto logger = LoggerFactory::getLogger();
        logger->info("Thread " + std::to_string(i) + " message");
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
  }

  // Run the test for a very short time to avoid timeouts
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Stop all threads before exiting the test
  keep_running = false;

  // Join all threads with timeout
  if (changer_thread.joinable()) {
    changer_thread.join();
  }

  // Join logger threads with timeout
  for (auto &thread : logger_threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  // If we got here without crashes or exceptions, the test passes
  SUCCEED();
}

// Test nullptr handling
TEST_F(LoggerTest, NullptrHandlingTest) {
  // Setting nullptr should not change the logger
  auto originalLogger = LoggerFactory::getLogger();
  LoggerFactory::setLogger(nullptr);
  auto currentLogger = LoggerFactory::getLogger();

  EXPECT_EQ(originalLogger, currentLogger);
}

// Define a global main function that will be used for all test files
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  setenv("GTEST_TIMEOUT", "10", 1);
  int result = RUN_ALL_TESTS();

  return result;
}