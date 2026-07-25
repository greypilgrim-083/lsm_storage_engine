#pragma once

#include <atomic>
#include <vector>
#include <mutex>
#include <chrono>

namespace minidb {

class Metrics {
public:
    static Metrics& GetInstance() {
        static Metrics instance;
        return instance;
    }

    // Counters for Amplification
    std::atomic<uint64_t> user_bytes_written{0};
    std::atomic<uint64_t> disk_bytes_written{0};
    
    std::atomic<uint64_t> sstables_queried{0};
    std::atomic<uint64_t> bloom_filter_drops{0};

    // Latency Tracking
    void RecordPutLatency(double latency_us);
    void RecordGetLatency(double latency_us);

    // Compute metrics
    double GetWriteAmplification() const;
    double GetReadAmplification() const;
    
    double GetP99PutLatency();
    double GetP99GetLatency();

    void Reset();

private:
    Metrics() = default;
    
    std::mutex latency_mutex;
    std::vector<double> put_latencies;
    std::vector<double> get_latencies;
};

// RAII Timer for latency tracking
class ScopedTimer {
public:
    enum class OpType { PUT, GET };
    ScopedTimer(OpType type);
    ~ScopedTimer();
private:
    OpType type_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

} // namespace minidb
