#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace minidb {

class BloomFilter {
public:
    BloomFilter(size_t expected_items, double false_positive_rate = 0.01);
    
    // For reading an existing filter from disk
    BloomFilter(const std::string& serialized);

    void Add(const std::string& key);
    bool PossiblyContains(const std::string& key) const;
    
    std::string Serialize() const;

private:
    size_t num_hashes_;
    std::vector<bool> bits_;
    
    uint32_t Hash(const std::string& key, size_t i) const;
};

} // namespace minidb
