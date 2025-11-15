cmake_minimum_required(VERSION 3.10)
project(udp_reliable_transfer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Windows networking library
# ---------------------------------------------------------
if (WIN32)
    set(WSOCK_LIB ws2_32)
else()
    set(WSOCK_LIB "")
endif()

# ---------------------------------------------------------
# Include root-level headers
# ---------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR})

# ---------------------------------------------------------
# xxHash (local version)
# ---------------------------------------------------------
add_library(xxhash STATIC
    ${CMAKE_SOURCE_DIR}/libs/xxhash.c
)
target_include_directories(xxhash PUBLIC ${CMAKE_SOURCE_DIR}/libs)

# ---------------------------------------------------------
# nlohmann_json (installed via MSYS2 pacman)
# ---------------------------------------------------------
find_package(nlohmann_json CONFIG REQUIRED)

# ---------------------------------------------------------
# Add backend core library (chunker + hasher + manifest)
# ---------------------------------------------------------
add_subdirectory(core)

# ---------------------------------------------------------
# Test executables
# ---------------------------------------------------------
add_executable(test_chunker
    ${CMAKE_SOURCE_DIR}/test_chunker.cpp
)
target_link_libraries(test_chunker PRIVATE backend_core)

add_executable(test_hash
    ${CMAKE_SOURCE_DIR}/test_hash.cpp
)
target_link_libraries(test_hash PRIVATE backend_core)

# ---------------------------------------------------------
# UDP Selective Repeat Sender/Receiver
# ---------------------------------------------------------
add_executable(udp_sender_sr
    ${CMAKE_SOURCE_DIR}/udp_sender_sr.cpp
    ${CMAKE_SOURCE_DIR}/reliable_packet.hpp
)
target_link_libraries(udp_sender_sr PRIVATE backend_core ${WSOCK_LIB})

add_executable(udp_receiver_sr
    ${CMAKE_SOURCE_DIR}/udp_receiver_sr.cpp
    ${CMAKE_SOURCE_DIR}/reliable_packet.hpp
)
target_link_libraries(udp_receiver_sr PRIVATE backend_core ${WSOCK_LIB})
add_executable(test_manifest_basic test_manifest_basic.cpp)
target_link_libraries(test_manifest_basic PRIVATE backend_core nlohmann_json::nlohmann_json)

