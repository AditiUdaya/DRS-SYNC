#include "core/chunk/Chunker.hpp"
#include <iostream>

int main() {
    try {
        std::cout << "Testing Chunker on bigfile.txt\n";

        // Split into chunks
        auto chunks = core::chunk::Chunker::splitFile("bigfile.txt");

        std::cout << "Total Chunks = " << chunks.size() << "\n\n";

        for (const auto& c : chunks) {
            std::cout << "Chunk " << c.index
                      << " | offset=" << c.offset
                      << " | size=" << c.size << " bytes\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
    }

    return 0;
}
