#include "ManifestManager.hpp"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace core::manifest {

static const std::string MANIFEST_DIR = "manifests";

void ManifestManager::saveManifest(const Manifest& m) {
    fs::create_directories(MANIFEST_DIR);

    json j;
    j["file_id"] = m.file_id;
    j["filename"] = m.filename;
    j["file_size"] = m.file_size;
    j["chunk_size"] = m.chunk_size;
    j["total_chunks"] = m.total_chunks;
    j["received_chunks"] = m.received_chunks;
    j["completed"] = m.completed;

    std::ofstream f(MANIFEST_DIR + "/" + m.file_id + ".json");
    f << j.dump(4);
}

Manifest ManifestManager::loadManifest(const std::string& file_id) {
    Manifest m;
    std::ifstream f(MANIFEST_DIR + "/" + file_id + ".json");

    if (!f.good())
        throw std::runtime_error("Manifest file not found");

    json j;
    f >> j;

    m.file_id = j.value("file_id", "");
    m.filename = j.value("filename", "");
    m.file_size = j.value("file_size", 0ULL);
    m.chunk_size = j.value("chunk_size", 0u);
    m.total_chunks = j.value("total_chunks", 0u);
    m.received_chunks = j.value("received_chunks", std::vector<int>{});
    m.completed = j.value("completed", false);

    return m;
}

} // namespace core::manifest
