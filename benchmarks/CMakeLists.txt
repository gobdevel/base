cmake_minimum_required(VERSION 3.15)
project(BaseBenchmarks)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# We'll use the main project's build system instead of duplicating it
# This is a simple CMakeLists.txt that assumes we're building from the main project

# Find required packages
find_package(Threads REQUIRED)

# For now, we'll create a simpler benchmark that can be built manually
# The user can compile this manually with the correct flags

message(STATUS "Benchmark runner source ready in ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "To build manually:")
message(STATUS "cd ${CMAKE_CURRENT_SOURCE_DIR}/../build/Release")
message(STATUS "clang++ -std=c++20 -O3 -I../include -L. -lbase ../benchmarks/benchmark_runner.cpp -o benchmark_runner")

# Create a simple target that shows instructions
add_custom_target(benchmark_instructions
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "To build the benchmark runner:"
    COMMAND ${CMAKE_COMMAND} -E echo "1. Make sure the main project is built:"
    COMMAND ${CMAKE_COMMAND} -E echo "   cmake --build ../build/Release"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "2. Then build the benchmark manually:"
    COMMAND ${CMAKE_COMMAND} -E echo "   cd ../build/Release"
    COMMAND ${CMAKE_COMMAND} -E echo "   clang++ -std=c++20 -O3 -DNDEBUG \\"
    COMMAND ${CMAKE_COMMAND} -E echo "           -I../../include \\"
    COMMAND ${CMAKE_COMMAND} -E echo "           -L. -lbase \\"
    COMMAND ${CMAKE_COMMAND} -E echo "           ../../benchmarks/benchmark_runner.cpp \\"
    COMMAND ${CMAKE_COMMAND} -E echo "           -o benchmark_runner"
    COMMAND ${CMAKE_COMMAND} -E echo ""
)

# Create output directory for results
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/results)

# Add custom targets for different benchmark types
add_custom_target(bench_all
    COMMAND ${CMAKE_BINARY_DIR}/benchmark_runner
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/results
    COMMENT "Running all benchmarks"
)

add_custom_target(bench_logger
    COMMAND ${CMAKE_BINARY_DIR}/benchmark_runner --logger
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/results
    COMMENT "Running logger benchmarks"
)

add_custom_target(bench_messaging
    COMMAND ${CMAKE_BINARY_DIR}/benchmark_runner --messaging
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/results
    COMMENT "Running messaging benchmarks"
)

add_custom_target(bench_config
    COMMAND ${CMAKE_BINARY_DIR}/benchmark_runner --config
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/results
    COMMENT "Running configuration benchmarks"
)

add_custom_target(bench_threads
    COMMAND ${CMAKE_BINARY_DIR}/benchmark_runner --threads
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/results
    COMMENT "Running thread management benchmarks"
)

add_custom_target(bench_memory
    COMMAND ${CMAKE_BINARY_DIR}/benchmark_runner --memory
    DEPENDS benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/results
    COMMENT "Running memory usage benchmarks"
)

# Documentation
install(FILES README.md DESTINATION share/doc/base-benchmarks)
