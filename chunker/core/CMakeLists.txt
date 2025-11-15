cmake_minimum_required(VERSION 3.10)

# Create a library for the backend core
add_library(backend_core
    chunk/Chunker.cpp
)

target_include_directories(backend_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
