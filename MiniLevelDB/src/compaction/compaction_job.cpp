#include "compaction_job.h"
#include "sstable_reader.h"
#include "sstable_builder.h"
#include <map>
#include <iostream>
#include <filesystem>

namespace minidb {

bool CompactionJob::CompactFiles(const std::vector<std::string>& input_files, 
                                 const std::string& output_filepath) {
    if (input_files.empty()) return false;

    // A simple in-memory merge.
    // In a production system, this would use a K-way streaming merge iterator to save RAM.
    // We use a map to deduplicate and sort. Newer files should overwrite older ones.
    // We assume input_files are ordered from oldest to newest.
    std::map<std::string, std::string> merged_data;

    for (const auto& filepath : input_files) {
        SSTableReader reader(filepath);
        auto kvs = reader.ReadAll();
        for (const auto& kv : kvs) {
            merged_data[kv.first] = kv.second;
        }
    }

    SSTableBuilder builder(output_filepath);
    for (const auto& kv : merged_data) {
        // Drop tombstones at the highest level (or all for simplicity if we only have 2 levels)
        if (!kv.second.empty()) {
            builder.Add(kv.first, kv.second);
        }
    }
    builder.Finish();

    // Delete old files
    for (const auto& filepath : input_files) {
        std::error_code ec;
        std::filesystem::remove(filepath, ec);
    }

    return true;
}

} // namespace minidb
