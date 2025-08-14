cmake_minimum_required(VERSION 3.15)

# Set project version as in VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_FROM_FILE)
string(STRIP "${VERSION_FROM_FILE}" VERSION_FROM_FILE)
project(StateManager VERSION ${VERSION_FROM_FILE} LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTING "Build the testing tree" OFF)
option(BUILD_EXAMPLES "Build example applications" OFF)
option(ENABLE_CODE_FORMATTING "Enable code formatting (requires clang-format)" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis (requires cppcheck)" OFF)
option(BUILD_DOCS "Build documentation (requires Doxygen)" OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_INCLUDEDIR include CACHE PATH "C++ header files (include)")
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Object code libraries (lib)")

# Initialize Git submodules
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "Git submodule update failed with ${GIT_SUBMOD_RESULT}")
    endif()
endif()

# Ensure hiredis is built with static library to match redis-plus-plus
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static library for hiredis" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "Enable PIC for shared libraries" FORCE)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "Disable redis-plus-plus tests" FORCE)
set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "Disable redis-plus-plus shared library" FORCE)
set(DISABLE_TESTS ON CACHE BOOL "Disable hiredis tests" FORCE)

# Add hiredis first
add_subdirectory(external/hiredis)
set(HIREDIS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(hiredis_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(HIREDIS_LIB hiredis::hiredis)
set(hiredis_FOUND TRUE)

# Add other external libraries
add_subdirectory(external/redis-plus-plus)
add_subdirectory(external/json)


# Define source files
set(SOURCES
    src/logging/DefaultLogger.cpp
    src/logging/LoggerFactory.cpp

    src/thread/ThreadPool.cpp

    src/client/RedisClient.cpp
    src/client/RedisChannel.cpp
    src/client/RedisStorage.cpp

    src/core/StateManager.cpp
)

# Create the library
add_library(StateManager ${SOURCES})

# Set versioning for shared library
set_target_properties(StateManager PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories for the library
target_include_directories(StateManager
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external
        ${CMAKE_CURRENT_SOURCE_DIR}/external/hiredis
        ${CMAKE_CURRENT_SOURCE_DIR}/external/redis-plus-plus/src
)

# Link dependencies
target_link_libraries(StateManager
    PRIVATE
        nlohmann_json::nlohmann_json
        hiredis::hiredis
        redis++::redis++_static
)

# Create a combined static library using ar after build
if(UNIX AND CMAKE_AR)
    add_custom_command(TARGET StateManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/tmp_extract
        COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/tmp_extract && ${CMAKE_AR} -x $<TARGET_FILE:StateManager>
        COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/tmp_extract && ${CMAKE_AR} -x $<TARGET_FILE:hiredis::hiredis>
        COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/tmp_extract && ${CMAKE_AR} -x $<TARGET_FILE:redis++::redis++_static>
        COMMAND ${CMAKE_AR} -qcs $<TARGET_FILE:StateManager> ${CMAKE_CURRENT_BINARY_DIR}/tmp_extract/*.o
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/tmp_extract
        COMMENT "Combining static libraries into StateManager"
    )
endif()

# Compiler flags
target_compile_options(StateManager PRIVATE -Wall -Wextra -O2)

# Installation
install(TARGETS StateManager
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin
)

# Install the header files
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/json/include/nlohmann
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/hiredis/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external/redis-plus-plus/src/sw/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sw
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

include(CMakePackageConfigHelpers)

# Create a config file template directly in the CMakeLists.txt
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/StateManagerConfigTemplate.cmake"
"@PACKAGE_INIT@

include(CMakeFindDependencyMacro)
# Set variables for include directories and libraries
set(STATEMANAGER_INCLUDE_DIRS \"@PACKAGE_CMAKE_INSTALL_INCLUDEDIR@\")
set(STATEMANAGER_LIBRARIES StateManager)
")

configure_package_config_file(
    ${CMAKE_CURRENT_BINARY_DIR}/StateManagerConfigTemplate.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/StateManagerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StateManager
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/StateManagerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/StateManagerConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/StateManagerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StateManager
)


# Add examples
if(BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
    add_subdirectory(examples)
endif()

# Enable testing
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
endif()

# Code formatting
if(ENABLE_CODE_FORMATTING)
    find_program(CLANG_FORMAT clang-format)
    if(CLANG_FORMAT)
        file(GLOB_RECURSE ALL_SOURCE_FILES 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        )
        
        add_custom_target(format
            COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
            COMMENT "Formatting code with clang-format"
            VERBATIM
        )
    else()
        message(WARNING "clang-format not found, code formatting target not available")
    endif()
endif()

# Static analysis
if(ENABLE_STATIC_ANALYSIS)
    find_program(CPPCHECK cppcheck)
    if(CPPCHECK)
        add_custom_target(analyze
            COMMAND ${CPPCHECK} --enable=all --std=c++17 
                    --suppress=missingIncludeSystem
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/include
            COMMENT "Running static analysis with cppcheck"
            VERBATIM
        )
    else()
        message(WARNING "cppcheck not found, static analysis target not available")
    endif()
endif()

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        if(EXISTS ${DOXYGEN_IN})
            configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
            
            add_custom_target(docs
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
            )
        else()
            add_custom_target(docs
                COMMAND ${DOXYGEN_EXECUTABLE} -g ${DOXYGEN_OUT}
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
            )
        endif()
    else()
        message(WARNING "Doxygen not found, documentation target not available")
    endif()
endif()

# Package configuration
include(CPack)
add_custom_target(create-package
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
    COMMENT "Creating distribution package"
    VERBATIM
)

# Print build information
message(STATUS "")
message(STATUS "StateManager Configuration:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Shared libs:          ${BUILD_SHARED_LIBS}")
message(STATUS "  Build testing:        ${BUILD_TESTING}")
message(STATUS "  Build examples:       ${BUILD_EXAMPLES}")
message(STATUS "  Code formatting:      ${ENABLE_CODE_FORMATTING}")
message(STATUS "  Static analysis:      ${ENABLE_STATIC_ANALYSIS}")
message(STATUS "  Documentation:        ${BUILD_DOCS}")
message(STATUS "")