#include "bloom_filter.h"
#include <cmath>
#include <functional>

namespace minidb {

BloomFilter::BloomFilter(size_t expected_items, double false_positive_rate) {
    if (expected_items == 0) expected_items = 1;
    size_t num_bits = static_cast<size_t>(-(expected_items * std::log(false_positive_rate)) / (std::log(2) * std::log(2)));
    num_hashes_ = static_cast<size_t>((num_bits / expected_items) * std::log(2));
    if (num_hashes_ == 0) num_hashes_ = 1;
    bits_.resize(num_bits, false);
}

BloomFilter::BloomFilter(const std::string& serialized) {
    if (serialized.size() < sizeof(size_t)) return;
    num_hashes_ = *reinterpret_cast<const size_t*>(serialized.data());
    
    bits_.resize((serialized.size() - sizeof(size_t)) * 8, false);
    const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(serialized.data() + sizeof(size_t));
    for (size_t i = 0; i < bits_.size(); ++i) {
        if (byte_data[i / 8] & (1 << (i % 8))) {
            bits_[i] = true;
        }
    }
}

uint32_t BloomFilter::Hash(const std::string& key, size_t i) const {
    std::hash<std::string> hasher;
    // Simple double hashing
    uint32_t h1 = hasher(key);
    uint32_t h2 = hasher(key + std::to_string(i));
    return (h1 + i * h2) % bits_.size();
}

void BloomFilter::Add(const std::string& key) {
    for (size_t i = 0; i < num_hashes_; ++i) {
        bits_[Hash(key, i)] = true;
    }
}

bool BloomFilter::PossiblyContains(const std::string& key) const {
    for (size_t i = 0; i < num_hashes_; ++i) {
        if (!bits_[Hash(key, i)]) {
            return false;
        }
    }
    return true;
}

std::string BloomFilter::Serialize() const {
    std::string out;
    out.append(reinterpret_cast<const char*>(&num_hashes_), sizeof(num_hashes_));
    
    size_t num_bytes = (bits_.size() + 7) / 8;
    std::vector<uint8_t> bytes(num_bytes, 0);
    for (size_t i = 0; i < bits_.size(); ++i) {
        if (bits_[i]) {
            bytes[i / 8] |= (1 << (i % 8));
        }
    }
    out.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return out;
}

} // namespace minidb
