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
)

target_link_libraries(drogon_test PRIVATE
    Drogon::Drogon
    blake3
)
