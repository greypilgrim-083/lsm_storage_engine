#pragma once

#include <string>
#include <fstream>
#include <optional>
#include <vector>
#include "bloom_filter.h"

namespace minidb {

class SSTableReader {
public:
    SSTableReader(const std::string& filepath);
    ~SSTableReader();

    std::optional<std::string> Get(const std::string& key);

    // Read all KV pairs (for compaction)
    std::vector<std::pair<std::string, std::string>> ReadAll();

    const std::string& GetFilepath() const { return filepath_; }

private:
    std::string filepath_;
    std::ifstream file_;
    
    uint64_t index_offset_{0};
    uint64_t filter_offset_{0};
    
    std::optional<BloomFilter> filter_;
    
    struct IndexEntry {
        std::string key;
        uint64_t offset;
    };
    std::vector<IndexEntry> index_;

    void LoadFooterAndMeta();
};

} // namespace minidb
