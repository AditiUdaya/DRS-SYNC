cmake_minimum_required(VERSION 3.10)
project(udp_reliable_transfer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows needs Winsock
if (WIN32)
    set(WSOCK_LIB ws2_32)
else()
    set(WSOCK_LIB "")
endif()

# Include this dir for headers
include_directories(${CMAKE_SOURCE_DIR})

# ---- Selective Repeat executables ----

add_executable(udp_sender_sr
    ${CMAKE_SOURCE_DIR}/udp_sender_sr.cpp
    ${CMAKE_SOURCE_DIR}/reliable_packet.hpp
)

target_link_libraries(udp_sender_sr ${WSOCK_LIB})

add_executable(udp_receiver_sr
    ${CMAKE_SOURCE_DIR}/udp_receiver_sr.cpp
    ${CMAKE_SOURCE_DIR}/reliable_packet.hpp
)

target_link_libraries(udp_receiver_sr ${WSOCK_LIB})

# ---- (Stop-and-wait code removed) ----
# To avoid CMake parsing errors, do NOT leave partial lines.
# Just delete the old executables entirely.

