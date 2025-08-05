cmake_minimum_required(VERSION 3.10)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the Threads package
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add executables
add_executable(custom_logger_example custom_logger_example.cpp)
add_executable(advanced_logging_example advanced_logging_example.cpp)

# Link against libraries
target_link_libraries(custom_logger_example 
    StateManager
    Threads::Threads
)

target_link_libraries(advanced_logging_example 
    StateManager
    Threads::Threads
)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/logging)