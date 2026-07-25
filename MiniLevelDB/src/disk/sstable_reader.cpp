#include "sstable_reader.h"
#include "metrics.h"
#include <iostream>

namespace minidb {

SSTableReader::SSTableReader(const std::string& filepath) : filepath_(filepath) {
    file_.open(filepath, std::ios::binary);
    if (file_.is_open()) {
        LoadFooterAndMeta();
    }
}

SSTableReader::~SSTableReader() {
    if (file_.is_open()) file_.close();
}

void SSTableReader::LoadFooterAndMeta() {
    // Read Footer (last 16 bytes)
    file_.seekg(0, std::ios::end);
    std::streampos file_size = file_.tellg();
    if (file_size < 16) return; // Invalid file

    file_.seekg(static_cast<uint64_t>(file_size) - 16, std::ios::beg);
    file_.read(reinterpret_cast<char*>(&index_offset_), sizeof(index_offset_));
    file_.read(reinterpret_cast<char*>(&filter_offset_), sizeof(filter_offset_));

    // Load Filter
    file_.seekg(filter_offset_, std::ios::beg);
    uint32_t filter_len;
    file_.read(reinterpret_cast<char*>(&filter_len), sizeof(filter_len));
    std::string filter_data(filter_len, '\0');
    file_.read(&filter_data[0], filter_len);
    filter_.emplace(filter_data);

    // Load Index
    file_.seekg(index_offset_, std::ios::beg);
    uint32_t num_index_entries;
    file_.read(reinterpret_cast<char*>(&num_index_entries), sizeof(num_index_entries));
    index_.resize(num_index_entries);
    for (uint32_t i = 0; i < num_index_entries; ++i) {
        uint32_t k_len;
        file_.read(reinterpret_cast<char*>(&k_len), sizeof(k_len));
        index_[i].key.resize(k_len);
        file_.read(&index_[i].key[0], k_len);
        file_.read(reinterpret_cast<char*>(&index_[i].offset), sizeof(index_[i].offset));
    }
}

std::optional<std::string> SSTableReader::Get(const std::string& key) {
    Metrics::GetInstance().sstables_queried++;

    if (filter_ && !filter_->PossiblyContains(key)) {
        Metrics::GetInstance().bloom_filter_drops++;
        return std::nullopt;
    }

    if (!file_.is_open() || index_.empty()) return std::nullopt;

    // Find the right block via binary search
    // We want the last entry where entry.key <= key
    int left = 0, right = index_.size() - 1;
    int best = -1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (index_[mid].key <= key) {
            best = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    if (best == -1) return std::nullopt;

    uint64_t read_offset = index_[best].offset;
    uint64_t end_offset = (static_cast<size_t>(best + 1) < index_.size()) ? index_[best + 1].offset : filter_offset_;

    file_.seekg(read_offset, std::ios::beg);

    while (file_.tellg() < static_cast<std::streampos>(end_offset)) {
        uint32_t k_len;
        file_.read(reinterpret_cast<char*>(&k_len), sizeof(k_len));
        if (file_.eof()) break;
        std::string k(k_len, '\0');
        file_.read(&k[0], k_len);

        uint32_t v_len;
        file_.read(reinterpret_cast<char*>(&v_len), sizeof(v_len));
        std::string v(v_len, '\0');
        file_.read(&v[0], v_len);

        if (k == key) {
            if (v.empty()) return std::nullopt; // Tombstone
            return v;
        }
        if (k > key) break; // Because data is sorted
    }
    return std::nullopt;
}

std::vector<std::pair<std::string, std::string>> SSTableReader::ReadAll() {
    std::vector<std::pair<std::string, std::string>> kv;
    file_.seekg(0, std::ios::beg);
    while (file_.tellg() < static_cast<std::streampos>(filter_offset_)) {
        uint32_t k_len;
        file_.read(reinterpret_cast<char*>(&k_len), sizeof(k_len));
        if (file_.eof() || file_.fail()) break;
        std::string k(k_len, '\0');
        file_.read(&k[0], k_len);

        uint32_t v_len;
        file_.read(reinterpret_cast<char*>(&v_len), sizeof(v_len));
        std::string v(v_len, '\0');
        file_.read(&v[0], v_len);
        kv.push_back({k, v});
    }
    return kv;
}

} // namespace minidb
