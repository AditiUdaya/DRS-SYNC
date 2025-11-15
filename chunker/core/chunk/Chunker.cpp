#include "Chunker.hpp"

#include <fstream>
#include <system_error>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace core {
namespace chunk {

uint64_t Chunker::fileSize(const std::string &filepath) {
    std::error_code ec;
    auto sz = fs::file_size(filepath, ec);
    if (ec) {
        std::stringstream ss;
        ss << "Chunker::fileSize: cannot stat file '" << filepath << "': " << ec.message();
        throw std::runtime_error(ss.str());
    }
    return static_cast<uint64_t>(sz);
}

std::vector<Chunk> Chunker::splitFile(const std::string &filepath, std::size_t chunkSize) {
    uint64_t sz = fileSize(filepath);
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Chunker::splitFile: cannot open file: " + filepath);
    }

    uint32_t totalChunks = static_cast<uint32_t>((sz + chunkSize - 1) / chunkSize);
    std::vector<Chunk> out;
    out.reserve(totalChunks);

    for (uint32_t i = 0; i < totalChunks; ++i) {
        uint64_t offset = static_cast<uint64_t>(i) * chunkSize;
        uint64_t remain = (sz > offset) ? (sz - offset) : 0;
        uint32_t thisSize = static_cast<uint32_t>(std::min<uint64_t>(chunkSize, remain));

        Chunk c;
        c.index = i;
        c.offset = offset;
        c.size = thisSize;
        c.data.resize(thisSize);

        if (thisSize > 0) {
            ifs.seekg(offset, std::ios::beg);
            if (!ifs.read(c.data.data(), thisSize)) {
                throw std::runtime_error("Chunker::splitFile: read failed at chunk " + std::to_string(i));
            }
        }

        out.push_back(std::move(c));
    }

    return out;
}

void Chunker::streamFile(const std::string &filepath,
                         const std::function<bool(const Chunk &chunk)> &onChunk,
                         std::size_t chunkSize) {
    uint64_t sz = fileSize(filepath);
    std::ifstream ifs(filepath, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Chunker::streamFile: cannot open file: " + filepath);
    }

    uint32_t index = 0;
    uint64_t offset = 0;

    while (offset < sz) {
        uint64_t remain = sz - offset;
        uint32_t thisSize = static_cast<uint32_t>(std::min<uint64_t>(chunkSize, remain));

        Chunk c;
        c.index = index;
        c.offset = offset;
        c.size = thisSize;
        c.data.resize(thisSize);

        ifs.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        if (!ifs.read(c.data.data(), thisSize)) {
            throw std::runtime_error("Chunker::streamFile: read failed at chunk " + std::to_string(index));
        }

        // if callback returns false, stop streaming early
        bool keepGoing = onChunk(c);
        if (!keepGoing) return;

        offset += thisSize;
        ++index;
    }
}

uint32_t Chunker::numChunks(const std::string &filepath, std::size_t chunkSize) {
    uint64_t sz = fileSize(filepath);
    return static_cast<uint32_t>((sz + chunkSize - 1) / chunkSize);
}

bool Chunker::writeChunkToDisk(const std::string &outpath, const Chunk &chunk) {
    std::ofstream ofs(outpath, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;

    // Simple on-disk format: index(4) offset(8) size(4) data
    uint32_t idx = chunk.index;
    uint64_t off = chunk.offset;
    uint32_t sz = chunk.size;

    ofs.write(reinterpret_cast<const char*>(&idx), sizeof(idx));
    ofs.write(reinterpret_cast<const char*>(&off), sizeof(off));
    ofs.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
    if (sz > 0) ofs.write(chunk.data.data(), sz);

    return ofs.good();
}

Chunk Chunker::readChunkFromDisk(const std::string &inpath) {
    std::ifstream ifs(inpath, std::ios::binary);
    if (!ifs) throw std::runtime_error("Chunker::readChunkFromDisk: cannot open file: " + inpath);

    Chunk c;
    uint32_t idx = 0;
    uint64_t off = 0;
    uint32_t sz = 0;

    ifs.read(reinterpret_cast<char*>(&idx), sizeof(idx));
    ifs.read(reinterpret_cast<char*>(&off), sizeof(off));
    ifs.read(reinterpret_cast<char*>(&sz), sizeof(sz));

    if (!ifs) throw std::runtime_error("Chunker::readChunkFromDisk: header read failed: " + inpath);

    c.index = idx;
    c.offset = off;
    c.size = sz;
    c.data.resize(sz);
    if (sz > 0) {
        ifs.read(c.data.data(), sz);
        if (!ifs) throw std::runtime_error("Chunker::readChunkFromDisk: data read failed: " + inpath);
    }
    return c;
}

} // namespace chunk
} // namespace core
