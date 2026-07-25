#include "db_impl.h"
#include "sstable_builder.h"
#include "sstable_reader.h"
#include "compaction_job.h"
#include "metrics.h"
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace minidb {

DBImpl::DBImpl(const std::string& db_path) : db_path_(db_path) {
    std::filesystem::create_directories(db_path_);
    
    levels_.resize(2); // L0 and L1

    wal_ = std::make_unique<WAL>(db_path_ + "/wal.log");
    wal_->Recover(memtable_);

    // Load existing SSTables
    for (const auto& entry : std::filesystem::directory_iterator(db_path_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sst") {
            std::string filename = entry.path().filename().string();
            if (filename.find("L0_") == 0) {
                levels_[0].push_back(entry.path().string());
            } else if (filename.find("L1_") == 0) {
                levels_[1].push_back(entry.path().string());
            }
            
            size_t underscore = filename.find('_');
            size_t dot = filename.find('.');
            if (underscore != std::string::npos && dot != std::string::npos) {
                uint64_t id = std::stoull(filename.substr(underscore + 1, dot - underscore - 1));
                if (id >= next_file_id_) {
                    next_file_id_ = id + 1;
                }
            }
        }
    }
    
    auto sort_func = [](const std::string& a, const std::string& b) {
        auto get_id = [](const std::string& path) {
            std::string filename = std::filesystem::path(path).filename().string();
            size_t underscore = filename.find('_');
            size_t dot = filename.find('.');
            return std::stoull(filename.substr(underscore + 1, dot - underscore - 1));
        };
        return get_id(a) < get_id(b);
    };
    std::sort(levels_[0].begin(), levels_[0].end(), sort_func);
    std::sort(levels_[1].begin(), levels_[1].end(), sort_func);

    // Start background compaction thread
    compaction_thread_ = std::thread(&DBImpl::BackgroundCompactionLoop, this);
}

DBImpl::~DBImpl() {
    stop_compaction_ = true;
    compaction_cv_.notify_all();
    if (compaction_thread_.joinable()) {
        compaction_thread_.join();
    }
    
    // Flush remaining memtable on exit
    if (!memtable_.IsEmpty()) {
        FlushMemTable();
    }
}

void DBImpl::Put(const std::string& key, const std::string& value) {
    ScopedTimer timer(ScopedTimer::OpType::PUT);
    
    std::unique_lock lock(mutex_);
    wal_->Append(key, value);
    memtable_.Put(key, value);

    if (memtable_.ApproximateSize() > 4 * 1024 * 1024) { // 4MB threshold
        FlushMemTable();
    }
}

void DBImpl::Delete(const std::string& key) {
    Put(key, "");
}

std::optional<std::string> DBImpl::Get(const std::string& key) {
    ScopedTimer timer(ScopedTimer::OpType::GET);
    
    std::unique_lock lock(mutex_);
    
    // 1. Check MemTable
    auto val = memtable_.Get(key);
    if (val.has_value()) {
        if (val.value().empty()) return std::nullopt; // Tombstone
        return val;
    }
    
    // 2. Check L0 (Newest to Oldest)
    for (auto it = levels_[0].rbegin(); it != levels_[0].rend(); ++it) {
        SSTableReader reader(*it);
        auto sst_val = reader.Get(key);
        if (sst_val.has_value()) {
            if (sst_val.value().empty()) return std::nullopt;
            return sst_val;
        }
    }

    // 3. Check L1 (Newest to Oldest)
    for (auto it = levels_[1].rbegin(); it != levels_[1].rend(); ++it) {
        SSTableReader reader(*it);
        auto sst_val = reader.Get(key);
        if (sst_val.has_value()) {
            if (sst_val.value().empty()) return std::nullopt;
            return sst_val;
        }
    }

    return std::nullopt;
}

std::string DBImpl::NewSSTablePath(int level) {
    return db_path_ + "/L" + std::to_string(level) + "_" + std::to_string(next_file_id_++) + ".sst";
}

void DBImpl::FlushMemTable() {
    if (memtable_.IsEmpty()) return;

    std::string sst_path = NewSSTablePath(0);
    SSTableBuilder builder(sst_path);
    auto entries = memtable_.GetEntries();
    for (const auto& [k, v] : entries) {
        builder.Add(k, v);
    }
    builder.Finish();

    levels_[0].push_back(sst_path);
    memtable_.Clear();
    wal_->Clear();

    compaction_cv_.notify_one();
}

void DBImpl::BackgroundCompactionLoop() {
    while (!stop_compaction_) {
        std::unique_lock lock(mutex_);
        compaction_cv_.wait(lock, [this]() {
            return stop_compaction_ || levels_[0].size() >= 4;
        });

        if (stop_compaction_) break;
        MaybeCompact();
    }
}

void DBImpl::MaybeCompact() {
    if (levels_[0].size() < 4) return;

    std::vector<std::string> l0_files = levels_[0];
    levels_[0].clear();

    // Release lock during I/O
    mutex_.unlock();
    
    std::string output_file = NewSSTablePath(1);
    bool success = CompactionJob::CompactFiles(l0_files, output_file);
    
    mutex_.lock();
    if (success) {
        levels_[1].push_back(output_file);
    }
}

} // namespace minidb
