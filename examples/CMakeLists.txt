cmake_minimum_required(VERSION 3.10)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the Threads package
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external/json/include)
include_directories(${CMAKE_SOURCE_DIR}/external/redis-plus-plus/src)
include_directories(${CMAKE_SOURCE_DIR}/external/hiredis)

# Add executables
add_executable(basic_demo basic_demo.cpp)
add_executable(multithreaded_demo multithreaded_demo.cpp)
add_executable(pub_sub_demo pub_sub_demo.cpp)

# Link against libraries
target_link_libraries(basic_demo 
    StateManager
    hiredis::hiredis
    redis++::redis++_static
    Threads::Threads
)

target_link_libraries(multithreaded_demo 
    StateManager
    hiredis::hiredis
    redis++::redis++_static
    Threads::Threads
)

target_link_libraries(pub_sub_demo 
    StateManager
    hiredis::hiredis
    redis++::redis++_static
    Threads::Threads
)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add subdirectories
add_subdirectory(logging)