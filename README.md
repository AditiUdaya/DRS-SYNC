

---

# ### **Chunking Module (`core/chunk`)**

The **Chunking Module** provides deterministic, fixed-size splitting of large files into smaller blocks. It forms the foundation for manifest creation, hashing, checkpointing, and reliable UDP file transfer.

---

## **Features**

### **1. Fixed-Size Chunk Splitting**

Splits any file into 64KB chunks by default (configurable).
Each chunk contains:

* `id` (index)
* `offset` (byte position)
* `size`
* raw buffer data

---

### **2. Supports Arbitrary File Sizes**

Handles files from KB to multi-GB.

```
total_chunks = ceil(file_size / chunk_size)
```

---

### **3. Zero-Copy Logic**

Efficiently reads file data directly into buffers.
Avoids intermediate allocations → faster and memory-safe.

---

### **4. Clean, Simple C++ API**

```cpp
Chunker c("bigfile.txt", 65536);
ChunkInfo info = c.get_chunk(0);
```

---

### **5. Integrates with Hashing and Manifest**

Chunk metadata is designed for:

* hashing
* SR (Selective Repeat) window transmission
* resend logic
* manifest persistence for resume

---

---

# ### **Hashing Module (`core/hash`)**

The **Hashing Module** provides high-performance 64-bit hashing for both chunks and full files using xxHash64.

---

## **Features**

### **1. Ultra-Fast xxHash64 (100% Native)**

Powered by `xxhash.c` inside `libs/`.
Benefits:

* multi-GB/s speed
* deterministic 64-bit hash
* no external dependencies
* stable across platforms

---

### **2. Buffer Hashing**

Hash any in-memory buffer:

```cpp
uint64_t h = Hash::xxhash64(ptr, length);
```

Used for:

* verifying chunk correctness
* comparing pre/post retransmission payloads

---

### **3. File Hashing**

Stream-hash entire file:

```cpp
uint64_t h = Hash::xxhash64_file("bigfile.txt");
```

Used to confirm full file integrity after transfer.

---

### **4. Cross-Platform & Reliable**

Works on:

* Windows (MSYS2/MinGW)
* Linux
* macOS

No SIMD linking issues (unlike BLAKE3).

---

### **5. Fully Integrated**

The hashing layer supports:

* manifest creation
* SR protocol ACK validation
* complete file integrity validation

---

---


### **Implemented core backend modules: Chunker, Hashing (xxHash), and Manifest System**

---

## **1. Chunking System**

* Added `core/chunk/Chunker.*`
* Deterministic 64KB splitting
* Computes offsets, sizes, chunk count
* Zero-copy buffered reading
* Added `test_chunker` to validate correctness

---

## **2. Hashing System using xxHash64**

* Replaced problematic BLAKE3 with xxHash
* Added raw-buffer hashing + file hashing
* Pulled xxHash into `libs/xxhash.c/.h`
* Added `test_hash` for validation
* Fully cross-platform and fast

---

## **3. Manifest System**

* Added `Manifest` struct + JSON serialization
* Added `ManifestManager` for:

  * save/load
  * mark chunks
  * completion detection
* Uses `nlohmann::json`
* Stores manifest files under `/manifests/`
* Added basic tests and validated workflow

---

## **4. Project Structure Cleanup**

* Organized core modules into:

  * `core/chunk`
  * `core/hash`
  * `core/manifest`
* Updated top-level CMake:

  * proper include paths
  * added xxHash + JSON
  * added test executables

---

## **5. Build System Fixes**

* Resolved missing SIMD dependencies
* Fixed incorrect include paths
* Fixed duplicate target definitions
* Ensured consistent, reproducible build command

---

## **Outcome**

Backend core layer (Chunker + Hashing + Manifest) is complete, tested, and ready for integration with the Selective Repeat UDP sender/receiver, checkpointing, and full DRS-SYNC pipeline.

---


