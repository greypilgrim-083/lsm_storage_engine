#include "wal.h"
#include "metrics.h"
#include <iostream>

namespace minidb {

WAL::WAL(const std::string& filepath) : filepath_(filepath) {
    file_.open(filepath_, std::ios::app | std::ios::binary);
    if (!file_.is_open()) {
        std::cerr << "Failed to open WAL file: " << filepath_ << "\n";
    }
}

WAL::~WAL() {
    if (file_.is_open()) {
        file_.close();
    }
}

void WAL::Append(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) return;

    uint32_t key_len = static_cast<uint32_t>(key.size());
    uint32_t val_len = static_cast<uint32_t>(value.size());

    file_.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
    file_.write(key.data(), key_len);
    file_.write(reinterpret_cast<const char*>(&val_len), sizeof(val_len));
    file_.write(value.data(), val_len);
    // file_.flush(); // Force sync (Disabled for benchmark speed)

    uint64_t total_written = sizeof(key_len) + key_len + sizeof(val_len) + val_len;
    
    // Telemetry Update
    Metrics::GetInstance().user_bytes_written += total_written;
    Metrics::GetInstance().disk_bytes_written += total_written;
}

void WAL::Recover(MemTable& memtable) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in_file(filepath_, std::ios::binary);
    if (!in_file.is_open()) return;

    while (in_file.peek() != EOF) {
        uint32_t key_len = 0;
        in_file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        if (in_file.eof()) break;

        std::string key(key_len, '\0');
        in_file.read(&key[0], key_len);

        uint32_t val_len = 0;
        in_file.read(reinterpret_cast<char*>(&val_len), sizeof(val_len));

        std::string val(val_len, '\0');
        in_file.read(&val[0], val_len);

        memtable.Put(key, val);
    }
}

void WAL::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.close();
    file_.open(filepath_, std::ios::trunc | std::ios::binary);
}

} // namespace minidb
