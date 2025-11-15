#include "drs_sync/integrity.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace drs_sync {

// Simple xxHash32 implementation
uint32_t Integrity::xxhash32(const uint8_t* data, size_t length, uint32_t seed) {
    const uint32_t PRIME1 = 2654435761U;
    const uint32_t PRIME2 = 2246822519U;
    const uint32_t PRIME3 = 3266489917U;
    const uint32_t PRIME4 = 668265263U;
    const uint32_t PRIME5 = 374761393U;
    
    uint32_t h32 = seed + PRIME5 + static_cast<uint32_t>(length);
    
    for (size_t i = 0; i < length; i++) {
        h32 += data[i] * PRIME5;
        h32 = ((h32 << 11) | (h32 >> 21)) * PRIME1;
    }
    
    h32 ^= h32 >> 15;
    h32 *= PRIME2;
    h32 ^= h32 >> 13;
    h32 *= PRIME3;
    h32 ^= h32 >> 16;
    
    return h32;
}

// Simplified file hash using basic checksum
std::string Integrity::file_hash(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";
    
    uint64_t hash = 0;
    char buffer[8192];
    
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        size_t count = file.gcount();
        for (size_t i = 0; i < count; i++) {
            hash = hash * 31 + static_cast<uint8_t>(buffer[i]);
        }
    }
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

bool Integrity::verify_chunk(const uint8_t* data, size_t length, 
                             uint32_t expected_checksum) {
    uint32_t actual = xxhash32(data, length);
    return actual == expected_checksum;
}

} // namespace drs_sync