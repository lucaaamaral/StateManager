# Logging System Documentation

This document describes the logging system in the StateManager library and provides instructions on how to connect external logging classes.

## Overview

The StateManager library includes a flexible logging system that:

- Provides different log levels (TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL)
- Supports a default console logger implementation
- Allows integration with external logging frameworks
- Uses a factory pattern to manage logger instances

## Components

The logging system consists of the following components:

### LogLevel

An enumeration that defines the hierarchy of log levels:

```cpp
enum class LogLevel {
    TRACE,    // Most verbose level for detailed tracing
    DEBUG,    // Debug information
    INFO,     // General information
    WARNING,  // Warnings that don't prevent operation
    ERROR,    // Errors that may impact functionality
    CRITICAL  // Critical errors that prevent operation
};
```

Utility functions are provided for converting between string and enum representations:
- `logLevelToString(LogLevel level)`: Converts a LogLevel enum to a string
- `stringToLogLevel(const std::string& level)`: Converts a string to a LogLevel enum

### LoggerIface

The abstract interface that all logger implementations must implement:

```cpp
class LoggerIface {
public:
    virtual ~LoggerIface() = default;
    
    // Core logging methods that must be implemented
    virtual void log(const std::string& level, const std::string& message) = 0;
    virtual void log(LogLevel level, const std::string& message);
    
    // Convenience methods for different log levels
    virtual void trace(const std::string& message);
    virtual void debug(const std::string& message);
    virtual void info(const std::string& message);
    virtual void warning(const std::string& message);
    virtual void error(const std::string& message);
    virtual void critical(const std::string& message);
};
```

### DefaultLogger

A basic implementation of LoggerIface that outputs log messages to the console:

```cpp
class DefaultLogger : public LoggerIface {
public:
    void log(const std::string& level, const std::string& message) override;
    void log(LogLevel level, const std::string& message) override;
};
```

### LoggerFactory

A factory class that implements the Singleton pattern to manage a global logger instance. The LoggerFactory implements the Singleton pattern with the following characteristics:

1. **Thread-Safe Initialization**: Uses a static local instance in `getInstance()` combined with mutex protection to ensure thread-safe lazy initialization.
2. **Lazy Initialization**: The singleton instance is only created when first needed.
3. **Global Access Point**: Provides a centralized access point to the logger through static methods.
4. **Default Implementation**: Automatically creates a default logger if none is provided.
5. **Customization**: Allows the default logger to be replaced with a custom implementation.

## Connecting External Logging Classes

To connect an external logging class to the StateManager library, follow these steps:

### 1. Create a Custom Logger Implementation

Create a class that implements the `LoggerIface` interface:

```cpp
#include "logging/LoggerIface.h"
#include "YourExternalLoggingFramework.h"

class CustomLogger : public StateManager::logging::LoggerIface {
private:
    // Your external logger instance
    YourExternalLogger externalLogger;

public:
    CustomLogger() {
        // Initialize your external logger
    }

    // Implement the required methods
    void log(const std::string& level, const std::string& message) override {
        // Convert to your logging framework's format and log
        externalLogger.log(level, message);
    }

    void log(StateManager::logging::LogLevel level, const std::string& message) override {
        // You can use the default implementation or customize it
        // Default calls the string-based version after converting the enum
        LoggerIface::log(level, message);
        
        // Or implement custom behavior:
        // switch (level) {
        //     case StateManager::logging::LogLevel::DEBUG:
        //         externalLogger.debug(message);
        //         break;
        //     // ... handle other levels
        // }
    }
};
```

### 2. Register Your Custom Logger

Register your custom logger with the LoggerFactory at the beginning of your application:

```cpp
#include "logging/LoggerFactory.h"
#include "YourCustomLogger.h"

void initializeLogging() {
    // Create your custom logger
    auto customLogger = std::make_shared<CustomLogger>();
    
    // Set it as the global logger
    StateManager::logging::LoggerFactory::setLogger(customLogger);
}

int main() {
    // Initialize logging early in your application
    initializeLogging();
    
    // Now all logging in StateManager will use your custom logger
    // ...
}
```

### 3. Using the Logger

The library will now use your custom logger for all logging operations. You don't need to change any code in the library itself.

## Advanced Usage

### Thread Safety

The logging system is thread-safe. The LoggerFactory uses a mutex to protect access to the global logger instance.

### Performance Considerations

When implementing a custom logger, consider the performance implications:

- Expensive logging operations should be deferred until needed
- Consider using asynchronous logging for high-performance applications
- Implement filtering by log level to reduce overhead

### Log Level Filtering

Your custom logger can implement log level filtering to control the verbosity of logs:

```cpp
class FilteredLogger : public StateManager::logging::LoggerIface {
private:
    StateManager::logging::LogLevel minLevel;
    std::shared_ptr<StateManager::logging::LoggerIface> baseLogger;

public:
    FilteredLogger(
        StateManager::logging::LogLevel minLevel,
        std::shared_ptr<StateManager::logging::LoggerIface> baseLogger
    ) : minLevel(minLevel), baseLogger(std::move(baseLogger)) {}

    void log(StateManager::logging::LogLevel level, const std::string& message) override {
        if (level >= minLevel) {
            baseLogger->log(level, message);
        }
    }

    void log(const std::string& level, const std::string& message) override {
        log(StateManager::logging::stringToLogLevel(level), message);
    }
};

// Usage:
auto baseLogger = StateManager::logging::LoggerFactory::createDefaultLogger();
auto filteredLogger = std::make_shared<FilteredLogger>(
    StateManager::logging::LogLevel::WARNING,  // Only log WARNING and above
    baseLogger
);
StateManager::logging::LoggerFactory::setLogger(filteredLogger);
```

## Examples

### Basic Usage with Default Logger

```cpp
#include "logging/LoggerFactory.h"

void exampleFunction() {
    auto logger = StateManager::logging::LoggerFactory::getLogger();
    
    logger->trace("Detailed tracing information");
    logger->debug("Debug information");
    logger->info("General information");
    logger->warning("Warning message");
    logger->error("Error message");
    logger->critical("Critical error message");
}
```

### Integration with spdlog

```cpp
#include "logging/LoggerIface.h"
#include "logging/LoggerFactory.h"
#include <spdlog/spdlog.h>
#include <memory>

class SpdlogAdapter : public StateManager::logging::LoggerIface {
private:
    std::shared_ptr<spdlog::logger> logger;

public:
    SpdlogAdapter(std::shared_ptr<spdlog::logger> logger) : logger(logger) {}

    void log(const std::string& level, const std::string& message) override {
        auto logLevel = StateManager::logging::stringToLogLevel(level);
        log(logLevel, message);
    }

    void log(StateManager::logging::LogLevel level, const std::string& message) override {
        switch (level) {
            case StateManager::logging::LogLevel::TRACE:
                logger->trace(message);
                break;
            case StateManager::logging::LogLevel::DEBUG:
                logger->debug(message);
                break;
            case StateManager::logging::LogLevel::INFO:
                logger->info(message);
                break;
            case StateManager::logging::LogLevel::WARNING:
                logger->warn(message);
                break;
            case StateManager::logging::LogLevel::ERROR:
                logger->error(message);
                break;
            case StateManager::logging::LogLevel::CRITICAL:
                logger->critical(message);
                break;
        }
    }
};

// Usage:
void initLogging() {
    auto spdLogger = spdlog::stdout_color_mt("redis_state_manager");
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    
    auto adapter = std::make_shared<SpdlogAdapter>(spdLogger);
    StateManager::logging::LoggerFactory::setLogger(adapter);
}
```

## Conclusion

The StateManager logging system is designed to be flexible and extensible. By implementing the LoggerIface interface, you can integrate any external logging framework with the library.

## Singleton Pattern Implementation

The LoggerFactory class implements the Singleton design pattern to ensure that only one logger instance exists throughout the application. This approach provides several benefits:

### Implementation Details

The singleton pattern is implemented using the following techniques:

1. **Static Instance Method**: The `getInstance()` method uses the "static local variable" approach for thread-safe lazy initialization:
   ```cpp
   LoggerFactory& LoggerFactory::getInstance() {
       static LoggerFactory instance;
       return instance;
   }
   ```
   This technique is guaranteed to be thread-safe since C++11, as the standard ensures that initialization of static local variables is performed only once, even in multi-threaded environments.

2. **Private Constructor**: The LoggerFactory constructor is public but is only called by the getInstance() method, ensuring controlled instantiation.

3. **Static Members**: The global logger instance and mutex are static members:
   ```cpp
   static std::shared_ptr<LoggerIface> global_logger_;
   static std::mutex logger_mutex_;
   ```

4. **Double-Checked Locking**: The implementation uses mutex protection to ensure thread safety when accessing or modifying the global logger:
   ```cpp
   std::lock_guard<std::mutex> lock(logger_mutex_);
   ```

### Thread Safety

The LoggerFactory singleton implementation is thread-safe due to:

1. **Static Local Instance**: C++11 guarantees thread-safe initialization of the static local instance in `getInstance()`.
2. **Mutex Protection**: All access to the global logger is protected by a mutex.
3. **Atomic Operations**: The shared_ptr ensures atomic reference counting.

### Benefits of the Singleton Pattern for Logging

Using the Singleton pattern for the logger provides several advantages:

1. **Global Access**: Any component in the application can access the logger without passing it as a parameter.
2. **Single Configuration**: The logging configuration is defined in one place and applies globally.
3. **Resource Efficiency**: Only one logger instance is created, saving memory and resources.
4. **Consistent Behavior**: All log messages are handled consistently through the same logger instance.
5. **Lazy Initialization**: The logger is only created when first needed, improving startup performance.

### Usage Example

```cpp
// Get the logger instance (creates it if it doesn't exist)
auto logger = StateManager::logging::LoggerFactory::getLogger();

// Log a message
logger->info("Application started");

// Later in the code, get the same logger instance
auto sameLogger = StateManager::logging::LoggerFactory::getLogger();
sameLogger->debug("Operation completed");  // Uses the same logger instance
```