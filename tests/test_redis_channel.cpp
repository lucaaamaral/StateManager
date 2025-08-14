#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "client/RedisChannel.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

// Mock callback class to track invocations
class MockCallback {
private:
  std::atomic<int> call_count_{0};
  std::string last_message_;
  std::mutex message_mutex_;
  std::condition_variable cv_;
  bool message_received_ = false;

public:
  void operator()(const std::string &message) {
    std::lock_guard<std::mutex> lock(this->message_mutex_);
    this->call_count_++;
    this->last_message_ = message;
    this->message_received_ = true;
    cv_.notify_all();
  }

  int getCallCount() const { return call_count_; }

  std::string getLastMessage() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(message_mutex_));
    return last_message_;
  }

  bool waitForMessage(int timeout_ms = 200) {
    std::unique_lock<std::mutex> lock(message_mutex_);
    return cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                        [this] { return message_received_; });
  }

  void reset() {
    std::lock_guard<std::mutex> lock(message_mutex_);
    call_count_ = 0;
    last_message_.clear();
    message_received_ = false;
  }
};

// Test fixture for RedisChannel tests
class RedisChannelTest : public ::testing::Test {
protected:
  RedisChannel *channel;
  std::shared_ptr<logging::LoggerIface> logger;

  void SetUp() override {
    logger = logging::LoggerFactory::getLogger();
    logging::LoggerFactory::setLevel(logging::LogLevel::TRACE);
    channel = &RedisChannel::getInstance();

    // Give some time for initialization
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  void TearDown() override {
    if (channel) {
      channel->unsubscribe(); // Cleanup all subscriptions
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }
};

// Test singleton instance access
TEST_F(RedisChannelTest, SingletonAccess) {
  EXPECT_NO_THROW({
    RedisChannel &instance1 = RedisChannel::getInstance();
    RedisChannel &instance2 = RedisChannel::getInstance();

    // Both references should point to the same instance
    EXPECT_EQ(&instance1, &instance2);
  });
}

// Test subscribing to a single channel and receiving messages
TEST_F(RedisChannelTest, SubscribeAndReceiveMessage) {
  auto callback = std::make_shared<MockCallback>();
  const std::string test_channel = "test_channel_1";
  const std::string test_message = "Hello, Redis!";

  // Subscribe to channel with lambda that captures shared_ptr
  channel->subscribe(test_channel,
                     [callback](const std::string &msg) { (*callback)(msg); });

  // Give some time for subscription to be established
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish a message using RedisChannel's publish method
  bool publish_result = channel->publish(test_channel, test_message);
  EXPECT_TRUE(publish_result);

  // Wait for message to be received
  EXPECT_TRUE(callback->waitForMessage(100));
  EXPECT_EQ(callback->getCallCount(), 1);
  EXPECT_EQ(callback->getLastMessage(), test_message);
}

// Test subscribing to multiple channels
TEST_F(RedisChannelTest, SubscribeUnsubscribeSubscribe) {
  auto callback1 = std::make_shared<MockCallback>();
  auto callback2 = std::make_shared<MockCallback>();
  const std::string channel1 = "test_channel_1";
  const std::string channel2 = "test_channel_2";
  const std::string message1 = "Message for channel 1";
  const std::string message2 = "Message for channel 2";

  // Subscribe to channel with lambda that captures shared_ptr
  channel->subscribe(
      channel1, [callback1](const std::string &msg) { (*callback1)(msg); });

  // Give some time for subscription to be established
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish a message using RedisChannel's publish method
  bool publish_result = channel->publish(channel1, message1);
  EXPECT_TRUE(publish_result);

  // Wait for message to be received
  EXPECT_TRUE(callback1->waitForMessage(100));
  EXPECT_EQ(callback1->getCallCount(), 1);
  EXPECT_EQ(callback1->getLastMessage(), message1);

  channel->unsubscribe();
  EXPECT_TRUE(callback1->waitForMessage(10));

  // Subscribe to both channels with lambda that captures shared_ptr
  channel->subscribe(
      channel1, [callback1](const std::string &msg) { (*callback1)(msg); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  channel->subscribe(
      channel2, [callback2](const std::string &msg) { (*callback2)(msg); });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish messages to both channels
  EXPECT_TRUE(channel->publish(channel1, message1));
  EXPECT_TRUE(channel->publish(channel2, message2));

  // Wait for messages
  EXPECT_TRUE(callback1->waitForMessage(200));
  EXPECT_TRUE(callback2->waitForMessage(200));

  EXPECT_EQ(callback1->getLastMessage(), message1);
  EXPECT_EQ(callback2->getLastMessage(), message2);
}

// Test unsubscribing from a specific channel
TEST_F(RedisChannelTest, UnsubscribeSpecificChannel) {
  auto callback1 = std::make_shared<MockCallback>();
  auto callback2 = std::make_shared<MockCallback>();
  const std::string channel1 = "test_channel_1";
  const std::string channel2 = "test_channel_2";

  // Subscribe to multiple channels
  channel->subscribe(
      channel1, [callback1](const std::string &msg) { (*callback1)(msg); });
  channel->subscribe(
      channel2, [callback2](const std::string &msg) { (*callback2)(msg); });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Unsubscribe from one channel
  channel->unsubscribe(channel1);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish messages to both channels
  channel->publish(channel1, "Should not receive this");
  channel->publish(channel2, "Should receive this");

  // Only callback2 should receive message
  EXPECT_FALSE(callback1->waitForMessage(500));
  EXPECT_TRUE(callback2->waitForMessage(2000));

  EXPECT_EQ(callback1->getCallCount(), 0);
  EXPECT_EQ(callback2->getCallCount(), 1);
}

// Test unsubscribing from all channels
TEST_F(RedisChannelTest, UnsubscribeAllChannels) {
  auto callback1 = std::make_shared<MockCallback>();
  auto callback2 = std::make_shared<MockCallback>();
  const std::string channel1 = "test_channel_1";
  const std::string channel2 = "test_channel_2";

  // Subscribe to multiple channels
  channel->subscribe(
      channel1, [callback1](const std::string &msg) { (*callback1)(msg); });
  channel->subscribe(
      channel2, [callback2](const std::string &msg) { (*callback2)(msg); });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Unsubscribe from all channels
  channel->unsubscribe();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish messages
  channel->publish(channel1, "Should not receive this");
  channel->publish(channel2, "Should not receive this either");

  // Neither callback should receive messages
  EXPECT_FALSE(callback1->waitForMessage(500));
  EXPECT_FALSE(callback2->waitForMessage(500));

  EXPECT_EQ(callback1->getCallCount(), 0);
  EXPECT_EQ(callback2->getCallCount(), 0);
}

// Test callback replacement for same channel
TEST_F(RedisChannelTest, CallbackReplacement) {
  auto callback1 = std::make_shared<MockCallback>();
  auto callback2 = std::make_shared<MockCallback>();
  const std::string test_channel = "test_channel";
  const std::string test_message = "Test message";

  // Subscribe with first callback
  channel->subscribe(
      test_channel, [callback1](const std::string &msg) { (*callback1)(msg); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Subscribe again with different callback (should replace)
  channel->subscribe(
      test_channel, [callback2](const std::string &msg) { (*callback2)(msg); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish message
  channel->publish(test_channel, test_message);

  // Only callback2 should receive the message
  EXPECT_FALSE(callback1->waitForMessage(500));
  EXPECT_TRUE(callback2->waitForMessage(2000));

  EXPECT_EQ(callback1->getCallCount(), 0);
  EXPECT_EQ(callback2->getCallCount(), 1);
  EXPECT_EQ(callback2->getLastMessage(), test_message);
}

// Test multiple messages on same channel
TEST_F(RedisChannelTest, MultipleMessages) {
  auto callback = std::make_shared<MockCallback>();
  const std::string test_channel = "test_channel";
  const int message_count = 5;

  channel->subscribe(test_channel,
                     [callback](const std::string &msg) { (*callback)(msg); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish multiple messages
  for (int i = 0; i < message_count; ++i) {
    channel->publish(test_channel, "Message " + std::to_string(i));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Wait for all messages to be processed
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  EXPECT_EQ(callback->getCallCount(), message_count);
}

// Test thread safety of subscription operations
TEST_F(RedisChannelTest, ThreadSafetySubscriptions) {
  const int num_threads = 3;
  const int subscriptions_per_thread = 5;
  std::vector<std::thread> threads;
  std::vector<std::shared_ptr<MockCallback>> callbacks;

  // Create callbacks
  for (int i = 0; i < num_threads * subscriptions_per_thread; ++i) {
    callbacks.push_back(std::make_shared<MockCallback>());
  }

  // Create threads that subscribe to channels using singleton
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([t, subscriptions_per_thread, &callbacks]() {
      RedisChannel &instance = RedisChannel::getInstance();
      for (int i = 0; i < subscriptions_per_thread; ++i) {
        int callback_index = t * subscriptions_per_thread + i;
        std::string channel_name =
            "thread_" + std::to_string(t) + "_channel_" + std::to_string(i);

        auto callback = callbacks[callback_index];
        instance.subscribe(channel_name, [callback](const std::string &msg) {
          (*callback)(msg);
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  }

  // Wait for all threads to complete
  for (auto &thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Test that subscriptions work by publishing messages
  for (int t = 0; t < num_threads; ++t) {
    for (int i = 0; i < subscriptions_per_thread; ++i) {
      std::string channel_name =
          "thread_" + std::to_string(t) + "_channel_" + std::to_string(i);
      channel->publish(channel_name, "Test message");
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Verify messages were received
  int total_messages = 0;
  for (auto &callback : callbacks) {
    total_messages += callback->getCallCount();
  }
  EXPECT_EQ(total_messages, num_threads * subscriptions_per_thread);
}

// Test constants are properly defined
TEST_F(RedisChannelTest, ConstantsTest) {
  EXPECT_EQ(RedisChannel::MAX_WORKER_COUNT, 5);
  EXPECT_EQ(RedisChannel::UP_THRESHOLD, 10);
  EXPECT_EQ(RedisChannel::DOWN_THRESHOLD, 3);
}

// Test cleanup functionality with singleton
TEST_F(RedisChannelTest, CleanupTest) {
  auto callback = std::make_shared<MockCallback>();
  const std::string test_channel = "cleanup_test_channel";

  // Subscribe to a channel
  channel->subscribe(test_channel,
                     [callback](const std::string &msg) { (*callback)(msg); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Unsubscribe should clean up properly
  channel->unsubscribe(test_channel);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Publish message - should not be received since we unsubscribed
  channel->publish(test_channel, "Should not receive");

  EXPECT_FALSE(callback->waitForMessage(500));
  EXPECT_EQ(callback->getCallCount(), 0);
}

// Test rapid subscribe/unsubscribe operations
TEST_F(RedisChannelTest, RapidSubscribeUnsubscribe) {
  auto callback = std::make_shared<MockCallback>();
  const std::string test_channel = "rapid_test_channel";

  // Perform rapid subscribe/unsubscribe operations
  for (int i = 0; i < 5; ++i) {
    channel->subscribe(
        test_channel, [callback](const std::string &msg) { (*callback)(msg); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    channel->publish(test_channel, "Message " + std::to_string(i));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    channel->unsubscribe(test_channel);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  // Should have received 5 messages
  EXPECT_EQ(callback->getCallCount(), 5);
}

// Test message handling with different data sizes
TEST_F(RedisChannelTest, VariableMessageSizes) {
  auto callback = std::make_shared<MockCallback>();
  const std::string test_channel = "size_test_channel";

  channel->subscribe(test_channel,
                     [callback](const std::string &msg) { (*callback)(msg); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Test different message sizes
  std::vector<std::string> messages = {
      "small", std::string(100, 'A'), // 100 characters
      std::string(1000, 'B'),         // 1KB
      std::string(10000, 'C')         // 10KB
  };

  for (const auto &msg : messages) {
    callback->reset();
    bool published = channel->publish(test_channel, msg);
    EXPECT_TRUE(published);

    EXPECT_TRUE(callback->waitForMessage(2000));
    EXPECT_EQ(callback->getLastMessage(), msg);
  }
}

// Test publishing without subscribers
TEST_F(RedisChannelTest, PublishWithoutSubscribers) {
  const std::string test_channel = "no_subscribers_channel";
  const std::string test_message = "This message has no subscribers";

  // Should be able to publish even without subscribers
  bool result = channel->publish(test_channel, test_message);
  EXPECT_TRUE(result);
}

// Test unsubscribing from non-existent channel
TEST_F(RedisChannelTest, UnsubscribeNonExistentChannel) {
  const std::string non_existent_channel = "non_existent_channel";

  // Should not throw when unsubscribing from non-existent channel
  EXPECT_NO_THROW({ channel->unsubscribe(non_existent_channel); });
}

// Debug test to check consumer thread behavior
TEST_F(RedisChannelTest, DebugConsumerThread) {
  auto callback = std::make_shared<MockCallback>();
  const std::string test_channel = "debug_channel";

  logger->info("=== Debug Consumer Thread Test ===");

  // Subscribe
  channel->subscribe(test_channel,
                     [callback](const std::string &msg) { (*callback)(msg); });

  logger->info("Subscription completed, waiting 2 seconds...");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  logger->info("Publishing message...");
  bool publish_result = channel->publish(test_channel, "debug_message");
  logger->info("Publish result: " +
               std::string(publish_result ? "SUCCESS" : "FAILED"));

  logger->info("Waiting 3 seconds for message consumption...");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  logger->info("Final callback count: " +
               std::to_string(callback->getCallCount()));

  // Don't assert, just observe behavior
  SUCCEED();
}

// Debug test for multiple subscriptions
TEST_F(RedisChannelTest, DebugMultipleSubscriptions) {
  logger->info("=== Debug Multiple Subscriptions Test ===");

  auto callback1 = std::make_shared<MockCallback>();
  auto callback2 = std::make_shared<MockCallback>();

  // Subscribe to first channel
  logger->info("Subscribing to channel1...");
  channel->subscribe("debug_channel_1", [callback1](const std::string &msg) {
    (*callback1)(msg);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Subscribe to second channel
  logger->info("Subscribing to channel2...");
  channel->subscribe("debug_channel_2", [callback2](const std::string &msg) {
    (*callback2)(msg);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Test publish to both channels
  logger->info("Publishing to channel1...");
  bool result1 = channel->publish("debug_channel_1", "message1");
  logger->info("Publish channel1 result: " +
               std::string(result1 ? "SUCCESS" : "FAILED"));

  logger->info("Publishing to channel2...");
  bool result2 = channel->publish("debug_channel_2", "message2");
  logger->info("Publish channel2 result: " +
               std::string(result2 ? "SUCCESS" : "FAILED"));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  logger->info("Callback1 count: " + std::to_string(callback1->getCallCount()));
  logger->info("Callback2 count: " + std::to_string(callback2->getCallCount()));

  SUCCEED();
}

// Test subscribe/unsubscribe/resubscribe pattern with thorough publishing
TEST_F(RedisChannelTest, SubscribeUnsubscribeResubscribePattern) {
  logger->info("=== Subscribe/Unsubscribe/Resubscribe Pattern Test ===");

  // Test subscribing to a channel, unsubscribing, then subscribing to another
  // channel (5x)
  for (int i = 1; i <= 5; ++i) {
    logger->info("=== ITERATION " + std::to_string(i) + " ===");

    auto callback = std::make_shared<MockCallback>();
    std::string channel_name = "test_cycle_channel_" + std::to_string(i);
    std::string test_message = "cycle_message_" + std::to_string(i);

    // Step 1: Subscribe to channel
    logger->info("Step 1: Subscribing to " + channel_name);
    channel->subscribe(
        channel_name, [callback](const std::string &msg) { (*callback)(msg); });

    // Wait for subscription to be established
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Step 2: Test publishing - First publish attempt
    logger->info("Step 2: First publish attempt to " + channel_name);
    bool publish_result1 =
        channel->publish(channel_name, test_message + "_first");
    logger->info("First publish result: " +
                 std::string(publish_result1 ? "SUCCESS" : "FAILED"));

    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logger->info("After first publish - Callback count: " +
                 std::to_string(callback->getCallCount()));

    // Step 3: Second publish attempt (without unsubscribing)
    logger->info("Step 3: Second publish attempt to " + channel_name);
    bool publish_result2 =
        channel->publish(channel_name, test_message + "_second");
    logger->info("Second publish result: " +
                 std::string(publish_result2 ? "SUCCESS" : "FAILED"));

    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logger->info("After second publish - Callback count: " +
                 std::to_string(callback->getCallCount()));

    // Step 4: Third publish attempt (stress test)
    logger->info("Step 4: Third publish attempt to " + channel_name);
    bool publish_result3 =
        channel->publish(channel_name, test_message + "_third");
    logger->info("Third publish result: " +
                 std::string(publish_result3 ? "SUCCESS" : "FAILED"));

    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    logger->info("After third publish - Final callback count: " +
                 std::to_string(callback->getCallCount()));

    // Step 5: Unsubscribe from channel
    logger->info("Step 5: Unsubscribing from " + channel_name);
    channel->unsubscribe(channel_name);

    // Wait for unsubscription to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Step 6: Test publishing after unsubscribe (should fail or return 0
    // subscribers)
    logger->info("Step 6: Publish after unsubscribe to " + channel_name);
    bool publish_result4 =
        channel->publish(channel_name, test_message + "_after_unsub");
    logger->info("Publish after unsubscribe result: " +
                 std::string(publish_result4 ? "SUCCESS" : "FAILED"));

    // Final callback count should remain unchanged
    logger->info("Final callback count after unsubscribe: " +
                 std::to_string(callback->getCallCount()));

    // Log summary for this iteration
    logger->info("ITERATION " + std::to_string(i) + " SUMMARY:");
    logger->info(
        "  - Publishes before unsub: " + std::to_string(publish_result1) +
        ", " + std::to_string(publish_result2) + ", " +
        std::to_string(publish_result3));
    logger->info("  - Publish after unsub: " + std::to_string(publish_result4));
    logger->info("  - Total messages received: " +
                 std::to_string(callback->getCallCount()));

    // Short pause between iterations
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  logger->info("=== Pattern test completed successfully ===");
  SUCCEED();
}

} // namespace StateManager