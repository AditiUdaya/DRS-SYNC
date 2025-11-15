#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace drs_sync {

class Integrity {
public:
    // xxHash32 for chunk checksums (fast)
    static uint32_t xxhash32(const uint8_t* data, size_t length, uint32_t seed = 0);
    
    // BLAKE3 for file hashes (secure) - simplified version using SHA256
    static std::string file_hash(const std::string& filepath);
    
    // Verify chunk checksum
    static bool verify_chunk(const uint8_t* data, size_t length, uint32_t expected_checksum);
};

} // namespace drs_sync