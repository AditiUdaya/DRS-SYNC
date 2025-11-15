#pragma once
#include "Manifest.hpp"
#include <string>

namespace core::manifest {

struct ManifestManager {

    static void saveManifest(const Manifest& m);
    static Manifest loadManifest(const std::string& file_id);

};

} // namespace core::manifest
