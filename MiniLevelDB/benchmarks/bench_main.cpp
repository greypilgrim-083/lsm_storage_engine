#include <benchmark/benchmark.h>
#include "db_impl.h"
#include "metrics.h"
#include <filesystem>
#include <random>
#include <iostream>

using namespace minidb;

std::string GenerateRandomString(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result(length, '\0');
    for (int i = 0; i < length; ++i) {
        result[i] = charset[rand() % charset.length()];
    }
    return result;
}

static void BM_LSM_HeavyWrite(benchmark::State& state) {
    std::string db_path = "bench_db_dir";
    std::filesystem::remove_all(db_path);
    
    Metrics::GetInstance().Reset();
    
    DBImpl* db = new DBImpl(db_path);
    
    std::vector<std::string> keys;
    std::vector<std::string> values;
    for (int i = 0; i < state.range(0); ++i) {
        keys.push_back("user_key_" + std::to_string(i));
        values.push_back(GenerateRandomString(256));
    }

    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            db->Put(keys[i], values[i]);
        }
    }
    
    // Benchmark Read performance after writes
    for (int i = 0; i < state.range(0); ++i) {
        db->Get(keys[i]); // Should hit mostly bloom filters / memtable
    }

    delete db; // Closes and flushes

    state.counters["WriteAmplification"] = Metrics::GetInstance().GetWriteAmplification();
    state.counters["ReadBloomFilterDropRate"] = Metrics::GetInstance().GetReadAmplification();
    state.counters["p99_Put_Latency_us"] = Metrics::GetInstance().GetP99PutLatency();
    state.counters["p99_Get_Latency_us"] = Metrics::GetInstance().GetP99GetLatency();
    state.counters["Disk_Bytes_Written"] = Metrics::GetInstance().disk_bytes_written.load();
    
    std::filesystem::remove_all(db_path);
}

// Benchmark 10,000 sequential inserts
BENCHMARK(BM_LSM_HeavyWrite)->Arg(10000)->Unit(benchmark::kMillisecond);


static void BM_LSM_RandomBottleneck(benchmark::State& state) {
    std::string db_path = "bench_db_random";
    std::filesystem::remove_all(db_path);
    
    Metrics::GetInstance().Reset();
    DBImpl* db = new DBImpl(db_path);
    
    std::vector<std::string> random_keys;
    std::vector<std::string> missing_keys;
    std::vector<std::string> values;
    
    // Generate COMPLETELY RANDOM keys (worst case for LSM trees)
    for (int i = 0; i < state.range(0); ++i) {
        random_keys.push_back(GenerateRandomString(16)); // Random 16-char key
        missing_keys.push_back("MISSING_" + GenerateRandomString(16)); 
        values.push_back(GenerateRandomString(256));
    }

    for (auto _ : state) {
        // 1. Heavy Random Writes (Forces intense background compaction)
        for (int i = 0; i < state.range(0); ++i) {
            db->Put(random_keys[i], values[i]);
            if (i > 0 && i % 10000 == 0) {
                std::cerr << "Put Progress: " << i << " / " << state.range(0) << "\n";
            }
        }
        
        // 2. Heavy Point Reads for MISSING keys (Tests the true power of the Bloom Filter)
        for (int i = 0; i < state.range(0); ++i) {
            db->Get(missing_keys[i]); 
            if (i > 0 && i % 10000 == 0) {
                std::cerr << "Get Progress: " << i << " / " << state.range(0) << "\n";
            }
        }
    }

    delete db; // Closes and flushes

    state.counters["WriteAmplification"] = Metrics::GetInstance().GetWriteAmplification();
    state.counters["ReadBloomFilterDropRate"] = Metrics::GetInstance().GetReadAmplification();
    state.counters["p99_Put_Latency_us"] = Metrics::GetInstance().GetP99PutLatency();
    state.counters["p99_Get_Latency_us"] = Metrics::GetInstance().GetP99GetLatency();
    state.counters["Disk_Bytes_Written"] = Metrics::GetInstance().disk_bytes_written.load();
    
    std::filesystem::remove_all(db_path);
}

// Stress test with 100,000 random operations
BENCHMARK(BM_LSM_RandomBottleneck)->Arg(100000)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
