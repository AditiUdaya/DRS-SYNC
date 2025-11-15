# DRS-SYNC


---

# ### Chunking Module (`core/chunk`)

The **Chunking Module** provides deterministic, fixed-size splitting of large files into smaller blocks. It forms the foundation for manifest creation, hashing, checkpointing, and reliable UDP file transfer.

## Features

### **1. Fixed-Size Chunk Splitting**

Splits any file into 64KB chunks by default (configurable).
Each chunk contains:

* `id` (index)
* `offset` (byte position)
* `size`
* raw buffer data

### **2. Supports Arbitrary File Sizes**

Handles files ranging from a few KB to multiple GB.
Automatically computes chunk count:

```
total_chunks = ceil(file_size / chunk_size)
```

### **3. Zero Copy Logic**

Efficiently reads file data directly into chunk buffers.
No unnecessary intermediate allocations.

### **4. Backed by Simple, Safe C++ API**

Example:

```cpp
Chunker c("bigfile.txt", 65536);
ChunkInfo info = c.get_chunk(0);
```

### **5. Integrates with Hashing and Manifest**

Designed so each chunk can be hashed and serialized for storage, transmission, or resume logic.

---

# ### Hashing Module (`core/hash`)

The **Hashing Module** provides high-performance, 64-bit hashing for file chunks and entire files, using the xxHash algorithm.

## Features

### **1. Ultra-Fast xxHash64**

Uses xxHash64 (via `xxhash.c`) for extremely fast, non-cryptographic hashing.

* Multi-GB/s throughput
* Deterministic 64-bit hashes
* Zero external dependencies

### **2. In-Memory Buffer Hashing**

Hash any byte buffer:

```cpp
uint64_t h = Hash::xxhash64(ptr, length);
```

Used for:

* Chunk integrity
* Network payload verification

### **3. File Hashing**

Stream-hashes entire files, no matter the size:

```cpp
uint64_t h = Hash::xxhash64_file("bigfile.txt");
```

Ensures complete file integrity on both sender & receiver.

### **4. Cross-Platform Support**

Fully supported on:

* Windows (MSYS2 / MinGW)
* Linux
* macOS

### **5. Designed for Integration**

Works seamlessly with:

* Chunker
* Manifest system
* Reliable UDP protocol
* Resume & checkpoint logic

---


