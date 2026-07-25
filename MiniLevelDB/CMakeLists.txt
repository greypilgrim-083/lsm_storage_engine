cmake_minimum_required(VERSION 3.14)
project(MiniLevelDB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Common compiler flags for performance
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O3 -pthread)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Fetch GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Fetch GBenchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.7.1
)
FetchContent_MakeAvailable(googlebenchmark)

# Subdirectories
add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)
