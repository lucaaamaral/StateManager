#include "core/StateManager.h"
#include <iostream>
#include <iomanip>

using namespace StateManager;

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

void demonstrateBasicOperations() {
    printSeparator("BASIC CRUD OPERATIONS");
    
    try {
        // Create StateManager with default Redis configuration (localhost:6379)
        RedisConfig config; // Uses defaults
        StateManager stateManager(config);
        
        std::cout << "✓ StateManager initialized with default config" << std::endl;
        
        // 1. WRITE OPERATION
        std::cout << "\n1. Writing JSON data to Redis..." << std::endl;
        nlohmann::json userData = {
            {"id", 1001},
            {"name", "Alice Johnson"},
            {"email", "alice.johnson@example.com"},
            {"age", 28},
            {"active", true},
            {"preferences", {
                {"theme", "dark"},
                {"notifications", true},
                {"language", "en"}
            }},
            {"tags", {"developer", "team-lead", "python"}}
        };
        
        bool writeSuccess = stateManager.write("user:1001", userData);
        if (writeSuccess) {
            std::cout << "✓ Successfully wrote user data" << std::endl;
            std::cout << "  Data: " << userData.dump() << std::endl;
        } else {
            std::cout << "✗ Failed to write user data" << std::endl;
            return;
        }
        
        // 2. READ OPERATION
        std::cout << "\n2. Reading data from Redis..." << std::endl;
        auto [readError, readData] = stateManager.read("user:1001");
        if (!readError.has_value()) {
            std::cout << "✓ Successfully read user data:" << std::endl;
            std::cout << readData.dump(2) << std::endl;
            
            // Access specific fields
            std::cout << "\n  User details:" << std::endl;
            std::cout << "    Name: " << readData["name"].get<std::string>() << std::endl;
            std::cout << "    Email: " << readData["email"].get<std::string>() << std::endl;
            std::cout << "    Age: " << readData["age"].get<int>() << std::endl;
            std::cout << "    Active: " << (readData["active"].get<bool>() ? "Yes" : "No") << std::endl;
            std::cout << "    Theme: " << readData["preferences"]["theme"].get<std::string>() << std::endl;
        } else {
            std::cout << "✗ Failed to read user data: " << readError.value() << std::endl;
            return;
        }
        
        // 3. UPDATE OPERATION (overwrite with modified data)
        std::cout << "\n3. Updating user data..." << std::endl;
        userData["age"] = 29;
        userData["preferences"]["theme"] = "light";
        userData["last_login"] = "2024-08-05T15:30:00Z";
        userData["login_count"] = 47;
        
        bool updateSuccess = stateManager.write("user:1001", userData);
        if (updateSuccess) {
            std::cout << "✓ Successfully updated user data" << std::endl;
            
            // Verify the update
            auto [verifyError, verifyData] = stateManager.read("user:1001");
            if (!verifyError.has_value()) {
                std::cout << "  Updated age: " << verifyData["age"].get<int>() << std::endl;
                std::cout << "  Updated theme: " << verifyData["preferences"]["theme"].get<std::string>() << std::endl;
                std::cout << "  Last login: " << verifyData["last_login"].get<std::string>() << std::endl;
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
            auto [deleteVerifyError, deleteVerifyData] = stateManager.read("user:1001");
            if (deleteVerifyError.has_value()) {
                std::cout << "✓ Verified deletion - key no longer exists" << std::endl;
                std::cout << "  Error message: " << deleteVerifyError.value() << std::endl;
            } else {
                std::cout << "✗ Unexpected: key still exists after deletion" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to delete user data" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception in basic operations: " << e.what() << std::endl;
    }
}

void demonstrateDataTypes() {
    printSeparator("JSON DATA TYPES DEMO");
    
    try {
        RedisConfig config;
        StateManager stateManager(config);
        
        std::cout << "Testing different JSON data types..." << std::endl;
        
        // Integer
        nlohmann::json intData = 42;
        stateManager.write("test:integer", intData);
        auto [intError, intResult] = stateManager.read("test:integer");
        std::cout << "Integer: " << intResult.get<int>() << std::endl;
        
        // Float
        nlohmann::json floatData = 3.14159;
        stateManager.write("test:float", floatData);
        auto [floatError, floatResult] = stateManager.read("test:float");
        std::cout << "Float: " << std::fixed << std::setprecision(5) << floatResult.get<double>() << std::endl;
        
        // Boolean
        nlohmann::json boolData = true;
        stateManager.write("test:boolean", boolData);
        auto [boolError, boolResult] = stateManager.read("test:boolean");
        std::cout << "Boolean: " << (boolResult.get<bool>() ? "true" : "false") << std::endl;
        
        // String
        nlohmann::json stringData = "Hello, StateManager! 🚀";
        stateManager.write("test:string", stringData);
        auto [stringError, stringResult] = stateManager.read("test:string");
        std::cout << "String: " << stringResult.get<std::string>() << std::endl;
        
        // Array
        nlohmann::json arrayData = nlohmann::json::array({1, "two", 3.0, true, nullptr});
        stateManager.write("test:array", arrayData);
        auto [arrayError, arrayResult] = stateManager.read("test:array");
        std::cout << "Array: " << arrayResult.dump() << std::endl;
        
        // Object
        nlohmann::json objectData = {
            {"name", "StateManager"},
            {"version", "1.0"},
            {"features", nlohmann::json::array({"redis", "json", "pubsub"})},
            {"config", {
                {"host", "localhost"},
                {"port", 6379}
            }}
        };
        stateManager.write("test:object", objectData);
        auto [objError, objResult] = stateManager.read("test:object");
        std::cout << "Object:" << std::endl << objResult.dump(2) << std::endl;
        
        // Null
        nlohmann::json nullData = nullptr;
        stateManager.write("test:null", nullData);
        auto [nullError, nullResult] = stateManager.read("test:null");
        std::cout << "Null: " << (nullResult.is_null() ? "null" : "not null") << std::endl;
        
        // Cleanup
        std::cout << "\nCleaning up test data..." << std::endl;
        stateManager.erase("test:integer");
        stateManager.erase("test:float");
        stateManager.erase("test:boolean");
        stateManager.erase("test:string");
        stateManager.erase("test:array");
        stateManager.erase("test:object");
        stateManager.erase("test:null");
        std::cout << "✓ Test data cleaned up" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception in data types demo: " << e.what() << std::endl;
    }
}

void demonstrateErrorHandling() {
    printSeparator("ERROR HANDLING DEMO");
    
    try {
        RedisConfig config;
        StateManager stateManager(config);
        
        std::cout << "Testing error conditions..." << std::endl;
        
        // 1. Reading non-existent key
        std::cout << "\n1. Reading non-existent key..." << std::endl;
        auto [error1, data1] = stateManager.read("nonexistent:key");
        if (error1.has_value()) {
            std::cout << "✓ Expected error: " << error1.value() << std::endl;
        } else {
            std::cout << "✗ Unexpected success reading non-existent key" << std::endl;
        }
        
        // 2. Erasing non-existent key
        std::cout << "\n2. Erasing non-existent key..." << std::endl;
        bool eraseResult = stateManager.erase("nonexistent:key");
        if (!eraseResult) {
            std::cout << "✓ Expected failure when erasing non-existent key" << std::endl;
        } else {
            std::cout << "✗ Unexpected success erasing non-existent key" << std::endl;
        }
        
        // 3. Invalid key (empty string)
        std::cout << "\n3. Using empty key..." << std::endl;
        nlohmann::json testData = "test";
        bool emptyKeyResult = stateManager.write("", testData);
        if (!emptyKeyResult) {
            std::cout << "✓ Expected failure with empty key" << std::endl;
        } else {
            std::cout << "✗ Unexpected success with empty key" << std::endl;
        }
        
        // 4. Large data handling
        std::cout << "\n4. Testing large data handling..." << std::endl;
        nlohmann::json largeData;
        for (int i = 0; i < 1000; ++i) {
            largeData["item_" + std::to_string(i)] = {
                {"id", i},
                {"name", "Item " + std::to_string(i)},
                {"description", "This is a description for item " + std::to_string(i)},
                {"active", i % 2 == 0}
            };
        }
        
        bool largeWriteResult = stateManager.write("test:large", largeData);
        if (largeWriteResult) {
            std::cout << "✓ Successfully wrote large data (" << largeData.size() << " items)" << std::endl;
            
            auto [largeReadError, largeReadData] = stateManager.read("test:large");
            if (!largeReadError.has_value()) {
                std::cout << "✓ Successfully read large data back" << std::endl;
                std::cout << "  Verified size: " << largeReadData.size() << " items" << std::endl;
                stateManager.erase("test:large");
            }
        } else {
            std::cout << "✗ Failed to write large data" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception in error handling demo: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "StateManager Library - Basic Demo" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "This demo showcases the core functionality of the StateManager library." << std::endl;
    std::cout << "Make sure Redis is running on localhost:6379 before running this demo." << std::endl;
    
    // Run the demonstrations
    demonstrateBasicOperations();
    demonstrateDataTypes();
    demonstrateErrorHandling();
    
    printSeparator("DEMO COMPLETED");
    std::cout << "All demonstrations completed successfully!" << std::endl;
    std::cout << "Check the output above for results of each operation." << std::endl;
    
    return 0;
}