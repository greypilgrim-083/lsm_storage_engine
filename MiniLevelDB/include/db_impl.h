#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "memtable.h"
#include "wal.h"

namespace minidb {

class DBImpl {
public:
    DBImpl(const std::string& db_path);
    ~DBImpl();

    void Put(const std::string& key, const std::string& value);
    void Delete(const std::string& key);
    std::optional<std::string> Get(const std::string& key);

private:
    std::string db_path_;
    
    std::mutex mutex_;
    MemTable memtable_;
    std::unique_ptr<WAL> wal_;
    
    // Levels of SSTables (just file paths)
    // levels_[0] = L0 files, levels_[1] = L1 files, etc.
    std::vector<std::vector<std::string>> levels_;
    uint64_t next_file_id_{1};

    // Background compaction thread
    std::thread compaction_thread_;
    std::atomic<bool> stop_compaction_{false};
    std::condition_variable compaction_cv_;

    void FlushMemTable();
    void BackgroundCompactionLoop();
    void MaybeCompact();
    std::string NewSSTablePath(int level);
};

} // namespace minidb
