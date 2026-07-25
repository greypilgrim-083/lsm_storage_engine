add_library(minidb_core
    core/db_impl.cpp
    mem/memtable.cpp
    mem/wal.cpp
    disk/sstable_builder.cpp
    disk/sstable_reader.cpp
    disk/bloom_filter.cpp
    compaction/compaction_job.cpp
    telemetry/metrics.cpp
)
target_include_directories(minidb_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
