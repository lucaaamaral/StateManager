# Custom Logging Examples

This directory contains examples of how to use custom loggers with the StateManager library.

## Overview

The StateManager library supports external logging libraries through a flexible logging interface. This allows you to:

1. Use the built-in default logger (outputs to console)
2. Provide your own custom logger implementation
3. Integrate with existing logging frameworks

## Example Files

- `custom_logger_example.cpp`: Demonstrates basic ways to use custom loggers with StateManager
- `advanced_logging_example.cpp`: Demonstrates advanced logging techniques including:
  - Rotating file logger with size-based rotation
  - Filtered logger for controlling log verbosity
  - Asynchronous logger for improved performance
  - Performance measuring logger for monitoring log throughput
  - Thread-safe singleton pattern demonstration

## How to Use Custom Loggers

### Option 1: Use the Default Logger

The default logger outputs log messages to the console with timestamps. This is used automatically if no custom logger is provided.

```cpp
// The default logger will be used automatically
StateManager::RedisConfig config;
auto stateManager = StateManager::StateManager::createWithDefaults();
```

### Option 2: Provide a Custom Logger

You can create your own logger by implementing the `StateManager::logging::LoggerIface` interface:

```cpp
// Create a custom logger
auto customLogger = std::make_shared<YourCustomLogger>();

// Set it as the global logger
StateManager::logging::LoggerFactory::setLogger(customLogger);

// Now create your state manager
StateManager::RedisConfig config;
auto stateManager = StateManager::StateManager::createWithDefaults();
```

### Option 3: Integrate with Existing Logging Frameworks

You can create an adapter for any existing logging framework by implementing the `LoggerIface` interface:

```cpp
class SpdlogAdapter : public StateManager::logging::LoggerIface {
private:
    std::shared_ptr<spdlog::logger> logger_;

public:
    SpdlogAdapter(std::shared_ptr<spdlog::logger> logger) : logger_(logger) {}

    void log(const std::string& level, const std::string& message, const std::string& context = "") override {
        if (level == "DEBUG") logger_->debug(message);
        else if (level == "INFO") logger_->info(message);
        else if (level == "WARNING") logger_->warn(message);
        else if (level == "ERROR") logger_->error(message);
        else if (level == "CRITICAL") logger_->critical(message);
        else logger_->info(message);
    }

    void log(StateManager::logging::LogLevel level, const std::string& message, const std::string& context = "") override {
        switch (level) {
            case StateManager::logging::LogLevel::DEBUG: logger_->debug(message); break;
            case StateManager::logging::LogLevel::INFO: logger_->info(message); break;
            case StateManager::logging::LogLevel::WARNING: logger_->warn(message); break;
            case StateManager::logging::LogLevel::ERROR: logger_->error(message); break;
            case StateManager::logging::LogLevel::CRITICAL: logger_->critical(message); break;
            default: logger_->info(message); break;
        }
    }
};
```

## Advanced Logging Techniques

### Rotating File Logger

The `RotatingFileLogger` example demonstrates how to implement a file logger that automatically rotates log files when they reach a certain size:

```cpp
// Create a rotating file logger
auto rotatingLogger = std::make_shared<RotatingFileLogger>(
    "app_log",     // Base filename
    1024 * 1024,   // Max file size in bytes (1MB)
    5              // Maximum number of files to keep
);

// Set it as the global logger
StateManager::logging::LoggerFactory::setLogger(rotatingLogger);
```

### Filtered Logger

The `FilteredLogger` example shows how to filter logs based on their level:

```cpp
// Create a filtered logger that only logs WARNING and above
auto baseLogger = StateManager::logging::LoggerFactory::getLogger();
auto filteredLogger = std::make_shared<FilteredLogger>(
    StateManager::logging::LogLevel::WARNING,  // Minimum level to log
    baseLogger
);

// Set it as the global logger
StateManager::logging::LoggerFactory::setLogger(filteredLogger);
```

### Asynchronous Logger

The `AsyncLogger` example demonstrates how to implement asynchronous logging for improved performance:

```cpp
// Create an asynchronous logger
auto baseLogger = StateManager::logging::LoggerFactory::getLogger();
auto asyncLogger = std::make_shared<AsyncLogger>(
    baseLogger,
    1000  // Maximum queue size
);

// Set it as the global logger
StateManager::logging::LoggerFactory::setLogger(asyncLogger);
```

### Performance Measuring Logger

The `PerformanceLogger` example shows how to measure logging performance:

```cpp
// Create a performance measuring logger
auto baseLogger = StateManager::logging::LoggerFactory::getLogger();
auto perfLogger = std::make_shared<PerformanceLogger>(baseLogger);

// Set it as the global logger
StateManager::logging::LoggerFactory::setLogger(perfLogger);

// Later, print performance statistics
perfLogger->printStatistics();
```

## Building the Examples

To build the examples, use the following commands:

```bash
# From the project root directory
mkdir -p build && cd build
cmake ..
make
```

The examples will be built in the `build/examples/logging` directory.

## Thread Safety

All the logger implementations in these examples are thread-safe. The LoggerFactory singleton ensures that logger access and configuration is thread-safe across your entire application.