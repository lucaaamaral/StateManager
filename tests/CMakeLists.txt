cmake_minimum_required(VERSION 3.15)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find the Threads package
find_package(Threads REQUIRED)

# Set output directory for test executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Define test sources
set(TEST_SOURCES
    test_logger.cpp
    test_threadpool.cpp
    test_redis_channel.cpp
    test_redis_client.cpp
    test_state_manager.cpp
)

# Create test executable
add_executable(StateManagerTests ${TEST_SOURCES})

# Link libraries
target_link_libraries(StateManagerTests
    PRIVATE
        StateManager
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
        nlohmann_json::nlohmann_json
        redis++::redis++_static
        hiredis::hiredis
)

# Compiler flags
target_compile_options(StateManagerTests PRIVATE -Wall -Wextra -O2)

# Add test
add_test(NAME StateManagerTests COMMAND StateManagerTests)
set_tests_properties(StateManagerTests PROPERTIES 
    TIMEOUT 10  # Set 10 second timeout
    ENVIRONMENT "GTEST_TIMEOUT=10"  # Also set environment variable for gtest
)

# Install test executable
install(TARGETS StateManagerTests DESTINATION bin)
