# RedisStateManager Testing Documentation

## Overview

This document provides comprehensive information about the testing strategy, test cases, and scenarios implemented for the RedisStateManager library. The testing framework is built using GoogleTest and covers unit tests, integration tests, thread safety tests, and performance tests.

## Test Structure

### Test Organization

```
tests/
├── CMakeLists.txt              # Test build configuration
├── test_logger.cpp             # Logging system tests
├── test_redis_channel.cpp      # Pub/sub functionality tests
├── test_redis_client.cpp       # Redis client connection tests
├── test_state_manager.cpp      # Core state management tests
└── test_threadpool.cpp         # ThreadPool implementation tests
```

### Test Fixture Pattern

All test files follow a consistent fixture pattern:

```cpp
class ComponentTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize test environment
    // Set logging levels
    // Create test instances
  }

  void TearDown() override {
    // Clean up resources
    // Reset singleton states
  }
};
```

## Test Execution

### Building Tests

```bash
# Standard build with testing enabled
mkdir -p build && cd build
cmake -DBUILD_TESTING=ON ..
cmake --build .
```

### Running Tests

```bash
# Run all tests
./bin/StateManagerTests

# Run specific test suites
./bin/StateManagerTests --gtest_filter="ThreadPoolTest.*"
./bin/StateManagerTests --gtest_filter="*RedisChannel*"

# Run single test
./bin/StateManagerTests --gtest_filter="*StateManagerTest.JSONDataTypes_int*"
```

### Docker Testing Environment

For consistent testing environment:

```bash
# Run tests in Docker (recommended)
docker-compose up test

# Development environment with volume mount
docker-compose up -d redis
docker-compose run --rm dev
```

## Test Requirements

### Prerequisites

1. **Redis Server**: Tests require a running Redis instance
   - Default connection: `localhost:6379`
   - No authentication required for tests
   - Clean database state recommended

2. **GoogleTest**: Testing framework dependency
   - Automatically built with project
   - Version managed via CMake

3. **System Requirements**:
   - C++17 compatible compiler
   - CMake 3.10 or higher
   - Sufficient system resources for concurrent tests

### Environment Setup

**Redis Configuration**:
```bash
# Start Redis for testing
redis-server --port 6379 --daemonize yes

# Or use Docker
docker run -d -p 6379:6379 redis:alpine
```

**Test Data Cleanup**:
Tests are designed to be self-contained and clean up after execution, but manual cleanup may be needed:
```bash
redis-cli FLUSHDB  # Clear test database
```

## Test Results and Coverage

### Expected Test Results

When Redis is available, all tests should pass:
```
[==========] Running X tests from Y test suites.
[----------] Global test environment set-up.
[----------] X tests from StateManagerTest
[  PASSED  ] All tests should pass with Redis running
[----------] X tests from StateManagerTest (N ms total)
[==========] X tests from Y test suites ran. (N ms total)
[  PASSED  ] X tests.
```

### Test Coverage Areas

**Core Functionality**:
- ✅ JSON serialization/deserialization
- ✅ Redis storage operations
- ✅ Error handling and edge cases
- ✅ Thread safety validations

**Integration Points**:
- ✅ Redis connectivity
- ✅ Pub/sub message flow
- ✅ Logging integration
- ✅ ThreadPool integration

**Performance and Scalability**:
- ✅ Multiple key operations
- ✅ Concurrent access patterns
- ✅ Resource cleanup

## Test Development Guidelines

### Adding New Tests

1. **Follow Naming Convention**:
   ```cpp
   TEST_F(ComponentTest, FeatureArea_specific_case)
   ```

2. **Use Descriptive Assertions**:
   ```cpp
   EXPECT_TRUE(result) << "Operation should succeed with valid input";
   EXPECT_EQ(actual, expected) << "Values should match after round-trip";
   ```

3. **Implement Proper Cleanup**:
   ```cpp
   void TearDown() override {
     // Clean up test data
     if (test_key_created) {
       state_manager->erase(test_key);
     }
   }
   ```

4. **Test Both Success and Failure Cases**:
   ```cpp
   // Test success case
   EXPECT_TRUE(operation_with_valid_input());
   
   // Test failure case
   EXPECT_FALSE(operation_with_invalid_input());
   ```

### Thread Safety Testing

For thread safety tests:
```cpp
TEST_F(ComponentTest, ThreadSafety_concurrent_access) {
  const int num_threads = 10;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};
  
  // Create concurrent operations
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      // Perform thread-safe operations
      if (perform_operation()) {
        success_count.fetch_add(1);
      }
    });
  }
  
  // Wait for completion
  for (auto& t : threads) {
    t.join();
  }
  
  EXPECT_EQ(success_count.load(), num_threads);
}
```

## Continuous Integration

### Test Automation

Tests are designed to run in CI/CD environments:

1. **Docker-based Testing**: Consistent environment across platforms
2. **Automated Redis Setup**: Tests can start Redis containers
3. **Parallel Execution**: Tests support parallel test execution
4. **Result Reporting**: GoogleTest XML output for CI integration

### Performance Benchmarks

Regular performance testing includes:
- **Throughput Tests**: Operations per second measurements
- **Latency Tests**: Response time distributions
- **Memory Tests**: Resource usage monitoring
- **Stress Tests**: High-load scenario validation

## Troubleshooting

### Common Test Failures

1. **Redis Connection Errors**:
   ```
   Error: failed to connect to Redis (localhost:6379): Connection refused
   ```
   **Solution**: Ensure Redis server is running and accessible

2. **Thread Safety Violations**:
   ```
   Error: Data race detected or assertion failed in concurrent test
   ```
   **Solution**: Review thread safety implementation and synchronization

3. **Memory Leaks**:
   ```
   Error: Memory leak detected in component
   ```
   **Solution**: Verify proper RAII patterns and resource cleanup

### Debug Test Execution

Enable verbose logging for debugging:
```bash
# Run with debug logging
./bin/StateManagerTests --gtest_filter="FailingTest.*" --verbose

# Or set environment variable
export GTEST_VERBOSE=1
./bin/StateManagerTests
```

## Summary

The RedisStateManager testing framework provides comprehensive coverage across all components and integration points. Tests are organized by functionality and include unit tests, integration tests, thread safety validation, and performance verification. The testing infrastructure supports both local development and continuous integration environments, ensuring robust validation of the library's functionality across different scenarios and use cases.