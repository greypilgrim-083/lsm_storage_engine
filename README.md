# MiniLevelDB — LSM Storage Engine

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Build](https://img.shields.io/badge/build-CMake%203.14%2B-brightgreen.svg)
![Testing](https://img.shields.io/badge/testing-GoogleTest-orange.svg)
![Benchmarks](https://img.shields.io/badge/benchmarks-Google%20Benchmark-red.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**MiniLevelDB** is a lightweight, high-performance, embedded Log-Structured Merge-tree (LSM-tree) key-value storage engine implemented in modern **C++17**. Inspired by the architectures of **LevelDB** and **RocksDB**, MiniLevelDB is designed to handle write-heavy workloads efficiently while offering fast read access through in-memory caching, SSTable indexing, probabilistic Bloom filters, and background level compaction.

---

## 🌟 Key Features

- ⚡ **High Write Throughput**: Converts random writes into fast, sequential disk writes using an in-memory buffer (MemTable) paired with an append-only Write-Ahead Log (WAL).
- 🛡️ **ACID Durability & Recovery**: Guarantees persistence via sequential WAL logs. On startup, automatically recovers un-flushed data into the MemTable.
- 🔒 **Thread-Safe Memory Layer**: Uses standard `std::shared_mutex` read-write locks for high-concurrency `Put`, `Get`, and `Delete` operations.
- 💾 **Immutable SSTables (Sorted String Tables)**: Flushes sorted data blocks to disk with explicit sparse indexing for binary search retrieval.
- 🌸 **Probabilistic Bloom Filters**: Embedded into SSTable footers to dramatically eliminate unnecessary disk reads for non-existent keys, minimizing Read Amplification.
- 🔄 **Asynchronous Compaction Engine**: Runs a dedicated background compaction thread (`BackgroundCompactionLoop`) to merge overlapping SSTables across LSM levels (L0, L1, etc.), purging tombstones and old versions to reclaim disk space and control Read/Space Amplification.
- 📊 **Real-time Telemetry & Metrics**: Native performance counters (`Metrics` singleton & `ScopedTimer`) measuring:
  - **Write Amplification Factor (WAF)**
  - **Read Amplification / Bloom Filter Drop Rate**
  - **P99 Put & Get Latency (microseconds)**
  - **Disk I/O Bytes Written**
- 🧪 **Comprehensive Benchmarking & Testing**: Ships with GoogleTest suites for functionality verification and Google Benchmark microbenchmarks for performance analysis under synthetic & random workloads.

---

## 🏗️ Storage Engine Architecture

The architecture follows standard Log-Structured Merge-tree principles:

```mermaid
flowchart TD
    subgraph Write Path
        ClientWrite["Client (Put / Delete)"]
        WAL["Write-Ahead Log (WAL)"]
        MemTable["In-Memory MemTable (std::map)"]
        ClientWrite -->|1. Append Log| WAL
        ClientWrite -->|2. Insert/Update| MemTable
    end

    subgraph Memory Flush
        MemTable -->|Flush when size > threshold| SST_L0["Level 0 SSTables (Disk)"]
    end

    subgraph Read Path
        ClientRead["Client (Get)"]
        ClientRead -->|1. Search| MemTable
        MemTable -->|2. Miss| SST_L0
        SST_L0 -->|3. Check Filter| BloomFilter{"Bloom Filter"}
        BloomFilter -->|Hit| SSTIndex["SST Index Lookup"]
        BloomFilter -->|Miss (Drop)| ReturnNull["Return Not Found"]
        SSTIndex --> DiskRead["Read Data Block"]
    end

    subgraph Background Compaction
        SST_L0 -->|Compaction Thread| SST_L1["Level 1 SSTables (Disk)"]
        SST_L1 -->|Merge & Deduplicate| SST_L2["Level 2 SSTables (Disk)"]
    end
```

### Component Breakdown

| Component | Responsibility | Technical Details |
| :--- | :--- | :--- |
| **`MemTable`** | Write buffer in RAM | Backed by `std::map`, thread-safe via `std::shared_mutex` |
| **`WAL`** | Durability log on disk | Sequential append-only file with recovery parsing |
| **`SSTableBuilder`** | Writes sorted KV blocks | Constructs sorted data blocks, index offsets, and Bloom Filter |
| **`SSTableReader`** | Reads SSTables from disk | Caches index block and deserializes Bloom Filter footer |
| **`BloomFilter`** | Probabilistic membership check | Multi-hash function bit-array (default 1% false positive rate) |
| **`CompactionJob`** | SSTable merging | Merges overlapping multi-file key ranges and purges tombstones |
| **`Metrics`** | Engine telemetry | Tracks disk bytes, user bytes, latency distributions, and drop rates |

---

## 📁 Repository Structure

```
MiniLevelDB/
├── CMakeLists.txt            # Main CMake build configuration (Fetches GTest & GBenchmark)
├── include/                  # Public Header Files
│   ├── bloom_filter.h        # Probabilistic filter declaration
│   ├── compaction_job.h      # Multi-SSTable merging interface
│   ├── db_impl.h             # Core DBImpl engine interface
│   ├── memtable.h            # In-memory skip/map table
│   ├── metrics.h             # Telemetry & P99 latency tracking
│   ├── sstable_builder.h     # SSTable disk builder
│   ├── sstable_reader.h      # SSTable disk reader & index lookup
│   └── wal.h                 # Write-Ahead Log interface
├── src/                      # Implementation Files
│   ├── CMakeLists.txt
│   ├── core/
│   │   └── db_impl.cpp       # Main database logic & background compaction worker
│   ├── mem/
│   │   ├── memtable.cpp      # Concurrent MemTable operations
│   │   └── wal.cpp           # WAL logging & crash recovery
│   ├── disk/
│   │   ├── bloom_filter.cpp  # Bit vector & multi-hashing implementation
│   │   ├── sstable_builder.cpp # Disk block serialisation
│   │   └── sstable_reader.cpp  # SSTable deserialization & binary search
│   ├── compaction/
│   │   └── compaction_job.cpp # Merge-sort compaction execution
│   └── telemetry/
│       └── metrics.cpp       # Telemetry aggregation & latency calculations
├── tests/
│   ├── CMakeLists.txt
│   └── test_main.cpp         # GoogleTest suite (Put/Get, Delete, WAL, Flush)
└── benchmarks/
    ├── CMakeLists.txt
    └── bench_main.cpp        # Google Benchmark suite (Write amp, P99 latency, Bloom Filter efficiency)
```

---

## 🚀 Quick Start & Usage

### C++ Example

```cpp
#include "db_impl.h"
#include "metrics.h"
#include <iostream>

int main() {
    // 1. Initialize DB at path
    minidb::DBImpl db("./data_dir");

    // 2. Insert Key-Value pairs
    db.Put("user_1001", "Alice");
    db.Put("user_1002", "Bob");

    // 3. Retrieve values
    auto val = db.Get("user_1001");
    if (val.has_value()) {
        std::cout << "Found user_1001: " << val.value() << std::endl;
    }

    // 4. Update / Delete keys
    db.Delete("user_1002");
    
    // 5. Inspect Engine Telemetry
    auto& metrics = minidb::Metrics::GetInstance();
    std::cout << "Write Amplification: " << metrics.GetWriteAmplification() << std::endl;
    std::cout << "P99 Put Latency (us): " << metrics.GetP99PutLatency() << std::endl;

    return 0; // Automatic close, flushes MemTable & gracefully stops compaction thread
}
```

---

## 🛠️ Building & Running on WSL (Linux)

### 1. Prerequisites (WSL / Ubuntu)

Install standard C++ build tools, GCC, CMake, and Git inside WSL:

```bash
sudo apt update
sudo apt install -y build-essential cmake git g++
```

### 2. Building from Source (WSL Terminal)

Navigate to your workspace directory inside WSL and compile using CMake:

```bash
# Navigate to the project directory
cd MiniLevelDB

# Create and configure build directory
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Compile target binaries using all CPU cores
make -j$(nproc)
```

### 3. Running Unit Tests in WSL

Unit tests are built using **GoogleTest** (automatically fetched via CMake):

```bash
# From the build directory:
ctest --output-on-failure

# Or run the GoogleTest binary directly:
./tests/minidb_tests
```

### 4. Running Microbenchmarks in WSL

Microbenchmarks are powered by **Google Benchmark**:

```bash
# From the build directory:
./benchmarks/minidb_bench
```

---

## 💻 Building Native Windows (Powershell / Command Prompt)

### Prerequisites
- **C++ Compiler**: `GCC` (MinGW), `Clang`, or `MSVC 2019+`
- **CMake**: `v3.14` or higher

```powershell
# Navigate to project directory
cd MiniLevelDB

# Configure & build with CMake
cmake -B build -S .
cmake --build build --config Release

# Run tests
cd build
ctest --output-on-failure
.\tests\minidb_tests.exe

# Run benchmarks
.\benchmarks\minidb_bench.exe
```

---

## 📈 Benchmark Results & Performance Profile

### Benchmark Output (`Google Benchmark`)

```text
------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Benchmark                               Time             CPU   Iterations   Disk_Bytes_Written   ReadBloomFilterDropRate   WriteAmplification   p99_Get_Latency_us   p99_Put_Latency_us
------------------------------------------------------------------------------------------------------------------------------------------------------------------------
BM_LSM_HeavyWrite/10000               195 ms         23.3 ms           31             88.6204M                         0              1.03244                0.866              559.788
```

### Performance Analysis Summary

| Metric | Result | Analysis / Engineering Insight |
| :--- | :--- | :--- |
| **Batch Write Time** | `195 ms` total (`23.3 ms` CPU) | Demonstrates low CPU overhead with parallel background thread execution. |
| **Write Amplification (WAF)** | `1.03x` | **Near-Optimal Sequential Disk Writes**. Minimal overhead during sequential batch ingest. |
| **P99 Get Latency** | `0.866 µs` (~866 ns) | **Sub-Microsecond Read Speed**. Hits in-memory MemTable buffer directly without disk I/O. |
| **P99 Put Latency** | `559.788 µs` | Sub-millisecond write latency including append-only WAL persistence. |
| **Total Disk I/O** | `88.62 MB` | Reflects WAL log records + flushed SSTables on disk. |

---

## 📊 Performance & Telemetry Metrics

MiniLevelDB features built-in telemetry to monitor key LSM engine health indicators:

- **Write Amplification Factor (WAF)**: Ratio of total bytes written to disk (WAL + Flushes + Compactions) vs bytes submitted by user (`user_bytes_written`).
- **Read Amplification**: Tracks total SSTables queried per `Get()` operation.
- **Bloom Filter Drop Rate**: Percentage of non-existent key lookups intercepted by the Bloom Filter before touching disk.
- **P99 Latency**: Microsecond-level 99th percentile latency tracking for `Put` and `Get` calls via RAII `ScopedTimer`.

---

## 📜 License

This project is open-source software licensed under the **MIT License**.
