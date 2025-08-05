# StateManager Library

A C++17 library for managing JSON state data in Redis with integrated publish/subscribe notifications and dynamic thread pool management.

## Features

- **CRUD Operations**: Write, Read, Erase operations for JSON data
- **Redis Integration**: Uses Redis as the storage backend with thread-safe operations
- **Pub/Sub System**: Full publish/subscribe functionality with Redis channels
- **Thread Safety**: Comprehensive thread safety with atomic operations and mutex-based locking
- **JSON Support**: Native JSON serialization/deserialization using nlohmann/json
- **Dynamic ThreadPool**: Scalable thread pool for asynchronous callback processing
- **Comprehensive Logging**: Integrated logging system with configurable levels
- **Connection Management**: Singleton Redis client with flexible configuration

## Key Dependencies

- **C++17** compatible compiler
- **CMake 3.10+** build system
- **Redis server** for storage and pub/sub
- **redis-plus-plus** - Modern C++ Redis client (statically linked)
- **hiredis** - Low-level Redis client library
- **nlohmann/json** - JSON parsing and serialization
- **GoogleTest** - Testing framework (tests only)

## Installation

### Prerequisites

Ensure Redis server is running:
```bash
redis-server
```

### Building the Library

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

### Running Tests

```bash
# Ensure Redis is running on localhost:6379
cmake -DBUILD_TESTING=ON ..
cmake --build . --target test
```

### Running Examples

```bash
# Build examples
cmake -DBUILD_EXAMPLES=ON ..
cmake --build .

# Basic CRUD operations demo
./bin/basic_demo

# Multi-threaded concurrency and load testing demo
./bin/multithreaded_demo

# Publish/subscribe functionality demo
./bin/pub_sub_demo
```

## Quick Start

### Basic Usage

```cpp
#include "core/StateManager.h"
using namespace StateManager;

// Create state manager with default configuration (localhost:6379)
RedisConfig config; // Uses defaults: localhost:6379
StateManager stateManager(config);

// Create JSON state
nlohmann::json userData = {
    {"id", 1},
    {"name", "Alice"},
    {"email", "alice@example.com"}
};

// Write state
bool success = stateManager.write("user:1", userData);
if (success) {
    std::cout << "User created successfully!" << std::endl;
}

// Read state
auto [error, data] = stateManager.read("user:1");
if (!error.has_value()) {
    std::cout << "User data: " << data.dump(2) << std::endl;
} else {
    std::cout << "Error: " << error.value() << std::endl;
}

// Update state
userData["last_login"] = "2024-07-14T18:00:00Z";
stateManager.write("user:1", userData);  // Overwrites existing data

// Delete state
stateManager.erase("user:1");
```

### Custom Redis Configuration

```cpp
RedisConfig config;
config.host = "redis.example.com";
config.port = 6380;
config.password = "secret";
config.database = 1;

StateManager stateManager(config);
```

### Pub/Sub Operations

```cpp
// Subscribe to channel notifications
stateManager.subscribe("user_updates", 
    [](const nlohmann::json& message) {
        std::cout << "Received notification: " << message.dump() << std::endl;
    });

// Publish notifications
nlohmann::json notification = {
    {"event", "user_created"},
    {"user_id", 123},
    {"timestamp", "2024-07-14T18:00:00Z"}
};
stateManager.publish("user_updates", notification);

// Unsubscribe from channel
stateManager.unsubscribe("user_updates");
```

## API Reference

### StateManager Class

#### Core Operations

- `bool write(const std::string& key, const nlohmann::json& value)`
  - Writes JSON data to Redis
  - Returns true on success, false on failure
  - Overwrites existing data if key exists

- `std::pair<error, nlohmann::json> read(const std::string& key)`
  - Reads JSON data from Redis
  - Returns pair with optional error and JSON data
  - Error is `std::nullopt` on success, contains message on failure

- `bool erase(const std::string& key)`
  - Deletes data from Redis
  - Returns true on success, false on failure

#### Pub/Sub Operations

- `void subscribe(const std::string& channel, std::function<void(const nlohmann::json&)> callback)`
  - Subscribe to Redis channel notifications
  - Callback receives deserialized JSON messages
  - Multiple callbacks can be registered per channel

- `void unsubscribe(const std::string& channel)`
  - Unsubscribe from Redis channel notifications
  - Removes all callbacks for the specified channel

- `bool publish(const std::string& channel, const nlohmann::json& data)`
  - Publish JSON data to Redis channel
  - Returns true on success, false on failure
  - Data is automatically serialized to JSON string

#### Configuration

- `static void setConfig(const RedisConfig& config)`
  - Configure Redis connection settings globally
  - Applied to all future connections

### Type Definitions

```cpp
using error = std::optional<std::string>;
using json = nlohmann::json;
```

### RedisConfig Structure

```cpp
struct RedisConfig {
    std::string host = "127.0.0.1";
    int port = 6379;
    std::string password = "";
    int database = 0;
    int timeout_ms = 1000;
};
```

## Thread Safety

The library is designed for concurrent access:

- **Storage Operations**: Protected by Redis-level locking using SETNX
- **Manager Operations**: Protected by mutex for create/update/delete operations
- **Read Operations**: Lock-free for better performance
- **Notifications**: Thread-safe pub/sub handling

## Error Handling

All operations return `StateResult` objects with:
- Success/failure status
- Descriptive error messages
- Result data (when applicable)

Common error scenarios:
- Redis connection failures
- Key already exists (create operation)
- Key not found (read/update/delete operations)
- JSON serialization/deserialization errors
- Lock acquisition failures

## Performance Considerations

- **Connection Pooling**: Each StateManager instance maintains its own Redis connection
- **Locking Overhead**: Write operations use Redis locks for consistency
- **JSON Serialization**: Automatic conversion between JSON and string representation
- **Pub/Sub Latency**: Notifications are asynchronous with minimal latency

## Examples

The `examples/` directory contains comprehensive demonstrations of the StateManager library:

### [basic_demo.cpp](examples/basic_demo.cpp)
**Complete CRUD operations showcase**
- Write, read, and erase operations with various JSON data types
- Error handling demonstrations
- Data integrity verification
- Large data handling

### [multithreaded_demo.cpp](examples/multithreaded_demo.cpp) 
**Thread safety and concurrency testing**
- Concurrent write operations from multiple threads
- Shared data access patterns with readers and writers
- Load testing with mixed operations
- Performance metrics and throughput analysis

### [pub_sub_demo.cpp](examples/pub_sub_demo.cpp)
**Publish/subscribe functionality showcase**
- Basic channel subscriptions and message publishing
- Multiple channel management
- Multiple subscribers on same channel
- State change notification patterns

Each example is self-contained and includes detailed console output to demonstrate the functionality.

## Testing

The library includes comprehensive tests covering all components and functionality:

### Test Categories

- **Unit Tests**: Individual component testing (RedisClient, ThreadPool, Logger)
- **Integration Tests**: Redis connectivity and pub/sub functionality  
- **Data Type Tests**: All JSON data types (int, float, bool, null, array, object)
- **CRUD Tests**: Complete lifecycle testing (write, read, erase)
- **Error Handling**: Non-existent keys, connection failures, edge cases
- **Thread Safety**: Concurrent operations and stress testing

### Test Organization

```
tests/
├── test_logger.cpp          # Logging system tests
├── test_redis_client.cpp    # Redis client connection tests
├── test_redis_channel.cpp   # Pub/sub functionality tests
├── test_threadpool.cpp      # ThreadPool implementation tests
└── test_state_manager.cpp   # Core StateManager functionality (15 test cases)
```

### Running Tests

```bash
# Build with testing enabled
mkdir -p build && cd build
cmake -DBUILD_TESTING=ON ..
cmake --build .

# Run all tests
./bin/StateManagerTests

# Run specific test suites
./bin/StateManagerTests --gtest_filter="StateManagerTest.*"
./bin/StateManagerTests --gtest_filter="ThreadPoolTest.*"

# Run specific test case
./bin/StateManagerTests --gtest_filter="*JSONDataTypes_int*"
```

For detailed testing information, see [TESTING.md](docs/TESTING.md).

### Running Tests with Docker Compose

The project includes a Docker Compose setup for testing with Redis. This is the recommended approach for running tests as it ensures a consistent environment with all dependencies.

```bash
# Start the testing environment
docker-compose up test

# This will:
# 1. Start a Redis container with the proper configuration
# 2. Build the StateManager library
# 3. Build and run the tests
```

#### Rebuilding After Code Changes

When you make changes to the code, you need to rebuild the Docker image to test your changes:

```bash
# Rebuild the test image
docker-compose build test

# Run tests with the updated code
docker-compose up test
```

For faster development iterations, you can use the development environment:

```bash
# Start the development environment
docker-compose up -d redis
docker-compose run --rm dev

# Inside the container, you can build and run tests
mkdir -p build && cd build
cmake -DBUILD_TESTING=ON ..
make
make test
```

This approach mounts your local code as a volume, so you can edit files locally and rebuild quickly inside the container without having to rebuild the entire Docker image.

## Contributing

1. Follow existing code style and patterns
2. Add tests for new functionality
3. Update documentation for API changes
4. Ensure all tests pass before submitting

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)**: Complete architectural overview with component relationships, layered design, and thread safety details
- **[TESTING.md](docs/TESTING.md)**: Comprehensive testing documentation covering test categories, scenarios, and execution guidelines  
- **[THREAD_POOL.md](docs/THREAD_POOL.md)**: Deep dive into the ThreadPool implementation with scaling policies, worker management, and performance characteristics
- **[BUILD_GUIDE.md](docs/BUILD_GUIDE.md)**: Build system instructions and setup
- **[LOGGING.md](docs/LOGGING.md)**: Logging system usage and configuration
- **[CMAKE.md](docs/CMAKE.md)**: CMake system documentation

## Architecture Overview

The StateManager library implements a layered architecture:

```
Application Layer:    StateManager (main API)
                           │
Service Layer:        RedisStorage, RedisChannel, Logging
                           │  
Infrastructure:       RedisClient, ThreadPool, RedisConfig
                           │
Dependencies:         redis-plus-plus, nlohmann/json, hiredis
```

Key architectural features:
- **Thread Safety**: Comprehensive synchronization with atomic operations
- **Dynamic Scaling**: ThreadPool scales workers based on queue load
- **Error Resilience**: Robust error handling and logging throughout
- **Resource Management**: RAII patterns and graceful shutdown

## License

See the LICENSE file in the project root for license information.
