# RedisStateManager Library Architecture

## Overview
The RedisStateManager is a C++17 library designed for managing JSON state data in Redis, with integrated publish/subscribe (pub/sub) notifications for state changes. It provides a thread-safe, efficient way to perform CRUD (Create, Read, Update, Delete) operations on state data while ensuring real-time notifications for distributed systems.

## Core Components

### 1. StateManager Class (`src/core/StateManager.cpp`, `include/core/StateManager.h`)
- **Purpose**: The central interface for interacting with state data, providing methods for CRUD operations, subscription to notifications, and connection management.
- **Key Features**:
  - CRUD operations: `write()`, `read()`, `erase()`.
  - Pub/sub operations: `subscribe()`, `unsubscribe()`, `publish()`.
  - Configuration management: `setConfig()`.
- **Thread Safety**: Thread-safe operations with mutex-based protection for writes and lock-free reads for performance.
- **Error Handling**: Returns `std::pair<error, json>` for read operations and boolean success indicators for write operations.

### 2. RedisStorage (`src/client/RedisStorage.cpp`, `include/client/RedisStorage.h`)
- **Purpose**: Handles the interaction with Redis as the underlying storage system for state data.
- **Key Features**:
  - Stores JSON data as strings in Redis using redis-plus-plus library.
  - Implements atomic operations using Redis SETNX for locking mechanisms.
  - Handles JSON serialization/deserialization with nlohmann/json.
  - Thread-safe operations with proper connection management.
- **Dependencies**: Uses redis-plus-plus library for Redis connectivity.

### 3. RedisChannel (`src/client/RedisChannel.cpp`, `include/client/RedisChannel.h`)
- **Purpose**: Singleton pub/sub notification system using Redis channels.
- **Key Features**:
  - Thread-safe subscriber management with atomic flags.
  - Uses internal ThreadPool for processing callbacks asynchronously.
  - Supports channel-based publish/subscribe operations.
  - Automatic JSON serialization/deserialization for pub/sub messages.
- **Thread Safety**: Atomic flags and thread-safe subscriber management.

### 4. RedisClient (`src/client/RedisClient.cpp`, `include/client/RedisClient.h`)
- **Purpose**: Singleton Redis connection manager with thread-safe client access.
- **Key Features**:
  - Centralized Redis connection management using redis-plus-plus.
  - Thread-safe client access with locking mechanisms.
  - Connection configuration and status monitoring.
  - Provides `lockClient()` pattern for thread-safe Redis operations.

### 5. ThreadPool (`src/thread/ThreadPool.cpp`, `include/thread/ThreadPool.h`)
- **Purpose**: Custom thread pool with dynamic scaling for asynchronous task processing.
- **Key Features**:
  - Dynamic scaling based on queue size (scale up when `queueSize > 10 * workerCount`).
  - Management thread handles worker lifecycle separately from worker threads.
  - Proper shutdown logic waits for all queued tasks to complete.
  - Configurable scaling policies through lambda functions.
- **Thread Safety**: Atomic worker management with proper shutdown synchronization.

### 6. Logging System (`src/logging/`, `include/logging/`)
- **Purpose**: Factory pattern logging with configurable levels and thread safety.
- **Key Features**:
  - `LoggerFactory` provides singleton logger instances.
  - `DefaultLogger` writes to stdout with configurable log levels.
  - Thread-safe logging throughout the library.
  - Per-component logging with contextual information.

### 7. RedisConfig Structure (`include/client/RedisConfig.h`)
- **Purpose**: Configuration structure for Redis connections.
- **Key Features**:
  - Customizable host, port, password, database, and timeout settings.
  - Default configuration connects to `localhost:6379`.
  - Used by RedisClient for connection initialization.

## Component Relationships

### Architecture Diagram
Below is a layered architecture diagram showing the component relationships and data flow:

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                        │
│                     ┌─────────────────┐                        │
│                     │  StateManager   │                        │
│                     └─────────────────┘                        │
└─────────────────────────────────────────────────────────────────┘
                               │
┌─────────────────────────────────────────────────────────────────┐
│                      Service Layer                             │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────┐   │
│  │ RedisStorage │  │ RedisChannel │  │   Logging System    │   │
│  └──────────────┘  └──────────────┘  └─────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                               │
┌─────────────────────────────────────────────────────────────────┐
│                   Infrastructure Layer                         │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────┐   │
│  │ RedisClient  │  │  ThreadPool  │  │    RedisConfig      │   │
│  │ (Singleton)  │  │              │  │                     │   │
│  └──────────────┘  └──────────────┘  └─────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                               │
┌─────────────────────────────────────────────────────────────────┐
│                     External Dependencies                      │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────┐   │
│  │redis-plus-plus│  │nlohmann/json │  │      hiredis        │   │
│  └──────────────┘  └──────────────┘  └─────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### Key Relationships

- **StateManager → RedisStorage**: Direct dependency for all CRUD operations (write, read, erase).
- **StateManager → RedisChannel**: Direct dependency for pub/sub operations (subscribe, unsubscribe, publish).
- **StateManager → LoggerFactory**: Uses logging system for operational logging.
- **RedisStorage → RedisClient**: Uses singleton client for Redis connections with `lockClient()` pattern.
- **RedisChannel → RedisClient**: Uses singleton client for pub/sub Redis connections.
- **RedisChannel → ThreadPool**: Uses thread pool for asynchronous callback processing.
- **RedisClient → RedisConfig**: Configured via RedisConfig structure.
- **All Components → Logging**: Integrated logging throughout the system using LoggerFactory pattern.

## Usage Flow

1. **Initialization**: A user creates a `StateManager` instance with either default or custom `RedisConfig` settings to connect to a Redis server.
2. **State Operations**:
   - **Write**: The user calls `write(key, json_value)`. The `StateManager` serializes the JSON, stores it in Redis via `RedisStorage`, and returns a boolean success indicator.
   - **Read**: The user calls `read(key)`. The `StateManager` retrieves the data from Redis, deserializes it, and returns `std::pair<error, json>` where error is `std::nullopt` on success.
   - **Erase**: The user calls `erase(key)` to remove data from Redis, returning a boolean success indicator.
3. **Pub/Sub Operations**:
   - **Subscribe**: Users call `subscribe(channel, callback)` to register for notifications on specific channels.
   - **Publish**: Users call `publish(channel, json_data)` to send notifications to subscribed clients.
   - **Unsubscribe**: Users call `unsubscribe(channel)` to stop receiving notifications.
4. **Error Handling**: Read operations return optional error messages, while write operations return boolean success indicators. All Redis connection errors are logged.

## Thread Safety and Performance

### Thread Safety Design
- **Storage operations**: Protected by Redis-level SETNX operations for atomicity.
- **Manager operations**: Mutex protection for create/update/delete operations.
- **Read operations**: Lock-free for performance optimization.
- **Pub/sub**: Atomic flags and thread-safe subscriber management.
- **ThreadPool**: Atomic worker management with proper shutdown synchronization.

### Performance Considerations
- **Redis connections**: Each component maintains its own connection for thread safety.
- **JSON handling**: Automatic conversion between JSON objects and Redis string storage.
- **ThreadPool scaling**: Dynamic scaling based on queue size with configurable policies.
- **Memory management**: RAII with smart pointers, no manual memory management.
- **Asynchronous processing**: ThreadPool handles pub/sub callbacks asynchronously.

## Key Dependencies

### Core Dependencies
- **redis-plus-plus**: Modern C++ Redis client (statically linked)
- **hiredis**: Low-level Redis client library 
- **nlohmann/json**: JSON parsing and serialization
- **GoogleTest**: Testing framework (tests only)

### Thread Safety Dependencies
- **std::atomic**: For atomic operations in ThreadPool and RedisChannel
- **std::mutex**: For thread-safe operations in StateManager
- **Redis SETNX**: For distributed locking mechanisms

## Testing Strategy

The architecture supports comprehensive testing through:
- **Unit tests**: Individual component testing (RedisClient, ThreadPool, Logger)
- **Integration tests**: Redis connectivity and pub/sub functionality
- **Thread safety tests**: Concurrent operations and stress testing
- **Performance tests**: Throughput and latency measurements
- **Error condition tests**: Connection failures and edge cases

## Extensibility

- **Interface patterns**: Components follow `*Iface.h` headers for testability and extensibility.
- **Factory patterns**: LoggerFactory allows custom logger implementations.
- **Configuration driven**: RedisConfig enables adaptation to various Redis setups.
- **Modular design**: Clear separation of concerns allows component replacement.

## Summary
The RedisStateManager library implements a layered architecture with clear separation of concerns. The `StateManager` serves as the application interface, orchestrating operations through service layer components (`RedisStorage`, `RedisChannel`, `Logging`) which in turn utilize infrastructure components (`RedisClient`, `ThreadPool`, `RedisConfig`). This design provides thread-safe, high-performance state management with comprehensive error handling and logging, supporting distributed applications requiring consistent state handling and real-time notifications.