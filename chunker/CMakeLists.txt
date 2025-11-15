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

include_directories(${CMAKE_SOURCE_DIR})

# ---- Backend Core ----
add_subdirectory(core)

# ---- Selective Repeat executables ----

add_executable(udp_sender_sr
    ${CMAKE_SOURCE_DIR}/udp_sender_sr.cpp
)

target_link_libraries(udp_sender_sr
    PRIVATE backend_core
    PRIVATE ${WSOCK_LIB}
)

add_executable(udp_receiver_sr
    ${CMAKE_SOURCE_DIR}/udp_receiver_sr.cpp
)

target_link_libraries(udp_receiver_sr
    PRIVATE backend_core
    PRIVATE ${WSOCK_LIB}
)
# ---- Test Chunker ----
add_executable(test_chunker ${CMAKE_SOURCE_DIR}/test_chunker.cpp)
target_link_libraries(test_chunker PRIVATE backend_core)
