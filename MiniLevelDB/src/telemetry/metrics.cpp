#include "metrics.h"
#include <algorithm>

namespace minidb {

void Metrics::RecordPutLatency(double latency_us) {
    std::lock_guard<std::mutex> lock(latency_mutex);
    put_latencies.push_back(latency_us);
}

void Metrics::RecordGetLatency(double latency_us) {
    std::lock_guard<std::mutex> lock(latency_mutex);
    get_latencies.push_back(latency_us);
}

double Metrics::GetWriteAmplification() const {
    if (user_bytes_written == 0) return 0.0;
    return static_cast<double>(disk_bytes_written) / user_bytes_written;
}

double Metrics::GetReadAmplification() const {
    // Read Amp = Total tables searched / Total reads
    // Since we don't track total reads directly here, we approximate by
    // sstables_queried / (sstables_queried - bloom_filter_drops + 1)
    // We will refine this later. For now, we return bloom filter drop rate.
    if (sstables_queried == 0) return 0.0;
    return static_cast<double>(sstables_queried - bloom_filter_drops) / sstables_queried;
}

double Metrics::GetP99PutLatency() {
    std::lock_guard<std::mutex> lock(latency_mutex);
    if (put_latencies.empty()) return 0.0;
    size_t idx = static_cast<size_t>(put_latencies.size() * 0.99);
    std::nth_element(put_latencies.begin(), put_latencies.begin() + idx, put_latencies.end());
    return put_latencies[idx];
}

double Metrics::GetP99GetLatency() {
    std::lock_guard<std::mutex> lock(latency_mutex);
    if (get_latencies.empty()) return 0.0;
    size_t idx = static_cast<size_t>(get_latencies.size() * 0.99);
    std::nth_element(get_latencies.begin(), get_latencies.begin() + idx, get_latencies.end());
    return get_latencies[idx];
}

void Metrics::Reset() {
    user_bytes_written = 0;
    disk_bytes_written = 0;
    sstables_queried = 0;
    bloom_filter_drops = 0;
    std::lock_guard<std::mutex> lock(latency_mutex);
    put_latencies.clear();
    get_latencies.clear();
}

ScopedTimer::ScopedTimer(OpType type) : type_(type), start_(std::chrono::high_resolution_clock::now()) {}

ScopedTimer::~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    double latency_us = std::chrono::duration<double, std::micro>(end - start_).count();
    if (type_ == OpType::PUT) {
        Metrics::GetInstance().RecordPutLatency(latency_us);
    } else {
        Metrics::GetInstance().RecordGetLatency(latency_us);
    }
}

} // namespace minidb
