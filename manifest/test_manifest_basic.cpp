#include <iostream>
#include <cassert>
#include "core/manifest/Manifest.hpp"
#include "core/manifest/ManifestManager.hpp"
#include <fstream>
#include <iostream>

using core::manifest::Manifest;
using core::manifest::ManifestManager;

using namespace std;

void banner(const string &s) {
    cout << "\n=== " << s << " ===\n";
}

/**
 * TEST 1: Basic manifest create + values
 */
void test_basic() {
    banner("BASIC CREATE");

    Manifest m;
    m.file_id = "abc";
    m.filename = "bigfile.txt";
    m.file_size = 1048576;
    m.chunk_size = 65536;
    m.total_chunks = m.file_size / m.chunk_size;

    assert(m.total_chunks == 16);
    assert(m.received_chunks.empty());

    cout << "Basic creation OK.\n";
}

/**
 * TEST 2: Mark chunks received
 */
void test_mark() {
    banner("MARK RECEIVED");

    Manifest m;
    m.total_chunks = 10;

    m.markChunkReceived(0);
    m.markChunkReceived(3);
    m.markChunkReceived(3); // duplicate should be ignored
    m.markChunkReceived(9);

    assert(m.received_chunks.size() == 3);
    assert(!m.completed);

    cout << "Marking OK.\n";
}

/**
 * TEST 3: Completion logic
 */
void test_completion() {
    banner("COMPLETION LOGIC");

    Manifest m;
    m.total_chunks = 4;

    m.markChunkReceived(0);
    m.markChunkReceived(1);
    m.markChunkReceived(2);
    assert(!m.completed);

    m.markChunkReceived(3);
    assert(m.completed);

    cout << "Completion OK.\n";
}

/**
 * TEST 4: Save/load roundtrip
 */
void test_save_load() {
    banner("SAVE + LOAD");

    Manifest m;
    m.file_id = "round";
    m.filename = "file.bin";
    m.total_chunks = 5;

    m.markChunkReceived(1);
    m.markChunkReceived(4);

    ManifestManager::saveManifest(m);

    Manifest loaded = ManifestManager::loadManifest("round");

    assert(loaded.file_id == "round");
    assert(loaded.received_chunks.size() == 2);
    assert(loaded.received_chunks[0] == 1);
    assert(loaded.received_chunks[1] == 4);

    cout << "Roundtrip OK.\n";
}

/**
 * TEST 5: Basic corrupted file check
 */
void test_corrupted() {
    banner("CORRUPTION DETECTION");

    // create valid
    Manifest m;
    m.file_id = "bad";
    m.total_chunks = 3;

    ManifestManager::saveManifest(m);

    // corrupt the JSON
    std::ofstream f("manifests/bad.json");
    f << "INVALID JSON { { {";
    f.close();

    try {
        Manifest _ = ManifestManager::loadManifest("bad");
        cout << "ERROR: Did NOT detect corruption!\n";
    } catch (...) {
        cout << "Corruption detected OK.\n";
    }
}

int main() {
    cout << "== IMPORTANT MANIFEST TESTS ==\n";

    test_basic();
    test_mark();
    test_completion();
    test_save_load();
    test_corrupted();

    cout << "\nAll important checks passed.\n";
    return 0;
}
