add_executable(minidb_bench bench_main.cpp)
target_link_libraries(minidb_bench PRIVATE minidb_core benchmark::benchmark benchmark::benchmark_main)
