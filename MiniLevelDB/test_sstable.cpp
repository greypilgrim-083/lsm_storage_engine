#include "include/sstable_builder.h"
#include "include/sstable_reader.h"
#include <iostream>

using namespace minidb;

int main() {
    std::string large_val(5 * 1024 * 1024, 'A');
    {
        SSTableBuilder builder("test.sst");
        builder.Add("large_key", large_val);
        builder.Finish();
    }
    {
        SSTableReader reader("test.sst");
        auto val = reader.Get("large_key");
        if (val.has_value()) {
            std::cout << "FOUND! length: " << val.value().size() << "\n";
        } else {
            std::cout << "NOT FOUND!\n";
        }
    }
    return 0;
}
