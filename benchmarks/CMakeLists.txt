cmake_minimum_required(VERSION 3.15)
project(BaseBenchmarks)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(benchmark REQUIRED)
find_package(Threads REQUIRED)

# Table benchmarks using Google Benchmark
if(benchmark_FOUND)
    add_executable(table_benchmark table_benchmark.cpp)
    target_link_libraries(table_benchmark
        base
        benchmark::benchmark
        Threads::Threads
    )
    target_include_directories(table_benchmark PRIVATE ../include)

    # Add custom targets for running benchmarks
    add_custom_target(run_table_benchmark
        COMMAND table_benchmark
        DEPENDS table_benchmark
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running table benchmarks with Google Benchmark"
    )

    add_custom_target(run_table_benchmark_json
        COMMAND table_benchmark --benchmark_format=json --benchmark_out=table_benchmark_results.json
        DEPENDS table_benchmark
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running table benchmarks with JSON output"
    )

    message(STATUS "Google Benchmark found - building table benchmarks")
else()
    message(WARNING "Google Benchmark not found - benchmark targets will not be available")
endif()

# Add benchmark adapter example
add_executable(benchmark_adapter_example benchmark_adapter_example.cpp)
target_link_libraries(benchmark_adapter_example
    base
    benchmark::benchmark
    Threads::Threads
)
target_include_directories(benchmark_adapter_example PRIVATE ../include)

# Create output directory for results
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark_results)

# Documentation
install(FILES README.md DESTINATION share/doc/base-benchmarks)

# Add profile-specific targets
add_custom_target(run_profile_quick
    COMMAND benchmark_adapter_example --benchmark_filter=".*_Quick"
    DEPENDS benchmark_adapter_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Quick profile benchmarks"
)

add_custom_target(run_profile_development
    COMMAND benchmark_adapter_example --benchmark_filter=".*_Development"
    DEPENDS benchmark_adapter_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Development profile benchmarks"
)

add_custom_target(run_profile_performance
    COMMAND benchmark_adapter_example --benchmark_filter=".*_Performance"
    DEPENDS benchmark_adapter_example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Performance profile benchmarks"
)
