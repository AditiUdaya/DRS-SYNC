#include <iostream>
#include "core/hash/Hash.hpp"
#include <cstring>

int main() {
    const char* msg = "Hello world";
    uint64_t h1 = Hash::xxhash64(msg, strlen(msg));

    std::cout << "xxHash64(\"Hello world\") = " << h1 << "\n";

    uint64_t file_hash = Hash::xxhash64_file("bigfile.txt");
    std::cout << "xxHash64(bigfile.txt) = " << file_hash << "\n";

    return 0;
}
