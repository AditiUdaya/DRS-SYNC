#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace core {
namespace chunk {

struct Chunk {
    uint32_t index = 0;                // chunk index starting from 0
    uint64_t offset = 0;               // offset in original file
    uint32_t size = 0;                 // bytes in this chunk
    std::vector<char> data;            // payload bytes (empty for streamed mode if not retained)

    // optional metadata fields can be added (e.g. hash, compressed flag)
};

class Chunker {
public:
    // Split whole file into chunks (loads all chunks into memory).
    // Throws std::runtime_error on error.
    // chunkSize default = 64*1024 (64 KiB)
    static std::vector<Chunk> splitFile(const std::string &filepath, std::size_t chunkSize = 64 * 1024);

    // Streaming API: call `onChunk` for every chunk found.
    // onChunk receives a Chunk by reference; if callback returns false, iteration stops early.
    // This API avoids retaining all chunks in memory.
    // Throws std::runtime_error on error.
    static void streamFile(const std::string &filepath,
                           const std::function<bool(const Chunk &chunk)> &onChunk,
                           std::size_t chunkSize = 64 * 1024);

    // Convenience: number of chunks a file will be split into (does not read file contents).
    // Throws std::runtime_error if file not accessible.
    static uint32_t numChunks(const std::string &filepath, std::size_t chunkSize = 64 * 1024);

    // Persist a single chunk to disk as binary file (path provided).
    // Returns true on success.
    static bool writeChunkToDisk(const std::string &outpath, const Chunk &chunk);

    // Load a chunk previously written by writeChunkToDisk().
    // Throws std::runtime_error on error.
    static Chunk readChunkFromDisk(const std::string &inpath);

private:
    // helper: open file and get size (throws on error)
    static uint64_t fileSize(const std::string &filepath);
};

} // namespace chunk
} // namespace core
