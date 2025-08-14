#include "core/StateManager.h"
#include "return/StateObj.h"
#include <iomanip>
#include <iostream>

// Remove ambiguous using namespace - use explicit scope resolution instead

void printSeparator(const std::string &title) {
  std::cout << "\n" << std::string(50, '=') << std::endl;
  std::cout << " " << title << std::endl;
  std::cout << std::string(50, '=') << std::endl;
}

// Example StateObj classes for demonstration
class UserProfile : public StateManager::StateObj {
public:
  int id;
  std::string name;
  std::string email;
  int age;
  bool active;
  nlohmann::json preferences;
  std::vector<std::string> tags;
  std::string last_login;
  int login_count;

  // Default constructor for deserialization
  UserProfile() = default;

  // Constructor with parameters
  UserProfile(int id, const std::string &name, const std::string &email,
              int age, bool active, const nlohmann::json &preferences,
              const std::vector<std::string> &tags)
      : id(id), name(name), email(email), age(age), active(active),
        preferences(preferences), tags(tags), login_count(0) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

// Define StateObj serialization for UserProfile
STATE_OBJ_DEFINE_TYPE(UserProfile, id, name, email, age, active, preferences,
                      tags, last_login, login_count)

class SimpleData : public StateManager::StateObj {
public:
  nlohmann::json value;

  SimpleData() = default;
  SimpleData(const nlohmann::json &val) : value(val) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(SimpleData, value)

class ProductInfo : public StateManager::StateObj {
public:
  std::string name;
  std::string version;
  std::vector<std::string> features;
  nlohmann::json config;

  ProductInfo() = default;
  ProductInfo(const std::string &name, const std::string &version,
              const std::vector<std::string> &features,
              const nlohmann::json &config)
      : name(name), version(version), features(features), config(config) {}

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

STATE_OBJ_DEFINE_TYPE(ProductInfo, name, version, features, config)

void demonstrateBasicOperations() {
  printSeparator("BASIC CRUD OPERATIONS");

  try {
    // Create StateManager with default Redis configuration (localhost:6379)
    StateManager::RedisConfig config; // Uses defaults
    StateManager::StateManager stateManager(config);

    std::cout << "✓ StateManager initialized with default config" << std::endl;

    // 1. WRITE OPERATION
    std::cout << "\n1. Writing StateObj data to Redis..." << std::endl;
    UserProfile userData(
        1001, "Alice Johnson", "alice.johnson@example.com", 28, true,
        nlohmann::json{
            {"theme", "dark"}, {"notifications", true}, {"language", "en"}},
        {"developer", "team-lead", "python"});

    bool writeSuccess = stateManager.write("user:1001", userData);
    if (writeSuccess) {
      std::cout << "✓ Successfully wrote user data" << std::endl;
      std::cout << "  User: " << userData.name << " (" << userData.email << ")"
                << std::endl;
    } else {
      std::cout << "✗ Failed to write user data" << std::endl;
      return;
    }

    // 2. READ OPERATION
    std::cout << "\n2. Reading data from Redis..." << std::endl;
    auto [readError, readObj] = stateManager.read("user:1001", UserProfile{});
    if (!readError.has_value()) {
      std::cout << "✓ Successfully read user data:" << std::endl;

      // Cast to the specific type
      UserProfile *readUser = dynamic_cast<UserProfile *>(readObj.get());
      if (readUser) {
        std::cout << "\n  User details:" << std::endl;
        std::cout << "    Name: " << readUser->name << std::endl;
        std::cout << "    Email: " << readUser->email << std::endl;
        std::cout << "    Age: " << readUser->age << std::endl;
        std::cout << "    Active: " << (readUser->active ? "Yes" : "No")
                  << std::endl;
        std::cout << "    Theme: "
                  << readUser->preferences["theme"].get<std::string>()
                  << std::endl;
        std::cout << "    Tags: ";
        for (const auto &tag : readUser->tags) {
          std::cout << tag << " ";
        }
        std::cout << std::endl;
      }
    } else {
      std::cout << "✗ Failed to read user data: " << readError.value()
                << std::endl;
      return;
    }

    // 3. UPDATE OPERATION (modify and write back)
    std::cout << "\n3. Updating user data..." << std::endl;
    userData.age = 29;
    userData.preferences["theme"] = "light";
    userData.last_login = "2024-08-05T15:30:00Z";
    userData.login_count = 47;

    bool updateSuccess = stateManager.write("user:1001", userData);
    if (updateSuccess) {
      std::cout << "✓ Successfully updated user data" << std::endl;

      // Verify the update
      auto [verifyError, verifyObj] =
          stateManager.read("user:1001", UserProfile{});
      if (!verifyError.has_value()) {
        UserProfile *verifyUser = dynamic_cast<UserProfile *>(verifyObj.get());
        if (verifyUser) {
          std::cout << "  Updated age: " << verifyUser->age << std::endl;
          std::cout << "  Updated theme: "
                    << verifyUser->preferences["theme"].get<std::string>()
                    << std::endl;
          std::cout << "  Last login: " << verifyUser->last_login << std::endl;
          std::cout << "  Login count: " << verifyUser->login_count
                    << std::endl;
        }
      }
    } else {
      std::cout << "✗ Failed to update user data" << std::endl;
    }

    // 4. ERASE OPERATION
    std::cout << "\n4. Deleting user data..." << std::endl;
    bool eraseSuccess = stateManager.erase("user:1001");
    if (eraseSuccess) {
      std::cout << "✓ Successfully deleted user data" << std::endl;

      // Verify deletion
      auto [deleteVerifyError, deleteVerifyObj] =
          stateManager.read("user:1001", UserProfile{});
      if (deleteVerifyError.has_value()) {
        std::cout << "✓ Verified deletion - key no longer exists" << std::endl;
        std::cout << "  Error message: " << deleteVerifyError.value()
                  << std::endl;
      } else {
        std::cout << "✗ Unexpected: key still exists after deletion"
                  << std::endl;
      }
    } else {
      std::cout << "✗ Failed to delete user data" << std::endl;
    }

  } catch (const std::exception &e) {
    std::cout << "✗ Exception in basic operations: " << e.what() << std::endl;
  }
}

void demonstrateDataTypes() {
  printSeparator("STATE OBJECT DATA TYPES DEMO");

  try {
    StateManager::RedisConfig config;
    StateManager::StateManager stateManager(config);

    std::cout << "Testing different data types with StateObj..." << std::endl;

    // Integer
    SimpleData intData(42);
    stateManager.write("test:integer", intData);
    auto [intError, intResult] =
        stateManager.read("test:integer", SimpleData{});
    if (!intError.has_value()) {
      SimpleData *intObj = dynamic_cast<SimpleData *>(intResult.get());
      std::cout << "Integer: " << intObj->value.get<int>() << std::endl;
    }

    // Float
    SimpleData floatData(3.14159);
    stateManager.write("test:float", floatData);
    auto [floatError, floatResult] =
        stateManager.read("test:float", SimpleData{});
    if (!floatError.has_value()) {
      SimpleData *floatObj = dynamic_cast<SimpleData *>(floatResult.get());
      std::cout << "Float: " << std::fixed << std::setprecision(5)
                << floatObj->value.get<double>() << std::endl;
    }

    // Boolean
    SimpleData boolData(true);
    stateManager.write("test:boolean", boolData);
    auto [boolError, boolResult] =
        stateManager.read("test:boolean", SimpleData{});
    if (!boolError.has_value()) {
      SimpleData *boolObj = dynamic_cast<SimpleData *>(boolResult.get());
      std::cout << "Boolean: "
                << (boolObj->value.get<bool>() ? "true" : "false") << std::endl;
    }

    // String
    SimpleData stringData(std::string("Hello, StateManager! 🚀"));
    stateManager.write("test:string", stringData);
    auto [stringError, stringResult] =
        stateManager.read("test:string", SimpleData{});
    if (!stringError.has_value()) {
      SimpleData *stringObj = dynamic_cast<SimpleData *>(stringResult.get());
      std::cout << "String: " << stringObj->value.get<std::string>()
                << std::endl;
    }

    // Array
    SimpleData arrayData(nlohmann::json::array({1, "two", 3.0, true, nullptr}));
    stateManager.write("test:array", arrayData);
    auto [arrayError, arrayResult] =
        stateManager.read("test:array", SimpleData{});
    if (!arrayError.has_value()) {
      SimpleData *arrayObj = dynamic_cast<SimpleData *>(arrayResult.get());
      std::cout << "Array: " << arrayObj->value.dump() << std::endl;
    }

    // Complex Object
    ProductInfo productData(
        "StateManager", "1.0", {"redis", "json", "pubsub"},
        nlohmann::json{{"host", "localhost"}, {"port", 6379}});
    stateManager.write("test:product", productData);
    auto [objError, objResult] =
        stateManager.read("test:product", ProductInfo{});
    if (!objError.has_value()) {
      ProductInfo *productObj = dynamic_cast<ProductInfo *>(objResult.get());
      std::cout << "Product Object:" << std::endl;
      std::cout << "  Name: " << productObj->name << std::endl;
      std::cout << "  Version: " << productObj->version << std::endl;
      std::cout << "  Features: ";
      for (const auto &feature : productObj->features) {
        std::cout << feature << " ";
      }
      std::cout << std::endl;
      std::cout << "  Config: " << productObj->config.dump() << std::endl;
    }

    // Null
    SimpleData nullData(nullptr);
    stateManager.write("test:null", nullData);
    auto [nullError, nullResult] = stateManager.read("test:null", SimpleData{});
    if (!nullError.has_value()) {
      SimpleData *nullObj = dynamic_cast<SimpleData *>(nullResult.get());
      std::cout << "Null: " << (nullObj->value.is_null() ? "null" : "not null")
                << std::endl;
    }

    // Cleanup
    std::cout << "\nCleaning up test data..." << std::endl;
    stateManager.erase("test:integer");
    stateManager.erase("test:float");
    stateManager.erase("test:boolean");
    stateManager.erase("test:string");
    stateManager.erase("test:array");
    stateManager.erase("test:product");
    stateManager.erase("test:null");
    std::cout << "✓ Test data cleaned up" << std::endl;

  } catch (const std::exception &e) {
    std::cout << "✗ Exception in data types demo: " << e.what() << std::endl;
  }
}

void demonstrateErrorHandling() {
  printSeparator("ERROR HANDLING DEMO");

  try {
    StateManager::RedisConfig config;
    StateManager::StateManager stateManager(config);

    std::cout << "Testing error conditions..." << std::endl;

    // 1. Reading non-existent key
    std::cout << "\n1. Reading non-existent key..." << std::endl;
    auto [error1, data1] = stateManager.read("nonexistent:key", SimpleData{});
    if (error1.has_value()) {
      std::cout << "✓ Expected error: " << error1.value() << std::endl;
    } else {
      std::cout << "✗ Unexpected success reading non-existent key" << std::endl;
    }

    // 2. Erasing non-existent key
    std::cout << "\n2. Erasing non-existent key..." << std::endl;
    bool eraseResult = stateManager.erase("nonexistent:key");
    if (!eraseResult) {
      std::cout << "✓ Expected failure when erasing non-existent key"
                << std::endl;
    } else {
      std::cout << "✗ Unexpected success erasing non-existent key" << std::endl;
    }

    // 3. Invalid key (empty string)
    std::cout << "\n3. Using empty key..." << std::endl;
    SimpleData testData("test");
    bool emptyKeyResult = stateManager.write("", testData);
    if (emptyKeyResult) {
      std::cout << "✓ Empty key accepted (implementation dependent)"
                << std::endl;
      // Clean up if it was accepted
      stateManager.erase("");
    } else {
      std::cout << "✓ Expected failure with empty key" << std::endl;
    }

    // 4. Large data handling
    std::cout << "\n4. Testing large data handling..." << std::endl;
    nlohmann::json largeDataValue;
    for (int i = 0; i < 1000; ++i) {
      largeDataValue["item_" + std::to_string(i)] =
          nlohmann::json{{"id", i},
                         {"name", "Item " + std::to_string(i)},
                         {"description", "This is a description for item " +
                                             std::to_string(i)},
                         {"active", i % 2 == 0}};
    }

    SimpleData largeData(largeDataValue);
    bool largeWriteResult = stateManager.write("test:large", largeData);
    if (largeWriteResult) {
      std::cout << "✓ Successfully wrote large data (" << largeDataValue.size()
                << " items)" << std::endl;

      auto [largeReadError, largeReadObj] =
          stateManager.read("test:large", SimpleData{});
      if (!largeReadError.has_value()) {
        SimpleData *largeReadData =
            dynamic_cast<SimpleData *>(largeReadObj.get());
        std::cout << "✓ Successfully read large data back" << std::endl;
        std::cout << "  Verified size: " << largeReadData->value.size()
                  << " items" << std::endl;
        stateManager.erase("test:large");
      }
    } else {
      std::cout << "✗ Failed to write large data" << std::endl;
    }

  } catch (const std::exception &e) {
    std::cout << "✗ Exception in error handling demo: " << e.what()
              << std::endl;
  }
}

int main() {
  std::cout << "StateManager Library - Basic Demo (StateObj Interface)"
            << std::endl;
  std::cout << "======================================================="
            << std::endl;
  std::cout << "This demo showcases the StateObj interface functionality."
            << std::endl;
  std::cout << "Make sure Redis is running on localhost:6379 before running "
               "this demo."
            << std::endl;

  // Run the demonstrations
  demonstrateBasicOperations();
  demonstrateDataTypes();
  demonstrateErrorHandling();

  printSeparator("DEMO COMPLETED");
  std::cout << "All demonstrations completed successfully!" << std::endl;
  std::cout << "The StateObj interface provides type-safe, object-oriented"
            << std::endl;
  std::cout << "access to StateManager's functionality with automatic JSON "
               "serialization."
            << std::endl;

  return 0;
}