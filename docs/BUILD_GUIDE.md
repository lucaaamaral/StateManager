# Build and Setup Guide

This guide provides comprehensive instructions for building and setting up the StateManager library.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Dependencies](#dependencies)
3. [Build Options](#build-options)
4. [Docker Setup](#docker-setup)
5. [Manual Installation](#manual-installation)
6. [Development Setup](#development-setup)
7. [Testing](#testing)
8. [Troubleshooting](#troubleshooting)

## Quick Start

### Using Docker (Recommended)

The fastest way to get started is using Docker:

```bash
# Clone the repository
git clone <repository-url>
cd StateManager

# Run the mock demo (no Redis required)
docker build --target lib_build -t redis-state-manager:dev .
docker run --rm redis-state-manager:dev ./bin/basic_demo

# Full development environment with Redis
docker-compose up dev
```

### Using CMake (Local Build)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake libhiredis-dev libgtest-dev

# Install nlohmann/json
sudo apt-get install nlohmann-json3-dev

# Build the project
mkdir build && cd build
cmake ..
make

# Run examples (requires Redis)
./bin/basic_demo
./bin/multithreaded_demo
```

## Dependencies

### Required Dependencies

1. **C++17 Compiler**
   - GCC 7+ or Clang 5+
   - CMake 3.10+

2. **nlohmann/json Library**
   - For JSON serialization/deserialization
   - Can be installed system-wide or included locally

3. **hiredis Library**
   - C client library for Redis
   - Required for Redis connectivity
   - Included as a Git submodule in the `external/` directory
   - Managed automatically by CMake during the build process

4. **redis-plus-plus Library**
   - C++ client library for Redis
   - Included as a Git submodule in the `external/` directory
   - Managed automatically by CMake during the build process

5. **For Mock Demo Only**
   - pthread library (usually available by default)

### Dependency Management

The project uses Git submodules to manage its core dependencies:

- **hiredis** and **redis-plus-plus** are included as Git submodules in the `external/` directory
- CMake automatically initializes and builds these dependencies during the build process
- These dependencies are declared as **private** in CMake, meaning they are not exposed to users of the library
- Users of the StateManager library do not need to include or link against these dependencies directly

When building with CMake, the build system:
1. Initializes the Git submodules if needed
2. Builds the dependencies with appropriate options
3. Links them privately to the StateManager library
4. Ensures proper encapsulation of the implementation details

### Optional Dependencies (for full Redis functionality)

1. **Redis Server**
   - Version 6.0+ recommended
   - Required for examples and full testing

2. **Google Test (GTest)**
   - Required for running the test suite
   - Optional for library building

### Installing Dependencies

#### Ubuntu/Debian

```bash
# Basic build tools
sudo apt-get update
sudo apt-get install build-essential cmake git pkg-config

# nlohmann/json (system installation)
sudo apt-get install nlohmann-json3-dev

# Redis and hiredis
sudo apt-get install redis-server libhiredis-dev

# Google Test
sudo apt-get install libgtest-dev libgmock-dev

# Build and install redis++
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

#### macOS (using Homebrew)

```bash
# Basic build tools
brew install cmake git pkg-config

# nlohmann/json
brew install nlohmann-json

# Redis and hiredis
brew install redis hiredis

# Google Test
brew install googletest

# redis++ (build from source)
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
sudo make install
```

#### CentOS/RHEL/Fedora

```bash
# Basic build tools (Fedora)
sudo dnf install gcc-c++ cmake git pkg-config

# Or for CentOS/RHEL
sudo yum install gcc-c++ cmake3 git pkg-config

# Install EPEL repository for additional packages (CentOS/RHEL)
sudo yum install epel-release

# Redis and hiredis
sudo dnf install redis hiredis-devel  # Fedora
# sudo yum install redis hiredis-devel  # CentOS/RHEL

# Build nlohmann/json and redis++ from source (similar to Ubuntu instructions)
```

## Build Options

### Using CMake

The project uses CMake as its build system with multiple options:

```bash
# Create build directory
mkdir build && cd build

# Configure with default options
cmake ..

# Configure with custom options
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON

# Build
make

# Install
sudo make install
```

### Available CMake Options

```bash
# Show all available options
cmake -LA ..

# Common options
-DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo  # Build type
-DBUILD_SHARED_LIBS=ON|OFF                       # Build shared library
-DBUILD_TESTING=ON|OFF                           # Build test suite
-DBUILD_EXAMPLES=ON|OFF                          # Build examples
-DENABLE_CODE_FORMATTING=ON|OFF                  # Enable code formatting
-DENABLE_STATIC_ANALYSIS=ON|OFF                  # Enable static analysis
-DBUILD_DOCS=ON|OFF                              # Build documentation
-DCMAKE_INSTALL_PREFIX=/path/to/install          # Custom install location
```

### Build Targets

```bash
# Build everything (default)
make

# Build specific targets
make StateManager       # Build library only
make basic_demo              # Build basic example
make multithreaded_demo      # Build multithreaded example
make StateManagerTests  # Build tests

# Run tests
make test
ctest -V                     # Verbose test output

# Format code (if ENABLE_CODE_FORMATTING=ON)
make format

# Static analysis (if ENABLE_STATIC_ANALYSIS=ON)
make analyze

# Generate documentation (if BUILD_DOCS=ON)
make docs

# Create package
make package
```

## Docker Setup

### Development Environment

```bash
# Build development image
docker build --target lib_build -t redis-state-manager:dev .

# Run interactive development environment
docker run -it --rm -v $(pwd):/app redis-state-manager:dev bash

# Or use Docker Compose
docker-compose up dev
```

### Testing with Docker Compose

The project includes a Docker Compose setup that provides a complete testing environment with Redis:

```bash
# Run tests with Docker Compose
docker-compose up test

# This command:
# 1. Starts a Redis container with the proper configuration
# 2. Builds the StateManager library
# 3. Runs the tests
```

#### Rebuilding After Code Changes

When you make changes to the code, you need to rebuild the Docker image to test your changes:

```bash
# Rebuild the test image
docker-compose build test

# Run tests with the updated code
docker-compose up test
```

#### Interactive Development

For faster development iterations, you can use the development environment:

```bash
# Start Redis in the background
docker-compose up -d redis

# Start an interactive development container
docker-compose run --rm dev

# Inside the container, you can build and run tests
mkdir -p build && cd build
cmake -DBUILD_TESTING=ON ..
make
make test
```

This approach mounts your local code as a volume, so you can edit files locally and rebuild quickly inside the container without having to rebuild the entire Docker image.

## Manual Installation

### Building from Source

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd StateManager
   ```

2. **Install dependencies** (see Dependencies section above)

3. **Build the library**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

4. **Install system-wide**
   ```bash
   sudo make install
   ```

5. **Verify installation**
   ```bash
   # Check if headers are installed
   ls /usr/local/include/state_manager/

   # Check if library is installed
   ls /usr/local/lib/libStateManager.a
   ```

### Using in Your Project

#### With pkg-config (after installation)

```bash
g++ -std=c++17 your_app.cpp $(pkg-config --cflags --libs StateManager)
```

#### Manual linking

```bash
g++ -std=c++17 -I/usr/local/include your_app.cpp -L/usr/local/lib -lStateManager -lhiredis -lredis++ -lpthread
```

#### CMake integration

```cmake
find_package(StateManager REQUIRED)
target_link_libraries(your_target StateManager::StateManager)
```

## Development Setup

### Setting up the Development Environment

1. **Clone and enter the repository**
   ```bash
   git clone <repository-url>
   cd StateManager
   ```

2. **Install development dependencies**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install clang-format cppcheck doxygen valgrind gdb

   # macOS
   brew install clang-format cppcheck doxygen valgrind
   ```

3. **Set up pre-commit hooks** (optional)
   ```bash
   # Format code before commits
   echo "cd build && cmake --build . --target format" > .git/hooks/pre-commit
   chmod +x .git/hooks/pre-commit
   ```

### Development Workflow

```bash
# Configure with development options
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON -DENABLE_CODE_FORMATTING=ON

# Make changes to source code
vim ../src/StateManager.cpp

# Format code
cmake --build . --target format

# Build and test
cmake --build .
cmake --build . --target test

# Run static analysis
cmake --build . --target analyze

# Generate documentation
cmake --build . --target docs
```

### IDE Configuration

#### Visual Studio Code

Create `.vscode/c_cpp_properties.json`:

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/include",
                "/usr/include/nlohmann",
                "/usr/local/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

## Testing

### Running Tests

```bash
# Configure with testing enabled
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=ON

# Build and run all tests
cmake --build .
cmake --build . --target test

# Run with CTest
ctest
ctest -V  # Verbose output

# Run specific test executable directly
./bin/StateManagerTests

# Run with verbose output
./bin/StateManagerTests --gtest_verbose
```

### Test Categories

1. **Unit Tests** (`test_state_manager.cpp`)
   - Tests core StateManager functionality
   - Mock implementations for isolated testing

2. **Integration Tests** (`test_redis_storage.cpp`)
   - Tests Redis integration
   - Requires running Redis server

3. **Example Applications**
   - Basic demo (`basic_demo.cpp`)
   - Multithreaded demo (`multithreaded_demo.cpp`)

### Testing with Docker

```bash
# Run tests in clean environment
docker-compose up test

# Interactive testing
docker-compose run --rm dev bash
cd build && cmake --build . --target test
```

## Troubleshooting

### Common Issues

#### 1. nlohmann/json not found

**Error**: `fatal error: nlohmann/json.hpp: No such file or directory`

**Solutions**:
```bash
# Install system-wide (Ubuntu/Debian)
sudo apt-get install nlohmann-json3-dev

# Or install to /usr/local/include
wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
sudo mkdir -p /usr/local/include/nlohmann
sudo cp json.hpp /usr/local/include/nlohmann/

# Or use Docker environment
docker-compose up dev
```

#### 2. Redis connection failed

**Error**: Connection refused or timeout errors

**Solutions**:
```bash
# Start Redis server
redis-server

# Check if Redis is running
redis-cli ping

# Use Docker Redis
docker-compose up redis

# Check Redis configuration
redis-cli config get bind
```

#### 3. hiredis/redis++ not found

**Error**: Library linking errors

**Solutions**:
```bash
# Install hiredis (Ubuntu/Debian)
sudo apt-get install libhiredis-dev

# Build redis++ from source
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make && sudo make install
sudo ldconfig

# Use Docker environment
docker-compose up dev
```

#### 4. GTest not found

**Error**: Test building fails

**Solutions**:
```bash
# Install GTest (Ubuntu/Debian)
sudo apt-get install libgtest-dev libgmock-dev

# Build without tests
make library examples mock

# Use Docker for testing
docker-compose up test
```

### Build Issues

#### Permission denied during install

```bash
# Use sudo for system installation
sudo make install

# Or install to user directory
make install PREFIX=$HOME/.local
```

#### Compiler version issues

```bash
# Check compiler version
g++ --version

# Update compiler (Ubuntu/Debian)
sudo apt-get install gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

### Getting Help

1. **Verbose build output**
   ```bash
   make VERBOSE=1
   ```

2. **Use Docker for consistent environment**
   ```bash
   docker-compose up dev
   ```

3. **Check the logs**
   ```bash
   # Redis logs
   tail -f /var/log/redis/redis-server.log

   # Application logs
   ./build/bin/basic_demo 2>&1 | tee app.log
   ```

## Validating the Build

To validate that compilation works correctly, use:

```bash
# Build the test stage to verify compilation
docker build . --target test
```

This command builds both the library and tests, confirming that the code compiles successfully without needing to run docker-compose.

## Rebuilding After Code Changes

When you make changes to the code, you need to rebuild the Docker image to test your changes:

```bash
# Rebuild the test image
docker-compose build test

# Run tests with the updated code
docker-compose up test
```

For faster development iterations, you can use the development environment which mounts your local code as a volume:

```bash
# Start Redis in the background
docker-compose up -d redis

# Start an interactive development container
docker-compose run --rm dev

# Inside the container, you can build and run tests
mkdir -p build && cd build
cmake -DBUILD_TESTING=ON ..
make
make test
```

This approach allows you to edit files locally and rebuild quickly inside the container without having to rebuild the entire Docker image.

## Additional Resources

- [Redis Documentation](https://redis.io/documentation)
- [nlohmann/json Documentation](https://json.nlohmann.me/)
- [hiredis Documentation](https://github.com/redis/hiredis)
- [redis++ Documentation](https://github.com/sewenew/redis-plus-plus)
- [Google Test Documentation](https://google.github.io/googletest/)

For more specific issues, check the project's issue tracker or create a new issue with:
- Operating system and version
- Compiler version
- Dependency versions
- Complete error messages
- Steps to reproduce