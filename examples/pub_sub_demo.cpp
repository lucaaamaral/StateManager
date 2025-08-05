#include "core/StateManager.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace StateManager;

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void demonstrateBasicPubSub() {
    printSeparator("BASIC PUB/SUB DEMO");
    
    RedisConfig config;
    StateManager publisher(config);
    StateManager subscriber(config);
    
    std::atomic<int> messagesReceived{0};
    std::mutex outputMutex;
    
    std::cout << "Setting up basic publish/subscribe demonstration..." << std::endl;
    
    // Subscribe to a channel
    const std::string channel = "user_events";
    std::cout << "\n1. Subscribing to channel: " << channel << std::endl;
    
    subscriber.subscribe(channel, [&](const nlohmann::json& message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        messagesReceived.fetch_add(1);
        std::cout << "📨 Received message: " << message.dump() << std::endl;
        
        // Print specific fields if they exist
        if (message.contains("event_type")) {
            std::cout << "   Event Type: " << message["event_type"].get<std::string>() << std::endl;
        }
        if (message.contains("user_id")) {
            std::cout << "   User ID: " << message["user_id"].get<int>() << std::endl;
        }
    });
    
    // Give subscription time to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    std::cout << "\n2. Publishing messages to channel..." << std::endl;
    
    // Publish several messages
    std::vector<nlohmann::json> messages = {
        {
            {"event_type", "user_login"},
            {"user_id", 1001},
            {"username", "alice"},
            {"timestamp", "2024-08-05T15:30:00Z"},
            {"ip_address", "192.168.1.100"}
        },
        {
            {"event_type", "user_logout"},
            {"user_id", 1001},
            {"username", "alice"},
            {"timestamp", "2024-08-05T16:45:00Z"},
            {"session_duration", 4500}
        },
        {
            {"event_type", "profile_update"},
            {"user_id", 1002},
            {"username", "bob"},
            {"changes", {"email", "phone"}},
            {"timestamp", "2024-08-05T17:00:00Z"}
        }
    };
    
    for (const auto& message : messages) {
        std::cout << "📤 Publishing: " << message["event_type"].get<std::string>() << std::endl;
        bool success = publisher.publish(channel, message);
        if (!success) {
            std::cout << "✗ Failed to publish message" << std::endl;
        }
        
        // Small delay to see messages arrive
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Wait a bit for all messages to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\n3. Unsubscribing from channel..." << std::endl;
    subscriber.unsubscribe(channel);
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Messages published: " << messages.size() << std::endl;
    std::cout << "  Messages received: " << messagesReceived.load() << std::endl;
}

void demonstrateMultipleChannels() {
    printSeparator("MULTIPLE CHANNELS DEMO");
    
    RedisConfig config;
    StateManager stateManager(config);
    
    std::atomic<int> ordersReceived{0};
    std::atomic<int> paymentsReceived{0};
    std::atomic<int> notificationsReceived{0};
    std::mutex outputMutex;
    
    std::cout << "Demonstrating multiple channel subscriptions..." << std::endl;
    
    // Subscribe to different channels
    std::cout << "\n1. Setting up subscriptions..." << std::endl;
    
    // Orders channel
    stateManager.subscribe("orders", [&](const nlohmann::json& message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        ordersReceived.fetch_add(1);
        std::cout << "🛒 ORDER: " << message["action"].get<std::string>() 
                  << " - Order #" << message["order_id"].get<int>() << std::endl;
    });
    
    // Payments channel
    stateManager.subscribe("payments", [&](const nlohmann::json& message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        paymentsReceived.fetch_add(1);
        std::cout << "💳 PAYMENT: $" << message["amount"].get<double>() 
                  << " - " << message["status"].get<std::string>() << std::endl;
    });
    
    // Notifications channel
    stateManager.subscribe("notifications", [&](const nlohmann::json& message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        notificationsReceived.fetch_add(1);
        std::cout << "🔔 NOTIFICATION: " << message["message"].get<std::string>() << std::endl;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    std::cout << "\n2. Publishing to different channels..." << std::endl;
    
    // Simulate a complete e-commerce workflow
    std::vector<std::pair<std::string, nlohmann::json>> channelMessages = {
        {"orders", {{"action", "created"}, {"order_id", 12345}, {"user_id", 1001}, {"items", 3}}},
        {"notifications", {{"type", "order_confirmation"}, {"message", "Your order #12345 has been confirmed"}}},
        {"payments", {{"order_id", 12345}, {"amount", 99.99}, {"status", "processing"}, {"method", "credit_card"}}},
        {"payments", {{"order_id", 12345}, {"amount", 99.99}, {"status", "completed"}, {"transaction_id", "tx_789"}}},
        {"orders", {{"action", "shipped"}, {"order_id", 12345}, {"tracking", "TRK123456789"}}},
        {"notifications", {{"type", "shipping"}, {"message", "Your order #12345 has been shipped. Tracking: TRK123456789"}}},
        {"orders", {{"action", "delivered"}, {"order_id", 12345}, {"delivery_time", "2024-08-07T14:30:00Z"}}},
        {"notifications", {{"type", "delivery"}, {"message", "Your order #12345 has been delivered"}}}
    };
    
    for (const auto& [channel, message] : channelMessages) {
        std::cout << "📤 Publishing to " << channel << " channel..." << std::endl;
        stateManager.publish(channel, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\n3. Cleaning up subscriptions..." << std::endl;
    stateManager.unsubscribe("orders");
    stateManager.unsubscribe("payments");
    stateManager.unsubscribe("notifications");
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Orders received: " << ordersReceived.load() << std::endl;
    std::cout << "  Payments received: " << paymentsReceived.load() << std::endl;
    std::cout << "  Notifications received: " << notificationsReceived.load() << std::endl;
}

void demonstrateMultipleSubscribers() {
    printSeparator("MULTIPLE SUBSCRIBERS DEMO");
    
    const std::string channel = "broadcast_channel";
    const int numSubscribers = 5;
    std::vector<std::unique_ptr<StateManager>> subscribers;
    std::vector<std::atomic<int>> messageCounters(numSubscribers);
    std::mutex outputMutex;
    
    RedisConfig config;
    StateManager publisher(config);
    
    std::cout << "Setting up " << numSubscribers << " subscribers to the same channel..." << std::endl;
    
    // Create multiple subscribers
    for (int i = 0; i < numSubscribers; ++i) {
        subscribers.push_back(std::make_unique<StateManager>(config));
        messageCounters[i] = 0;
        
        subscribers[i]->subscribe(channel, [&, i](const nlohmann::json& message) {
            std::lock_guard<std::mutex> lock(outputMutex);
            messageCounters[i].fetch_add(1);
            std::cout << "Subscriber " << i << " received: " 
                      << message["message"].get<std::string>() << std::endl;
        });
    }
    
    // Give subscriptions time to establish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nPublishing broadcast messages..." << std::endl;
    
    // Publish several broadcast messages
    for (int msgId = 1; msgId <= 5; ++msgId) {
        nlohmann::json message = {
            {"message_id", msgId},
            {"message", "Broadcast message #" + std::to_string(msgId)},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"priority", msgId % 2 == 0 ? "high" : "normal"}
        };
        
        std::cout << "📤 Broadcasting message #" << msgId << std::endl;
        publisher.publish(channel, message);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    // Wait for message processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::cout << "\nCleaning up subscribers..." << std::endl;
    for (int i = 0; i < numSubscribers; ++i) {
        subscribers[i]->unsubscribe(channel);
    }
    
    std::cout << "\nResults:" << std::endl;
    int totalReceived = 0;
    for (int i = 0; i < numSubscribers; ++i) {
        int count = messageCounters[i].load();
        std::cout << "  Subscriber " << i << ": " << count << " messages" << std::endl;
        totalReceived += count;
    }
    std::cout << "  Total messages received: " << totalReceived << std::endl;
    std::cout << "  Expected total: " << (5 * numSubscribers) << std::endl;
}

void demonstrateStateChangeNotifications() {
    printSeparator("STATE CHANGE NOTIFICATIONS DEMO");
    
    RedisConfig config;
    StateManager dataManager(config);
    StateManager notificationManager(config);
    
    std::atomic<int> notificationsReceived{0};
    std::mutex outputMutex;
    
    std::cout << "Demonstrating state change notifications pattern..." << std::endl;
    std::cout << "This shows how to combine data operations with pub/sub notifications." << std::endl;
    
    // Subscribe to state change notifications
    const std::string notificationChannel = "state_changes";
    
    notificationManager.subscribe(notificationChannel, [&](const nlohmann::json& notification) {
        std::lock_guard<std::mutex> lock(outputMutex);
        notificationsReceived.fetch_add(1);
        
        std::cout << "🔔 State Change Notification:" << std::endl;
        std::cout << "   Operation: " << notification["operation"].get<std::string>() << std::endl;
        std::cout << "   Key: " << notification["key"].get<std::string>() << std::endl;
        if (notification.contains("old_value")) {
            std::cout << "   Old Value: " << notification["old_value"].dump() << std::endl;
        }
        if (notification.contains("new_value")) {
            std::cout << "   New Value: " << notification["new_value"].dump() << std::endl;
        }
        std::cout << "   Timestamp: " << notification["timestamp"].get<std::string>() << std::endl;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    // Helper function to publish state change notifications
    auto publishStateChange = [&](const std::string& operation, const std::string& key, 
                                  const nlohmann::json& oldValue = nullptr, 
                                  const nlohmann::json& newValue = nullptr) {
        nlohmann::json notification = {
            {"operation", operation},
            {"key", key},
            {"timestamp", "2024-08-05T" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count() % 86400) + "Z"}
        };
        
        if (!oldValue.is_null()) {
            notification["old_value"] = oldValue;
        }
        if (!newValue.is_null()) {
            notification["new_value"] = newValue;
        }
        
        dataManager.publish(notificationChannel, notification);
    };
    
    std::cout << "\nPerforming data operations with notifications..." << std::endl;
    
    // 1. Create operation
    const std::string userKey = "user:2001";
    nlohmann::json userData = {
        {"id", 2001},
        {"name", "Charlie Brown"},
        {"email", "charlie@example.com"},
        {"status", "active"}
    };
    
    std::cout << "\n1. Creating user data..." << std::endl;
    bool createSuccess = dataManager.write(userKey, userData);
    if (createSuccess) {
        publishStateChange("CREATE", userKey, nullptr, userData);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // 2. Update operation
    std::cout << "\n2. Updating user data..." << std::endl;
    auto [readError, oldData] = dataManager.read(userKey);
    if (!readError.has_value()) {
        nlohmann::json updatedData = oldData;
        updatedData["status"] = "premium";
        updatedData["last_login"] = "2024-08-05T15:30:00Z";
        updatedData["login_count"] = 42;
        
        bool updateSuccess = dataManager.write(userKey, updatedData);
        if (updateSuccess) {
            publishStateChange("UPDATE", userKey, oldData, updatedData);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    // 3. Delete operation
    std::cout << "\n3. Deleting user data..." << std::endl;
    auto [deleteReadError, dataToDelete] = dataManager.read(userKey);
    if (!deleteReadError.has_value()) {
        bool deleteSuccess = dataManager.erase(userKey);
        if (deleteSuccess) {
            publishStateChange("DELETE", userKey, dataToDelete, nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    // Wait for final notifications
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nCleaning up..." << std::endl;
    notificationManager.unsubscribe(notificationChannel);
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  State change notifications received: " << notificationsReceived.load() << std::endl;
    std::cout << "  Expected notifications: 3 (CREATE, UPDATE, DELETE)" << std::endl;
}

int main() {
    std::cout << "StateManager Library - Pub/Sub Demo" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "This demo showcases the publish/subscribe functionality." << std::endl;
    std::cout << "Make sure Redis is running on localhost:6379 before running this demo." << std::endl;
    
    try {
        // Run the demonstrations
        demonstrateBasicPubSub();
        demonstrateMultipleChannels();
        demonstrateMultipleSubscribers();
        demonstrateStateChangeNotifications();
        
        printSeparator("DEMO COMPLETED");
        std::cout << "All pub/sub demonstrations completed successfully!" << std::endl;
        std::cout << "The StateManager library provides robust publish/subscribe" << std::endl;
        std::cout << "functionality for real-time messaging and notifications." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception during demo: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}