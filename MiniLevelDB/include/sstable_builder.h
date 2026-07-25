#pragma once

#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "bloom_filter.h"

namespace minidb {

class SSTableBuilder {
public:
    SSTableBuilder(const std::string& filepath);
    ~SSTableBuilder();

    // Appends a single K/V pair. Must be called in sorted order!
    void Add(const std::string& key, const std::string& value);
    
    // Writes the filter, index, and footer, then closes the file
    void Finish();

private:
    std::string filepath_;
    std::ofstream file_;
    uint64_t offset_{0};
    
    // We store index entries every K items or at the start of a block.
    // For simplicity, let's store every N keys in the index block.
    struct IndexEntry {
        std::string key;
        uint64_t offset;
    };
    std::vector<IndexEntry> index_;
    std::vector<std::string> keys_; // For bloom filter building
    size_t keys_added_{0};
};

} // namespace minidb
