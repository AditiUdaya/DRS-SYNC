#pragma once
#include <string>
#include <cstdint>

class Hash {
public:
    static uint64_t xxhash64(const void* data, size_t len);
    static uint64_t xxhash64_file(const std::string& path);
};
