#pragma once

#include <string>
#include <map>
#include <shared_mutex>
#include <optional>

namespace minidb {

class MemTable {
public:
    MemTable() = default;

    void Put(const std::string& key, const std::string& value);
    void Delete(const std::string& key);
    std::optional<std::string> Get(const std::string& key) const;

    // Approximate size in bytes of data stored
    size_t ApproximateSize() const;

    // Returns true if the MemTable is empty
    bool IsEmpty() const;

    // Iterate over entries (useful for flushing to SSTable)
    std::map<std::string, std::string> GetEntries() const;

    void Clear();

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::string> table_;
    size_t approximate_size_{0};
};

} // namespace minidb
