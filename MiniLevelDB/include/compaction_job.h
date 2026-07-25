#pragma once

#include <vector>
#include <string>

namespace minidb {

class CompactionJob {
public:
    // Takes a list of input SSTable filepaths and merges them into output_filepath.
    // Returns true on success.
    static bool CompactFiles(const std::vector<std::string>& input_files, 
                             const std::string& output_filepath);
};

} // namespace minidb
