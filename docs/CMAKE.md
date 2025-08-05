# CMake Configuration Documentation

This document provides a comprehensive overview of the CMake configuration for the StateManager project, explaining build options, dependencies, and best practices.

## Table of Contents

1. [Overview](#overview)
2. [Build Options](#build-options)
3. [Dependencies](#dependencies)
4. [Installation](#installation)
5. [Using StateManager in Your Project](#using-statemanager-in-your-project)
6. [Development Guidelines](#development-guidelines)

## Overview

StateManager uses CMake (minimum version 3.15) as its build system. The project follows modern CMake practices with a focus on:

- Target-based configuration
- Proper dependency management
- Modular component structure
- Comprehensive package configuration

## Build Options

The following build options can be configured:

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | OFF | Build shared libraries instead of static |
| `BUILD_TESTING` | OFF | Build the testing tree |
| `BUILD_EXAMPLES` | OFF | Build example applications |
| `ENABLE_CODE_FORMATTING` | OFF | Enable code formatting (requires clang-format) |
| `ENABLE_STATIC_ANALYSIS` | OFF | Enable static analysis (requires cppcheck) |
| `BUILD_DOCS` | OFF | Build documentation (requires Doxygen) |

### Setting Build Options

You can set these options when configuring the project:

```bash
cmake -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON ..
```

## Dependencies

StateManager has the following dependencies:

### Core Library Dependencies

These dependencies are statically linked into the library and are not exposed to users:

- **hiredis**: Redis client library
- **redis++**: C++ wrapper for hiredis
- **nlohmann_json**: JSON parsing library

### Test Dependencies

- **GoogleTest**: Testing framework (automatically fetched)
- **Threads**: Threading support

### Example Dependencies

- **Threads**: Threading support

## Dependency Management

The project follows these principles for dependency management:

1. **Private Dependencies**: Core dependencies (hiredis, redis++, nlohmann_json) are:
   - Included as Git submodules
   - Statically linked into StateManager
   - Not exposed to library users
   - Not mentioned in the StateManagerConfig.cmake file

2. **Modular Dependencies**: Each component declares only the dependencies it needs:
   - Main library doesn't require Threads
   - Tests and examples explicitly declare their Threads dependency

3. **Encapsulation**: Implementation details are properly encapsulated:
   - Private dependencies are not exposed to users
   - Users don't need to find or link against the library's private dependencies

## Installation

The library installs the following components:

- Library binary (static or shared)
- Header files
- CMake package configuration files

### Installation Directories

- Headers: `${CMAKE_INSTALL_PREFIX}/include`
- Library: `${CMAKE_INSTALL_PREFIX}/lib`
- CMake config: `${CMAKE_INSTALL_PREFIX}/lib/cmake/StateManager`

## Using StateManager in Your Project

After installing StateManager, you can use it in your CMake project:

```cmake
find_package(StateManager REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE StateManager)
```

The `find_package` command will:
- Find the StateManager library
- Set up include directories
- Make the StateManager target available

## Development Guidelines

When developing StateManager or extending its CMake configuration:

1. **Dependency Management**:
   - Keep private dependencies private
   - Statically link implementation dependencies
   - Each component should declare its own dependencies

2. **Package Configuration**:
   - Do not expose private dependencies in the config file
   - Ensure the config file is self-contained
   - Use `configure_package_config_file()` for proper path handling

3. **Testing**:
   - Validate changes with both minimal builds (library only) and full builds (with tests)
   - Use Docker for consistent build validation: `docker build . --target test` and `docker build . --target example`

4. **Documentation**:
   - Document CMake changes in `CMAKE_CHANGES.md`
   - Keep this documentation up to date