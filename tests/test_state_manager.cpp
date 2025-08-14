#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "client/RedisConfig.h"
#include "core/StateManager.h"
#include "logging/LoggerFactory.h"
#include "return/StateObj.h"

namespace StateManager {

// Test class for StateObj serialization
class UserProfile : public StateObj {
public:
  int id;
  std::string name;
  std::string email;
  int age;
  bool active;
  std::vector<std::string> tags;

  // Default constructor for deserialization
  UserProfile() = default;

  // Constructor with parameters
  UserProfile(int id, const std::string &name, const std::string &email,
              int age, bool active, const std::vector<std::string> &tags)
      : id(id), name(name), email(email), age(age), active(active), tags(tags) {
  }

  // Equality operator for testing
  bool operator==(const UserProfile &other) const {
    return id == other.id && name == other.name && email == other.email &&
           age == other.age && active == other.active && tags == other.tags;
  }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};

// Define StateObj serialization for UserProfile
STATE_OBJ_DEFINE_TYPE(UserProfile, id, name, email, age, active, tags)

// Simple test classes for basic data types
class StringState : public StateObj {
public:
  std::string value;
  StringState() = default;
  StringState(const std::string &val) : value(val) {}
  bool operator==(const StringState &other) const {
    return value == other.value;
  }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};
STATE_OBJ_DEFINE_TYPE(StringState, value)

class IntState : public StateObj {
public:
  int value;
  IntState() = default;
  IntState(int val) : value(val) {}
  bool operator==(const IntState &other) const { return value == other.value; }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};
STATE_OBJ_DEFINE_TYPE(IntState, value)

class FloatState : public StateObj {
public:
  double value;
  FloatState() = default;
  FloatState(double val) : value(val) {}
  bool operator==(const FloatState &other) const {
    return value == other.value;
  }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};
STATE_OBJ_DEFINE_TYPE(FloatState, value)

class BoolState : public StateObj {
public:
  bool value;
  BoolState() = default;
  BoolState(bool val) : value(val) {}
  bool operator==(const BoolState &other) const { return value == other.value; }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};
STATE_OBJ_DEFINE_TYPE(BoolState, value)

class ArrayState : public StateObj {
public:
  std::vector<int> numbers;
  std::vector<std::string> strings;
  ArrayState() = default;
  bool operator==(const ArrayState &other) const {
    return numbers == other.numbers && strings == other.strings;
  }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};
STATE_OBJ_DEFINE_TYPE(ArrayState, numbers, strings)

class ObjectState : public StateObj {
public:
  std::string name;
  double version;
  bool active;
  std::vector<std::string> tags;
  ObjectState() = default;
  bool operator==(const ObjectState &other) const {
    return name == other.name && version == other.version &&
           active == other.active && tags == other.tags;
  }

  // StateObj virtual method declarations
  nlohmann::json to_json() const override;
  void from_json(const nlohmann::json &j) override;
  std::unique_ptr<::StateManager::StateObj> clone() const override;
};
STATE_OBJ_DEFINE_TYPE(ObjectState, name, version, active, tags)

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

  void TearDown() override { state_manager.reset(); }
};

// Test basic StateObj write and read operations
TEST_F(StateManagerTest, BasicStateObjOperations) {
  // Test string
  StringState original_string("Hello, StateManager!");
  EXPECT_TRUE(state_manager->write("test_string", original_string));

  auto [error, read_obj] = state_manager->read("test_string", StringState{});
  EXPECT_FALSE(error.has_value())
      << "Error reading string: " << error.value_or("");
  EXPECT_NE(read_obj, nullptr);

  StringState *read_string = dynamic_cast<StringState *>(read_obj.get());
  EXPECT_NE(read_string, nullptr);
  EXPECT_EQ(read_string->value, "Hello, StateManager!");
  EXPECT_EQ(*read_string, original_string);

  logger->info("StateManager StateObj test passed successfully");
}

// Test different StateObj data types
TEST_F(StateManagerTest, StateObjDataTypes_int) {
  // Test integer
  IntState original_int(42);
  EXPECT_TRUE(state_manager->write("test_int", original_int));

  auto [error_int, read_int] = state_manager->read("test_int", IntState{});
  EXPECT_FALSE(error_int.has_value());
  EXPECT_NE(read_int, nullptr);

  IntState *int_state = dynamic_cast<IntState *>(read_int.get());
  EXPECT_NE(int_state, nullptr);
  EXPECT_EQ(int_state->value, 42);
  EXPECT_EQ(*int_state, original_int);
}

TEST_F(StateManagerTest, StateObjDataTypes_float) {
  // Test float
  FloatState original_float(3.14159);
  EXPECT_TRUE(state_manager->write("test_float", original_float));

  FloatState template_float;
  auto [error_float, read_float] =
      state_manager->read("test_float", template_float);
  EXPECT_FALSE(error_float.has_value());
  EXPECT_NE(read_float, nullptr);

  FloatState *float_state = dynamic_cast<FloatState *>(read_float.get());
  EXPECT_NE(float_state, nullptr);
  EXPECT_DOUBLE_EQ(float_state->value, 3.14159);
  EXPECT_EQ(*float_state, original_float);
}

TEST_F(StateManagerTest, StateObjDataTypes_bool) {
  // Test boolean
  BoolState original_bool(true);
  EXPECT_TRUE(state_manager->write("test_bool", original_bool));

  BoolState template_bool;
  auto [error_bool, read_bool] =
      state_manager->read("test_bool", template_bool);
  EXPECT_FALSE(error_bool.has_value());
  EXPECT_NE(read_bool, nullptr);

  BoolState *bool_state = dynamic_cast<BoolState *>(read_bool.get());
  EXPECT_NE(bool_state, nullptr);
  EXPECT_EQ(bool_state->value, true);
  EXPECT_EQ(*bool_state, original_bool);
}

TEST_F(StateManagerTest, StateObjDataTypes_array) {
  // Test array using ArrayState
  ArrayState original_array;
  original_array.numbers = {1, 2, 3};
  original_array.strings = {"test", "array"};

  EXPECT_TRUE(state_manager->write("test_array", original_array));

  auto [error_array, read_array] =
      state_manager->read("test_array", ArrayState{});
  EXPECT_FALSE(error_array.has_value());
  EXPECT_NE(read_array, nullptr);

  ArrayState *array_state = dynamic_cast<ArrayState *>(read_array.get());
  EXPECT_NE(array_state, nullptr);
  EXPECT_EQ(array_state->numbers.size(), 3);
  EXPECT_EQ(array_state->numbers[0], 1);
  EXPECT_EQ(array_state->numbers[2], 3);
  EXPECT_EQ(array_state->strings.size(), 2);
  EXPECT_EQ(array_state->strings[0], "test");
  EXPECT_EQ(array_state->strings[1], "array");
  EXPECT_EQ(*array_state, original_array);
}

TEST_F(StateManagerTest, StateObjDataTypes_object) {
  // Test object using ObjectState
  ObjectState original_object;
  original_object.name = "StateManager";
  original_object.version = 1.0;
  original_object.active = true;
  original_object.tags = {"redis", "cpp", "json"};

  EXPECT_TRUE(state_manager->write("test_object", original_object));

  auto [error_object, read_object] =
      state_manager->read("test_object", ObjectState{});
  EXPECT_FALSE(error_object.has_value());
  EXPECT_NE(read_object, nullptr);

  ObjectState *object_state = dynamic_cast<ObjectState *>(read_object.get());
  EXPECT_NE(object_state, nullptr);
  EXPECT_EQ(object_state->name, "StateManager");
  EXPECT_DOUBLE_EQ(object_state->version, 1.0);
  EXPECT_EQ(object_state->active, true);
  EXPECT_EQ(object_state->tags.size(), 3);
  EXPECT_EQ(object_state->tags[0], "redis");
  EXPECT_EQ(object_state->tags[1], "cpp");
  EXPECT_EQ(object_state->tags[2], "json");
  EXPECT_EQ(*object_state, original_object);
}

TEST_F(StateManagerTest, StateObjDataTypes_custom_class_basic) {
  // Test basic custom class serialization with StateObj

  // Create a UserProfile object
  UserProfile originalUser(1001, "Alice Smith", "alice@example.com", 28, true,
                           {"developer", "team-lead", "cpp"});

  // Write to StateManager using StateObj interface
  EXPECT_TRUE(state_manager->write("test_user_profile", originalUser));

  // Read back and verify
  auto [error_read, read_obj] =
      state_manager->read("test_user_profile", UserProfile{});
  EXPECT_FALSE(error_read.has_value());
  EXPECT_NE(read_obj, nullptr);

  // Convert back to UserProfile object
  UserProfile *deserializedUser = dynamic_cast<UserProfile *>(read_obj.get());
  EXPECT_NE(deserializedUser, nullptr);
  EXPECT_EQ(*deserializedUser, originalUser);

  // Verify individual fields
  EXPECT_EQ(deserializedUser->id, 1001);
  EXPECT_EQ(deserializedUser->name, "Alice Smith");
  EXPECT_EQ(deserializedUser->email, "alice@example.com");
  EXPECT_EQ(deserializedUser->age, 28);
  EXPECT_EQ(deserializedUser->active, true);
  EXPECT_EQ(deserializedUser->tags.size(), 3);
  EXPECT_EQ(deserializedUser->tags[0], "developer");
  EXPECT_EQ(deserializedUser->tags[1], "team-lead");
  EXPECT_EQ(deserializedUser->tags[2], "cpp");

  // Clean up
  EXPECT_TRUE(state_manager->erase("test_user_profile"));
}

TEST_F(StateManagerTest, StateObjDataTypes_custom_class_roundtrip) {
  // Test round-trip consistency for custom class using StateObj

  UserProfile originalUser(4004, "Diana Prince", "diana@example.com", 30, true,
                           {"designer", "ux", "frontend"});

  // First write and read
  EXPECT_TRUE(state_manager->write("test_roundtrip_user", originalUser));

  auto [error1, data1] =
      state_manager->read("test_roundtrip_user", UserProfile{});
  EXPECT_FALSE(error1.has_value());
  EXPECT_NE(data1, nullptr);

  UserProfile *user1 = dynamic_cast<UserProfile *>(data1.get());
  EXPECT_NE(user1, nullptr);
  EXPECT_EQ(*user1, originalUser);

  // Second write and read
  EXPECT_TRUE(state_manager->write("test_roundtrip_user2", *user1));

  auto [error2, data2] =
      state_manager->read("test_roundtrip_user2", UserProfile{});
  EXPECT_FALSE(error2.has_value());
  EXPECT_NE(data2, nullptr);

  UserProfile *user2 = dynamic_cast<UserProfile *>(data2.get());
  EXPECT_NE(user2, nullptr);
  EXPECT_EQ(*user2, originalUser);
  EXPECT_EQ(*user2, *user1);

  // Clean up
  EXPECT_TRUE(state_manager->erase("test_roundtrip_user"));
  EXPECT_TRUE(state_manager->erase("test_roundtrip_user2"));
}

TEST_F(StateManagerTest, CRUDOperations_basic) {
  const std::string key = "crud_test";

  // Create a state object for CRUD operations
  ObjectState original_state;
  original_state.name = "create";
  original_state.version = 1.0;
  original_state.active = true;
  original_state.tags = {"test"};

  // Create
  EXPECT_TRUE(state_manager->write(key, original_state));
  auto [error_read, read_value] = state_manager->read(key, ObjectState{});
  EXPECT_FALSE(error_read.has_value());
  EXPECT_NE(read_value, nullptr);

  ObjectState *read_state = dynamic_cast<ObjectState *>(read_value.get());
  EXPECT_NE(read_state, nullptr);
  EXPECT_EQ(read_state->name, "create");

  // Update
  original_state.name = "update";
  original_state.active = false;
  EXPECT_TRUE(state_manager->write(key, original_state));

  auto [error_update, updated_value] = state_manager->read(key, ObjectState{});
  EXPECT_FALSE(error_update.has_value());
  EXPECT_NE(updated_value, nullptr);

  ObjectState *updated_state = dynamic_cast<ObjectState *>(updated_value.get());
  EXPECT_NE(updated_state, nullptr);
  EXPECT_EQ(updated_state->name, "update");
  EXPECT_EQ(updated_state->active, false);

  // Delete
  EXPECT_TRUE(state_manager->erase(key));
  auto [error_after_delete, deleted_value] =
      state_manager->read(key, ObjectState{});
  EXPECT_TRUE(error_after_delete.has_value());
  EXPECT_EQ(error_after_delete.value(), "Key not found");
}

TEST_F(StateManagerTest, ErrorHandling_nonexistent_key) {
  // Test reading non-existent key
  auto [error_nonexistent, value_nonexistent] =
      state_manager->read("nonexistent_key", StringState{});
  EXPECT_TRUE(error_nonexistent.has_value());
  EXPECT_EQ(error_nonexistent.value(), "Key not found");
}

TEST_F(StateManagerTest, ErrorHandling_erase_nonexistent) {
  // Test erasing non-existent key
  EXPECT_FALSE(state_manager->erase("nonexistent_key"));
}

TEST_F(StateManagerTest, ErrorHandling_empty_key) {
  // Test invalid key names (empty key) with StateObj
  StringState test_value("test");
  EXPECT_TRUE(state_manager->write("", test_value));
}

TEST_F(StateManagerTest, EdgeCases_empty_string) {
  // Test empty string using StringState
  StringState empty_string("");
  EXPECT_TRUE(state_manager->write("empty_string", empty_string));

  auto [error_empty, read_empty] =
      state_manager->read("empty_string", StringState{});
  EXPECT_FALSE(error_empty.has_value());
  EXPECT_NE(read_empty, nullptr);

  StringState *string_state = dynamic_cast<StringState *>(read_empty.get());
  EXPECT_NE(string_state, nullptr);
  EXPECT_EQ(string_state->value, "");
}

TEST_F(StateManagerTest, EdgeCases_special_characters) {
  // Test special characters using StringState
  StringState special_chars(
      "Special chars: üñíçödé 中文 🚀 \"quotes\" \\backslash");
  EXPECT_TRUE(state_manager->write("special_chars", special_chars));

  auto [error_special, read_special] =
      state_manager->read("special_chars", StringState{});
  EXPECT_FALSE(error_special.has_value());
  EXPECT_NE(read_special, nullptr);

  StringState *string_state = dynamic_cast<StringState *>(read_special.get());
  EXPECT_NE(string_state, nullptr);
  EXPECT_EQ(string_state->value,
            "Special chars: üñíçödé 中文 🚀 \"quotes\" \\backslash");
}

} // namespace StateManager