#include "sstable_builder.h"
#include "metrics.h"
#include <iostream>

namespace minidb {

SSTableBuilder::SSTableBuilder(const std::string& filepath) : filepath_(filepath) {
    file_.open(filepath, std::ios::binary | std::ios::trunc);
    if (!file_.is_open()) {
        std::cerr << "Failed to open SSTable for writing: " << filepath << "\n";
    }
}

SSTableBuilder::~SSTableBuilder() {
    if (file_.is_open()) {
        Finish();
    }
}

void SSTableBuilder::Add(const std::string& key, const std::string& value) {
    if (!file_.is_open()) return;

    if (keys_added_ % 64 == 0) {
        index_.push_back({key, offset_});
    }

    uint32_t key_len = static_cast<uint32_t>(key.size());
    uint32_t val_len = static_cast<uint32_t>(value.size());

    file_.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
    file_.write(key.data(), key_len);
    file_.write(reinterpret_cast<const char*>(&val_len), sizeof(val_len));
    file_.write(value.data(), val_len);

    uint64_t bytes_written = sizeof(key_len) + key_len + sizeof(val_len) + val_len;
    offset_ += bytes_written;
    Metrics::GetInstance().disk_bytes_written += bytes_written;
    
    keys_.push_back(key);
    keys_added_++;
}

void SSTableBuilder::Finish() {
    if (!file_.is_open()) return;

    // Build Bloom Filter
    BloomFilter filter(keys_.size());
    for (const auto& k : keys_) {
        filter.Add(k);
    }
    std::string filter_data = filter.Serialize();
    
    uint64_t filter_offset = offset_;
    uint32_t filter_len = static_cast<uint32_t>(filter_data.size());
    file_.write(reinterpret_cast<const char*>(&filter_len), sizeof(filter_len));
    file_.write(filter_data.data(), filter_len);
    
    uint64_t bytes_written_filter = sizeof(filter_len) + filter_len;
    offset_ += bytes_written_filter;
    Metrics::GetInstance().disk_bytes_written += bytes_written_filter;

    // Build Index Block
    uint64_t index_offset = offset_;
    uint32_t num_index_entries = static_cast<uint32_t>(index_.size());
    file_.write(reinterpret_cast<const char*>(&num_index_entries), sizeof(num_index_entries));
    Metrics::GetInstance().disk_bytes_written += sizeof(num_index_entries);
    offset_ += sizeof(num_index_entries);

    for (const auto& entry : index_) {
        uint32_t k_len = static_cast<uint32_t>(entry.key.size());
        file_.write(reinterpret_cast<const char*>(&k_len), sizeof(k_len));
        file_.write(entry.key.data(), k_len);
        file_.write(reinterpret_cast<const char*>(&entry.offset), sizeof(entry.offset));
        
        uint64_t bytes_written = sizeof(k_len) + k_len + sizeof(entry.offset);
        offset_ += bytes_written;
        Metrics::GetInstance().disk_bytes_written += bytes_written;
    }

    // Write Footer (Fixed 16 bytes: index_offset + filter_offset)
    file_.write(reinterpret_cast<const char*>(&index_offset), sizeof(index_offset));
    file_.write(reinterpret_cast<const char*>(&filter_offset), sizeof(filter_offset));
    Metrics::GetInstance().disk_bytes_written += 16;
    
    file_.close();
}

} // namespace minidb
