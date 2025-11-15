#include "Hash.hpp"
#include <fstream>
#include <vector>
#include <cstring>
#include "../../libs/xxhash.h"

uint64_t Hash::xxhash64(const void* data, size_t len) {
    return XXH64(data, len, 0);
}

uint64_t Hash::xxhash64_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;

    std::vector<char> buffer(65536);

    XXH64_state_t* st = XXH64_createState();
    XXH64_reset(st, 0);

    while (f) {
        f.read(buffer.data(), buffer.size());
        size_t n = f.gcount();
        if (n > 0) {
            XXH64_update(st, buffer.data(), n);
        }
    }

    uint64_t hash = XXH64_digest(st);

    XXH64_freeState(st);
    return hash;
}
