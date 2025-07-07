cmake_minimum_required(VERSION 3.15)
project(BaseBenchmarks)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(benchmark REQUIRED)

# Table benchmarks using Google Benchmark

add_executable(table_benchmark table_benchmark.cpp)
target_link_libraries(table_benchmark
    base
    benchmark::benchmark
)

# Add benchmark adapter example
add_executable(benchmark_adapter_example benchmark_adapter_example.cpp)
target_link_libraries(benchmark_adapter_example
    base
    benchmark::benchmark
)

# Application benchmarks using Google Benchmark
add_executable(application_benchmark application_benchmark.cpp)
target_link_libraries(application_benchmark
    base
    benchmark::benchmark
)

# Memory tracking example
add_executable(memory_example memory_example.cpp)
target_link_libraries(memory_example
    base
    benchmark::benchmark
)

# Create output directory for results
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark_results)

# Documentation
install(FILES README.md DESTINATION share/doc/base-benchmarks)

message(STATUS "Application benchmarks configured successfully")
