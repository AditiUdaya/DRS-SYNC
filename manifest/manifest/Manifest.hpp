#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace core::manifest {

struct Manifest {
    std::string file_id;
    std::string filename;
    uint64_t file_size = 0;
    uint32_t chunk_size = 0;
    uint32_t total_chunks = 0;

    std::vector<int> received_chunks;
    bool completed = false;

    // ---- Add this helper ----
    void markChunkReceived(int c) {
        if (c < 0 || c >= (int)total_chunks) return;
        for (int x : received_chunks) if (x == c) return;
        received_chunks.push_back(c);
        if (received_chunks.size() == total_chunks) completed = true;
    }
};

} // namespace core::manifest
