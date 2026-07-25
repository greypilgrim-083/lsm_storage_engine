#include "memtable.h"
#include "metrics.h"

namespace minidb {

void MemTable::Put(const std::string& key, const std::string& value) {
    std::unique_lock lock(mutex_);
    
    auto it = table_.find(key);
    if (it != table_.end()) {
        // If updating an existing key, adjust size estimate
        approximate_size_ -= it->second.size();
        it->second = value;
        approximate_size_ += value.size();
    } else {
        table_[key] = value;
        approximate_size_ += key.size() + value.size();
    }
}

void MemTable::Delete(const std::string& key) {
    // Tombstone is an empty string
    Put(key, "");
}

std::optional<std::string> MemTable::Get(const std::string& key) const {
    std::shared_lock lock(mutex_);
    auto it = table_.find(key);
    if (it != table_.end()) {
        if (it->second.empty()) {
            return std::nullopt; // Tombstone
        }
        return it->second;
    }
    return std::nullopt;
}

size_t MemTable::ApproximateSize() const {
    std::shared_lock lock(mutex_);
    return approximate_size_;
}

bool MemTable::IsEmpty() const {
    std::shared_lock lock(mutex_);
    return table_.empty();
}

std::map<std::string, std::string> MemTable::GetEntries() const {
    std::shared_lock lock(mutex_);
    return table_; // Creates a copy
}

void MemTable::Clear() {
    std::unique_lock lock(mutex_);
    table_.clear();
    approximate_size_ = 0;
}

} // namespace minidb
