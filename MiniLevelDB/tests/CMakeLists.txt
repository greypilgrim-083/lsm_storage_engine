add_executable(minidb_test test_main.cpp)
target_link_libraries(minidb_test PRIVATE minidb_core gtest gtest_main)
add_test(NAME CoreTest COMMAND minidb_test)
