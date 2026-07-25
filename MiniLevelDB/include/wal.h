#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include "memtable.h"

namespace minidb {

class WAL {
public:
    explicit WAL(const std::string& filepath);
    ~WAL();

    // Appends a put/delete operation to the log
    void Append(const std::string& key, const std::string& value);

    // Reads the WAL file and repopulates the memtable
    void Recover(MemTable& memtable);

    // Clears the WAL file (typically after a memtable flush)
    void Clear();

private:
    std::string filepath_;
    std::ofstream file_;
    std::mutex mutex_;
};

} // namespace minidb
