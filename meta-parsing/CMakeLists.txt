cmake_minimum_required(VERSION 3.10)
project(drogon_test)

set(CMAKE_CXX_STANDARD 17)

find_package(Drogon CONFIG REQUIRED)

# ---- Add BLAKE3 Library ----
add_library(blake3 STATIC
    libs/blake3.c
    libs/blake3_dispatch.c
    libs/blake3_portable.c
)
target_include_directories(blake3 PUBLIC libs)

# ---- Executable (ONLY ONE target) ----
add_executable(drogon_test
    main.cpp
    ping_controller.cpp
    manifest_controller.cpp
)

target_link_libraries(drogon_test PRIVATE
    Drogon::Drogon
    blake3
)
# -------- Phase 3: UDP tools (sender & receiver) ----------
add_executable(udp_receiver udp_receiver.cpp)
target_link_libraries(udp_receiver PRIVATE ws2_32)

add_executable(udp_sender udp_sender.cpp)
target_link_libraries(udp_sender PRIVATE ws2_32)
add_executable(udp_sender_reliable
    ${CMAKE_SOURCE_DIR}/udp_sender_reliable.cpp
    ${CMAKE_SOURCE_DIR}/reliable_packet.hpp
)

target_link_libraries(udp_sender_reliable PRIVATE ws2_32)

add_executable(udp_receiver_reliable udp_receiver_reliable.cpp reliable_packet.hpp)
target_link_libraries(udp_receiver_reliable PRIVATE ws2_32)
