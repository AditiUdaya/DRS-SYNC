cmake_minimum_required(VERSION 3.15)
project(DRS_SYNC VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS: Add Homebrew paths
if(APPLE)
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE HOMEBREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    include_directories("${HOMEBREW_PREFIX}/include")
    link_directories("${HOMEBREW_PREFIX}/lib")
endif()

# Find minimal packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Check for Asio
find_path(ASIO_INCLUDE_DIR asio.hpp 
    PATHS /usr/local/include /opt/homebrew/include
)
if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Asio not found. Install: brew install asio")
endif()

# Include directories
include_directories(${ASIO_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)

# Core library sources (exclude api servers)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*api_server.*\\.cpp$")

# Core library
add_library(drs_sync_lib STATIC ${LIB_SOURCES})
target_link_libraries(drs_sync_lib 
    PUBLIC Threads::Threads
    sqlite3
)

# Lightweight API Server
add_executable(drs_sync_api src/api_server_light.cpp)
target_link_libraries(drs_sync_api 
    PRIVATE drs_sync_lib
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# macOS specific flags
if(APPLE)
    target_compile_definitions(drs_sync_api PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(drs_sync_api PRIVATE "-framework Security")
endif()

message(STATUS "✅ DRS-SYNC Lightweight configured successfully!")
message(STATUS "📦 Using header-only libraries: cpp-httplib + nlohmann/json")