#include "core/StateManager.h"
#include "return/StateObj.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

// Remove ambiguous using namespace - use explicit scope resolution instead

void printSeparator(const std::string &title) {
  std::cout << "\n" << std::string(60, '=') << std::endl;
  std::cout << " " << title << std::endl;
  std::cout << std::string(60, '=') << std::endl;
}

// StateObj classes for pub/sub messages
class UserEvent : public StateManager::StateObj {
public:
  std::string event_type;
  int user_id;
  std::string username;
  std::string timestamp;
  std::string ip_address;
  int session_duration;
  std::vector<std::string> changes;

  UserEvent() = default;
  UserEvent(const std::string &event_type, int user_id,
            const std::string &username, const std::string &timestamp)
      : event_type(event_type), user_id(user_id), username(username),
        timestamp(timestamp), session_duration(0) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(UserEvent, event_type, user_id, username, timestamp,
                      ip_address, session_duration, changes)

class OrderMessage : public StateManager::StateObj {
public:
  std::string action;
  int order_id;
  int user_id;
  int items;
  std::string tracking;
  std::string delivery_time;

  OrderMessage() = default;
  OrderMessage(const std::string &action, int order_id, int user_id = 0,
               int items = 0)
      : action(action), order_id(order_id), user_id(user_id), items(items) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(OrderMessage, action, order_id, user_id, items, tracking,
                      delivery_time)

class PaymentMessage : public StateManager::StateObj {
public:
  int order_id;
  double amount;
  std::string status;
  std::string method;
  std::string transaction_id;

  PaymentMessage() = default;
  PaymentMessage(int order_id, double amount, const std::string &status,
                 const std::string &method = "")
      : order_id(order_id), amount(amount), status(status), method(method) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(PaymentMessage, order_id, amount, status, method,
                      transaction_id)

class NotificationMessage : public StateManager::StateObj {
public:
  std::string type;
  std::string message;

  NotificationMessage() = default;
  NotificationMessage(const std::string &type, const std::string &message)
      : type(type), message(message) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(NotificationMessage, type, message)

class BroadcastMessage : public StateManager::StateObj {
public:
  int message_id;
  std::string message;
  long long timestamp;
  std::string priority;

  BroadcastMessage() = default;
  BroadcastMessage(int message_id, const std::string &message,
                   const std::string &priority = "normal")
      : message_id(message_id), message(message), priority(priority) {
    timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(BroadcastMessage, message_id, message, timestamp,
                      priority)

class StateChangeNotification : public StateManager::StateObj {
public:
  std::string operation;
  std::string key;
  std::string timestamp;
  nlohmann::json old_value;
  nlohmann::json new_value;

  StateChangeNotification() = default;
  StateChangeNotification(const std::string &operation, const std::string &key,
                          const std::string &timestamp)
      : operation(operation), key(key), timestamp(timestamp) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(StateChangeNotification, operation, key, timestamp,
                      old_value, new_value)

class UserData : public StateManager::StateObj {
public:
  int id;
  std::string name;
  std::string email;
  std::string status;
  std::string last_login;
  int login_count;

  UserData() = default;
  UserData(int id, const std::string &name, const std::string &email,
           const std::string &status)
      : id(id), name(name), email(email), status(status), login_count(0) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(UserData, id, name, email, status, last_login,
                      login_count)

void demonstrateBasicPubSub() {
  printSeparator("BASIC PUB/SUB DEMO");

  StateManager::RedisConfig config;
  StateManager::StateManager publisher(config);
  StateManager::StateManager subscriber(config);

  std::atomic<int> messagesReceived{0};
  std::mutex outputMutex;

  std::cout << "Setting up basic publish/subscribe demonstration..."
            << std::endl;

  // Subscribe to a channel
  const std::string channel = "user_events";
  std::cout << "\n1. Subscribing to channel: " << channel << std::endl;

  subscriber.subscribe(
      channel,
      [&](std::unique_ptr<StateManager::StateObj> message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        messagesReceived.fetch_add(1);

        // Cast to specific message type
        UserEvent *userEvent = dynamic_cast<UserEvent *>(message.get());
        if (userEvent) {
          std::cout << "📨 Received user event: " << userEvent->event_type
                    << std::endl;
          std::cout << "   Event Type: " << userEvent->event_type << std::endl;
          std::cout << "   User ID: " << userEvent->user_id << std::endl;
          std::cout << "   Username: " << userEvent->username << std::endl;
          std::cout << "   Timestamp: " << userEvent->timestamp << std::endl;
          if (!userEvent->ip_address.empty()) {
            std::cout << "   IP Address: " << userEvent->ip_address
                      << std::endl;
          }
          if (userEvent->session_duration > 0) {
            std::cout << "   Session Duration: " << userEvent->session_duration
                      << " seconds" << std::endl;
          }
          if (!userEvent->changes.empty()) {
            std::cout << "   Changes: ";
            for (const auto &change : userEvent->changes) {
              std::cout << change << " ";
            }
            std::cout << std::endl;
          }
        }
      },
      UserEvent{});

  // Give subscription time to establish
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::cout << "\n2. Publishing messages to channel..." << std::endl;

  // Publish several messages
  std::vector<UserEvent> messages = {
      UserEvent("user_login", 1001, "alice", "2024-08-05T15:30:00Z"),
      UserEvent("user_logout", 1001, "alice", "2024-08-05T16:45:00Z"),
      UserEvent("profile_update", 1002, "bob", "2024-08-05T17:00:00Z")};

  // Set additional fields for specific messages
  messages[0].ip_address = "192.168.1.100";
  messages[1].session_duration = 4500;
  messages[2].changes = {"email", "phone"};

  for (const auto &message : messages) {
    std::cout << "📤 Publishing: " << message.event_type << std::endl;
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

  StateManager::RedisConfig config;
  StateManager::StateManager stateManager(config);

  std::atomic<int> ordersReceived{0};
  std::atomic<int> paymentsReceived{0};
  std::atomic<int> notificationsReceived{0};
  std::mutex outputMutex;

  std::cout << "Demonstrating multiple channel subscriptions..." << std::endl;

  // Subscribe to different channels
  std::cout << "\n1. Setting up subscriptions..." << std::endl;

  // Orders channel
  stateManager.subscribe(
      "orders",
      [&](std::unique_ptr<StateManager::StateObj> message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        ordersReceived.fetch_add(1);
        OrderMessage *order = dynamic_cast<OrderMessage *>(message.get());
        if (order) {
          std::cout << "🛒 ORDER: " << order->action << " - Order #"
                    << order->order_id << std::endl;
        }
      },
      OrderMessage{});

  // Payments channel
  stateManager.subscribe(
      "payments",
      [&](std::unique_ptr<StateManager::StateObj> message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        paymentsReceived.fetch_add(1);
        PaymentMessage *payment = dynamic_cast<PaymentMessage *>(message.get());
        if (payment) {
          std::cout << "💳 PAYMENT: $" << payment->amount << " - "
                    << payment->status << std::endl;
        }
      },
      PaymentMessage{});

  // Notifications channel
  stateManager.subscribe(
      "notifications",
      [&](std::unique_ptr<StateManager::StateObj> message) {
        std::lock_guard<std::mutex> lock(outputMutex);
        notificationsReceived.fetch_add(1);
        NotificationMessage *notification =
            dynamic_cast<NotificationMessage *>(message.get());
        if (notification) {
          std::cout << "🔔 NOTIFICATION: " << notification->message
                    << std::endl;
        }
      },
      NotificationMessage{});

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  std::cout << "\n2. Publishing to different channels..." << std::endl;

  // Simulate a complete e-commerce workflow
  std::vector<std::pair<std::string, std::unique_ptr<StateManager::StateObj>>>
      channelMessages;

  // Create order
  channelMessages.emplace_back(
      "orders", std::make_unique<OrderMessage>("created", 12345, 1001, 3));
  channelMessages.emplace_back(
      "notifications",
      std::make_unique<NotificationMessage>(
          "order_confirmation", "Your order #12345 has been confirmed"));

  // Process payment
  auto payment1 = std::make_unique<PaymentMessage>(12345, 99.99, "processing",
                                                   "credit_card");
  channelMessages.emplace_back("payments", std::move(payment1));

  auto payment2 = std::make_unique<PaymentMessage>(12345, 99.99, "completed");
  payment2->transaction_id = "tx_789";
  channelMessages.emplace_back("payments", std::move(payment2));

  // Ship order
  auto shipOrder = std::make_unique<OrderMessage>("shipped", 12345);
  shipOrder->tracking = "TRK123456789";
  channelMessages.emplace_back("orders", std::move(shipOrder));
  channelMessages.emplace_back(
      "notifications",
      std::make_unique<NotificationMessage>(
          "shipping",
          "Your order #12345 has been shipped. Tracking: TRK123456789"));

  // Deliver order
  auto deliverOrder = std::make_unique<OrderMessage>("delivered", 12345);
  deliverOrder->delivery_time = "2024-08-07T14:30:00Z";
  channelMessages.emplace_back("orders", std::move(deliverOrder));
  channelMessages.emplace_back(
      "notifications", std::make_unique<NotificationMessage>(
                           "delivery", "Your order #12345 has been delivered"));

  for (const auto &[channel, message] : channelMessages) {
    std::cout << "📤 Publishing to " << channel << " channel..." << std::endl;
    stateManager.publish(channel, *message);
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
  std::cout << "  Notifications received: " << notificationsReceived.load()
            << std::endl;
}

void demonstrateMultipleSubscribers() {
  printSeparator("MULTIPLE SUBSCRIBERS DEMO");

  const std::string channel = "broadcast_channel";
  const int numSubscribers = 5;
  std::vector<std::unique_ptr<StateManager::StateManager>> subscribers;
  std::vector<std::atomic<int>> messageCounters(numSubscribers);
  std::mutex outputMutex;

  StateManager::RedisConfig config;
  StateManager::StateManager publisher(config);

  std::cout << "Setting up " << numSubscribers
            << " subscribers to the same channel..." << std::endl;

  // Create multiple subscribers
  for (int i = 0; i < numSubscribers; ++i) {
    subscribers.push_back(std::make_unique<StateManager::StateManager>(config));
    messageCounters[i] = 0;

    subscribers[i]->subscribe(
        channel,
        [&, i](std::unique_ptr<StateManager::StateObj> message) {
          std::lock_guard<std::mutex> lock(outputMutex);
          messageCounters[i].fetch_add(1);
          BroadcastMessage *broadcast =
              dynamic_cast<BroadcastMessage *>(message.get());
          if (broadcast) {
            std::cout << "Subscriber " << i
                      << " received: " << broadcast->message << std::endl;
          }
        },
        BroadcastMessage{});
  }

  // Give subscriptions time to establish
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "\nPublishing broadcast messages..." << std::endl;

  // Publish several broadcast messages
  for (int msgId = 1; msgId <= 5; ++msgId) {
    BroadcastMessage message(msgId,
                             "Broadcast message #" + std::to_string(msgId),
                             msgId % 2 == 0 ? "high" : "normal");

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
    std::cout << "  Subscriber " << i << ": " << count << " messages"
              << std::endl;
    totalReceived += count;
  }
  std::cout << "  Total messages received: " << totalReceived << std::endl;
  std::cout << "  Expected total: " << (5 * numSubscribers) << std::endl;
}

void demonstrateStateChangeNotifications() {
  printSeparator("STATE CHANGE NOTIFICATIONS DEMO");

  StateManager::RedisConfig config;
  StateManager::StateManager dataManager(config);
  StateManager::StateManager notificationManager(config);

  std::atomic<int> notificationsReceived{0};
  std::mutex outputMutex;

  std::cout << "Demonstrating state change notifications pattern..."
            << std::endl;
  std::cout
      << "This shows how to combine data operations with pub/sub notifications."
      << std::endl;

  // Subscribe to state change notifications
  const std::string notificationChannel = "state_changes";

  notificationManager.subscribe(
      notificationChannel,
      [&](std::unique_ptr<StateManager::StateObj> notification) {
        std::lock_guard<std::mutex> lock(outputMutex);
        notificationsReceived.fetch_add(1);

        StateChangeNotification *stateChange =
            dynamic_cast<StateChangeNotification *>(notification.get());
        if (stateChange) {
          std::cout << "🔔 State Change Notification:" << std::endl;
          std::cout << "   Operation: " << stateChange->operation << std::endl;
          std::cout << "   Key: " << stateChange->key << std::endl;
          if (!stateChange->old_value.is_null()) {
            std::cout << "   Old Value: " << stateChange->old_value.dump()
                      << std::endl;
          }
          if (!stateChange->new_value.is_null()) {
            std::cout << "   New Value: " << stateChange->new_value.dump()
                      << std::endl;
          }
          std::cout << "   Timestamp: " << stateChange->timestamp << std::endl;
        }
      },
      StateChangeNotification{});

  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  // Helper function to publish state change notifications
  auto publishStateChange = [&](const std::string &operation,
                                const std::string &key,
                                const nlohmann::json &oldValue = nullptr,
                                const nlohmann::json &newValue = nullptr) {
    StateChangeNotification notification(
        operation, key,
        "2024-08-05T" +
            std::to_string(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count() %
                86400) +
            "Z");

    if (!oldValue.is_null()) {
      notification.old_value = oldValue;
    }
    if (!newValue.is_null()) {
      notification.new_value = newValue;
    }

    dataManager.publish(notificationChannel, notification);
  };

  std::cout << "\nPerforming data operations with notifications..."
            << std::endl;

  // 1. Create operation
  const std::string userKey = "user:2001";
  UserData userData(2001, "Charlie Brown", "charlie@example.com", "active");

  std::cout << "\n1. Creating user data..." << std::endl;
  bool createSuccess = dataManager.write(userKey, userData);
  if (createSuccess) {
    publishStateChange("CREATE", userKey, nullptr, userData.to_json());
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // 2. Update operation
  std::cout << "\n2. Updating user data..." << std::endl;
  auto [readError, oldObj] = dataManager.read(userKey, UserData{});
  if (!readError.has_value()) {
    UserData *oldUserData = dynamic_cast<UserData *>(oldObj.get());
    if (oldUserData) {
      nlohmann::json oldJson = oldUserData->to_json();

      userData.status = "premium";
      userData.last_login = "2024-08-05T15:30:00Z";
      userData.login_count = 42;

      bool updateSuccess = dataManager.write(userKey, userData);
      if (updateSuccess) {
        publishStateChange("UPDATE", userKey, oldJson, userData.to_json());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
    }
  }

  // 3. Delete operation
  std::cout << "\n3. Deleting user data..." << std::endl;
  auto [deleteReadError, dataToDeleteObj] =
      dataManager.read(userKey, UserData{});
  if (!deleteReadError.has_value()) {
    UserData *dataToDelete = dynamic_cast<UserData *>(dataToDeleteObj.get());
    if (dataToDelete) {
      nlohmann::json dataToDeleteJson = dataToDelete->to_json();
      bool deleteSuccess = dataManager.erase(userKey);
      if (deleteSuccess) {
        publishStateChange("DELETE", userKey, dataToDeleteJson, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      }
    }
  }

  // Wait for final notifications
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "\nCleaning up..." << std::endl;
  notificationManager.unsubscribe(notificationChannel);

  std::cout << "\nResults:" << std::endl;
  std::cout << "  State change notifications received: "
            << notificationsReceived.load() << std::endl;
  std::cout << "  Expected notifications: 3 (CREATE, UPDATE, DELETE)"
            << std::endl;
}

int main() {
  std::cout << "StateManager Library - Pub/Sub Demo (StateObj Interface)"
            << std::endl;
  std::cout << "========================================================="
            << std::endl;
  std::cout << "This demo showcases the publish/subscribe functionality with "
               "StateObj."
            << std::endl;
  std::cout << "Make sure Redis is running on localhost:6379 before running "
               "this demo."
            << std::endl;

  try {
    // Run the demonstrations
    demonstrateBasicPubSub();
    demonstrateMultipleChannels();
    demonstrateMultipleSubscribers();
    demonstrateStateChangeNotifications();

    printSeparator("DEMO COMPLETED");
    std::cout << "All pub/sub demonstrations completed successfully!"
              << std::endl;
    std::cout << "The StateManager library provides robust publish/subscribe"
              << std::endl;
    std::cout << "functionality with type-safe StateObj messaging for real-time"
              << std::endl;
    std::cout << "communication and notifications." << std::endl;

  } catch (const std::exception &e) {
    std::cout << "✗ Exception during demo: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}