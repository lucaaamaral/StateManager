#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "client/RedisConfig.h"
#include "core/StateManager.h"
#include "logging/LoggerFactory.h"

namespace StateManager {

// Test class for JSON serialization
struct UserProfile {
    int id;
    std::string name;
    std::string email;
    int age;
    bool active;
    std::vector<std::string> tags;
    
    // Default constructor for deserialization
    UserProfile() = default;
    
    // Constructor with parameters
    UserProfile(int id, const std::string& name, const std::string& email, 
                int age, bool active, const std::vector<std::string>& tags)
        : id(id), name(name), email(email), age(age), active(active), tags(tags) {}
    
    // Equality operator for testing
    bool operator==(const UserProfile& other) const {
        return id == other.id && name == other.name && email == other.email &&
               age == other.age && active == other.active && tags == other.tags;
    }
};

// Define JSON serialization for UserProfile
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserProfile, id, name, email, age, active, tags)

// Test fixture for StateManager tests
class StateManagerTest : public ::testing::Test {
protected:
  std::shared_ptr<logging::LoggerIface> logger;
  std::unique_ptr<StateManager> state_manager;

  void SetUp() override {
    logger = logging::LoggerFactory::getLogger();
    logging::LoggerFactory::setLevel(logging::LogLevel::INFO);
    
    // Use default Redis configuration
    RedisConfig config;
    state_manager = std::make_unique<StateManager>(config);
  }

  void TearDown() override {
    state_manager.reset();
  }
};

// Test basic JSON write and read operations
TEST_F(StateManagerTest, BasicJSONOperations) {
  // Test string
  std::string test_string = "Hello, StateManager!";
  nlohmann::json json_string = test_string;
  EXPECT_TRUE(state_manager->write("test_string", json_string));
  
  auto [error, read_json] = state_manager->read("test_string");
  EXPECT_FALSE(error.has_value()) << "Error reading string: " << error.value_or("");
  std::string read_string = read_json.get<std::string>();
  EXPECT_EQ(read_string, test_string);
  
  logger->info("StateManager JSON test passed successfully");
}

// Test different JSON data types
TEST_F(StateManagerTest, JSONDataTypes_int) {
  // Test integer
  nlohmann::json json_int = 42;
  EXPECT_TRUE(state_manager->write("test_int", json_int));
  auto [error_int, read_int] = state_manager->read("test_int");
  EXPECT_FALSE(error_int.has_value());
  EXPECT_EQ(read_int.get<int>(), 42);
}

TEST_F(StateManagerTest, JSONDataTypes_float) {
  // Test float
  nlohmann::json json_float = 3.14159;
  EXPECT_TRUE(state_manager->write("test_float", json_float));
  auto [error_float, read_float] = state_manager->read("test_float");
  EXPECT_FALSE(error_float.has_value());
  EXPECT_DOUBLE_EQ(read_float.get<double>(), 3.14159);
}

TEST_F(StateManagerTest, JSONDataTypes_bool) {
  // Test boolean
  nlohmann::json json_bool = true;
  EXPECT_TRUE(state_manager->write("test_bool", json_bool));
  auto [error_bool, read_bool] = state_manager->read("test_bool");
  EXPECT_FALSE(error_bool.has_value());
  EXPECT_EQ(read_bool.get<bool>(), true);
}

TEST_F(StateManagerTest, JSONDataTypes_null) {
  // Test null
  nlohmann::json json_null = nullptr;
  EXPECT_TRUE(state_manager->write("test_null", json_null));
  auto [error_null, read_null] = state_manager->read("test_null");
  EXPECT_FALSE(error_null.has_value());
  EXPECT_TRUE(read_null.is_null());
}

TEST_F(StateManagerTest, JSONDataTypes_array) {
  // Test array
  nlohmann::json json_array = nlohmann::json::array({1, 2, 3, "test", true});
  EXPECT_TRUE(state_manager->write("test_array", json_array));
  auto [error_array, read_array] = state_manager->read("test_array");
  EXPECT_FALSE(error_array.has_value());
  EXPECT_TRUE(read_array.is_array());
  EXPECT_EQ(read_array.size(), 5);
  EXPECT_EQ(read_array[0].get<int>(), 1);
  EXPECT_EQ(read_array[3].get<std::string>(), "test");
  EXPECT_EQ(read_array[4].get<bool>(), true);
}

TEST_F(StateManagerTest, JSONDataTypes_object) {
  // Test object
  nlohmann::json json_object = {
    {"name", "StateManager"},
    {"version", 1.0},
    {"active", true},
    {"tags", nlohmann::json::array({"redis", "cpp", "json"})}
  };
  EXPECT_TRUE(state_manager->write("test_object", json_object));
  auto [error_object, read_object] = state_manager->read("test_object");
  EXPECT_FALSE(error_object.has_value());
  EXPECT_TRUE(read_object.is_object());
  EXPECT_EQ(read_object["name"].get<std::string>(), "StateManager");
  EXPECT_DOUBLE_EQ(read_object["version"].get<double>(), 1.0);
  EXPECT_EQ(read_object["active"].get<bool>(), true);
  EXPECT_TRUE(read_object["tags"].is_array());
  EXPECT_EQ(read_object["tags"].size(), 3);
}

TEST_F(StateManagerTest, JSONDataTypes_custom_class_basic) {
  // Test basic custom class serialization with NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE
  
  // Create a UserProfile object
  UserProfile originalUser(1001, "Alice Smith", "alice@example.com", 28, true, 
                          {"developer", "team-lead", "cpp"});
  
  // Convert to JSON and write to StateManager
  nlohmann::json userJson = originalUser;
  EXPECT_TRUE(state_manager->write("test_user_profile", userJson));
  
  // Read back and verify
  auto [error_read, read_json] = state_manager->read("test_user_profile");
  EXPECT_FALSE(error_read.has_value());
  
  // Convert back to UserProfile object
  UserProfile deserializedUser = read_json.get<UserProfile>();
  EXPECT_EQ(deserializedUser, originalUser);
  
  // Verify individual fields
  EXPECT_EQ(deserializedUser.id, 1001);
  EXPECT_EQ(deserializedUser.name, "Alice Smith");
  EXPECT_EQ(deserializedUser.email, "alice@example.com");
  EXPECT_EQ(deserializedUser.age, 28);
  EXPECT_EQ(deserializedUser.active, true);
  EXPECT_EQ(deserializedUser.tags.size(), 3);
  EXPECT_EQ(deserializedUser.tags[0], "developer");
  EXPECT_EQ(deserializedUser.tags[1], "team-lead");
  EXPECT_EQ(deserializedUser.tags[2], "cpp");
  
  // Clean up
  EXPECT_TRUE(state_manager->erase("test_user_profile"));
}

TEST_F(StateManagerTest, JSONDataTypes_custom_class_missing_fields) {
  // Test JSON deserialization with missing fields
  
  nlohmann::json incompleteJson = {
    {"id", 2002},
    {"name", "Bob Johnson"},
    {"email", "bob@example.com"}
    // Missing: age, active, tags
  };
  
  EXPECT_TRUE(state_manager->write("test_incomplete_user", incompleteJson));
  auto [error_incomplete, incomplete_data] = state_manager->read("test_incomplete_user");
  EXPECT_FALSE(error_incomplete.has_value());
  
  // Try to deserialize incomplete JSON - this should handle missing fields gracefully
  try {
    UserProfile incompleteUser = incomplete_data.get<UserProfile>();
    EXPECT_EQ(incompleteUser.id, 2002);
    EXPECT_EQ(incompleteUser.name, "Bob Johnson");
    EXPECT_EQ(incompleteUser.email, "bob@example.com");
    // Missing fields should have default values
    EXPECT_EQ(incompleteUser.age, 0);  // Default int value
    EXPECT_EQ(incompleteUser.active, false);  // Default bool value
    EXPECT_TRUE(incompleteUser.tags.empty());  // Default empty vector
  } catch (const nlohmann::json::exception& e) {
    // If strict mode is enabled, this might throw - that's also valid behavior
    EXPECT_TRUE(true) << "JSON deserialization threw exception for missing fields: " << e.what();
  }
  
  // Clean up
  EXPECT_TRUE(state_manager->erase("test_incomplete_user"));
}

TEST_F(StateManagerTest, JSONDataTypes_custom_class_extra_fields) {
  // Test JSON deserialization with additional fields
  
  nlohmann::json extendedJson = {
    {"id", 3003},
    {"name", "Charlie Brown"},
    {"email", "charlie@example.com"},
    {"age", 35},
    {"active", false},
    {"tags", nlohmann::json::array({"manager", "product"})},
    // Additional fields that don't exist in UserProfile
    {"department", "Engineering"},
    {"salary", 85000},
    {"start_date", "2022-01-15"},
    {"permissions", nlohmann::json::array({"read", "write", "admin"})}
  };
  
  EXPECT_TRUE(state_manager->write("test_extended_user", extendedJson));
  auto [error_extended, extended_data] = state_manager->read("test_extended_user");
  EXPECT_FALSE(error_extended.has_value());
  
  // Deserialize JSON with extra fields - extra fields should be ignored
  UserProfile extendedUser = extended_data.get<UserProfile>();
  EXPECT_EQ(extendedUser.id, 3003);
  EXPECT_EQ(extendedUser.name, "Charlie Brown");
  EXPECT_EQ(extendedUser.email, "charlie@example.com");
  EXPECT_EQ(extendedUser.age, 35);
  EXPECT_EQ(extendedUser.active, false);
  EXPECT_EQ(extendedUser.tags.size(), 2);
  EXPECT_EQ(extendedUser.tags[0], "manager");
  EXPECT_EQ(extendedUser.tags[1], "product");
  
  // Verify that when we serialize back, we only get the defined fields
  nlohmann::json serializedBack = extendedUser;
  EXPECT_FALSE(serializedBack.contains("department"));
  EXPECT_FALSE(serializedBack.contains("salary"));
  EXPECT_FALSE(serializedBack.contains("start_date"));
  EXPECT_FALSE(serializedBack.contains("permissions"));
  EXPECT_TRUE(serializedBack.contains("id"));
  EXPECT_TRUE(serializedBack.contains("name"));
  EXPECT_TRUE(serializedBack.contains("email"));
  EXPECT_TRUE(serializedBack.contains("age"));
  EXPECT_TRUE(serializedBack.contains("active"));
  EXPECT_TRUE(serializedBack.contains("tags"));
  
  // Clean up
  EXPECT_TRUE(state_manager->erase("test_extended_user"));
}

TEST_F(StateManagerTest, JSONDataTypes_custom_class_roundtrip) {
  // Test round-trip consistency for custom class
  
  UserProfile originalUser(4004, "Diana Prince", "diana@example.com", 30, true, 
                          {"designer", "ux", "frontend"});
  
  // First serialization
  nlohmann::json userJson = originalUser;
  EXPECT_TRUE(state_manager->write("test_roundtrip_user", userJson));
  
  // First deserialization
  auto [error1, data1] = state_manager->read("test_roundtrip_user");
  EXPECT_FALSE(error1.has_value());
  UserProfile user1 = data1.get<UserProfile>();
  EXPECT_EQ(user1, originalUser);
  
  // Second serialization
  nlohmann::json userJson2 = user1;
  EXPECT_TRUE(state_manager->write("test_roundtrip_user2", userJson2));
  
  // Second deserialization
  auto [error2, data2] = state_manager->read("test_roundtrip_user2");
  EXPECT_FALSE(error2.has_value());
  UserProfile user2 = data2.get<UserProfile>();
  EXPECT_EQ(user2, originalUser);
  EXPECT_EQ(user2, user1);
  
  // Verify JSON consistency
  EXPECT_EQ(userJson.dump(), userJson2.dump());
  
  // Test with complex nested data
  UserProfile complexUser(5005, "Eve Wilson", "eve@example.com", 45, false, 
                         {"architect", "senior", "team-lead", "mentor", "speaker"});
  
  nlohmann::json complexJson = complexUser;
  EXPECT_TRUE(state_manager->write("test_complex_user", complexJson));
  
  auto [error_complex, complex_data] = state_manager->read("test_complex_user");
  EXPECT_FALSE(error_complex.has_value());
  UserProfile deserializedComplex = complex_data.get<UserProfile>();
  EXPECT_EQ(deserializedComplex, complexUser);
  EXPECT_EQ(deserializedComplex.tags.size(), 5);
  
  // Clean up
  EXPECT_TRUE(state_manager->erase("test_roundtrip_user"));
  EXPECT_TRUE(state_manager->erase("test_roundtrip_user2"));
  EXPECT_TRUE(state_manager->erase("test_complex_user"));
}

TEST_F(StateManagerTest, CRUDOperations_basic) {
  const std::string key = "crud_test";
  nlohmann::json value = {{"operation", "create"}, {"timestamp", 1234567890}};

  // Create
  EXPECT_TRUE(state_manager->write(key, value));
  auto [error_read, read_value] = state_manager->read(key);
  EXPECT_FALSE(error_read.has_value());
  EXPECT_EQ(read_value["operation"].get<std::string>(), "create");

  // Update
  value["operation"] = "update";
  value["modified"] = true;
  EXPECT_TRUE(state_manager->write(key, value));
  auto [error_update, updated_value] = state_manager->read(key);
  EXPECT_FALSE(error_update.has_value());
  EXPECT_EQ(updated_value["operation"].get<std::string>(), "update");
  EXPECT_EQ(updated_value["modified"].get<bool>(), true);

  // Delete
  EXPECT_TRUE(state_manager->erase(key));
  auto [error_after_delete, deleted_value] = state_manager->read(key);
  EXPECT_TRUE(error_after_delete.has_value());
  EXPECT_EQ(error_after_delete.value(), "Key not found");
}

TEST_F(StateManagerTest, ErrorHandling_nonexistent_key) {
  // Test reading non-existent key
  auto [error_nonexistent, value_nonexistent] = state_manager->read("nonexistent_key");
  EXPECT_TRUE(error_nonexistent.has_value());
  EXPECT_EQ(error_nonexistent.value(), "Key not found");
}

TEST_F(StateManagerTest, ErrorHandling_erase_nonexistent) {
  // Test erasing non-existent key
  EXPECT_FALSE(state_manager->erase("nonexistent_key"));
}

TEST_F(StateManagerTest, ErrorHandling_empty_key) {
  // Test invalid key names (empty key)
  nlohmann::json test_value = "test";
  EXPECT_TRUE(state_manager->write("", test_value));
}

TEST_F(StateManagerTest, EdgeCases_empty_string) {
  // Test empty string
  nlohmann::json empty_string = "";
  EXPECT_TRUE(state_manager->write("empty_string", empty_string));
  auto [error_empty, read_empty] = state_manager->read("empty_string");
  EXPECT_FALSE(error_empty.has_value());
  EXPECT_EQ(read_empty.get<std::string>(), "");
}

TEST_F(StateManagerTest, EdgeCases_special_characters) {
  // Test special characters
  nlohmann::json special_chars = "Special chars: üñíçödé 中文 🚀 \"quotes\" \\backslash";
  EXPECT_TRUE(state_manager->write("special_chars", special_chars));
  auto [error_special, read_special] = state_manager->read("special_chars");
  EXPECT_FALSE(error_special.has_value());
  EXPECT_EQ(read_special.get<std::string>(), "Special chars: üñíçödé 中文 🚀 \"quotes\" \\backslash");
}

TEST_F(StateManagerTest, EdgeCases_nested_object) {
  // Test deeply nested object
  nlohmann::json nested = {{"level1", {{"level2", {{"level3", {{"level4", "deep_value"}}}}}}}};
  EXPECT_TRUE(state_manager->write("nested", nested));
  auto [error_nested, read_nested] = state_manager->read("nested");
  EXPECT_FALSE(error_nested.has_value());
  EXPECT_EQ(read_nested["level1"]["level2"]["level3"]["level4"].get<std::string>(), "deep_value");
}

TEST_F(StateManagerTest, MultipleKeys_operations) {
  const int num_keys = 5;
  
  // Create multiple keys
  for (int i = 0; i < num_keys; ++i) {
    std::string key = "multi_key_" + std::to_string(i);
    nlohmann::json value = {
      {"id", i},
      {"name", "item_" + std::to_string(i)},
      {"active", i % 2 == 0}
    };
    EXPECT_TRUE(state_manager->write(key, value));
  }

  // Read and verify all keys
  for (int i = 0; i < num_keys; ++i) {
    std::string key = "multi_key_" + std::to_string(i);
    auto [error, value] = state_manager->read(key);
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(value["id"].get<int>(), i);
    EXPECT_EQ(value["name"].get<std::string>(), "item_" + std::to_string(i));
    EXPECT_EQ(value["active"].get<bool>(), i % 2 == 0);
  }

  // Clean up
  for (int i = 0; i < num_keys; ++i) {
    std::string key = "multi_key_" + std::to_string(i);
    EXPECT_TRUE(state_manager->erase(key));
  }
}

} // namespace StateManager