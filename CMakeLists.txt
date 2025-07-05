cmake_minimum_required(VERSION 3.15)
project(base CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add the correct path for Conan-generated files
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/build/Release/generators")

# Include Conan dependencies first - but use explicit find_package as recommended
# include(${CMAKE_BINARY_DIR}/generators/conandeps_legacy.cmake)

find_package(spdlog REQUIRED)
find_package(tomlplusplus REQUIRED)
find_package(asio REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

add_library(base
    src/logger.cpp
    src/config.cpp
    src/application.cpp
    src/messaging.cpp
    src/cli.cpp
    src/tables.cpp
)
target_include_directories(base PUBLIC include)

# Add nlohmann_json include directory (header-only library)
find_path(NLOHMANN_JSON_INCLUDE_DIR
    NAMES nlohmann/json.hpp
    PATHS /Users/gobindp/.conan2/p/*/p/include
    NO_DEFAULT_PATH
)
if(NLOHMANN_JSON_INCLUDE_DIR)
    target_include_directories(base PRIVATE ${NLOHMANN_JSON_INCLUDE_DIR})
else()
    message(FATAL_ERROR "nlohmann/json.hpp not found")
endif()

# Add examples subdirectory only if it exists (not needed for Conan packages)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# Add benchmark executable only if benchmarks directory exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
    # Add Google Benchmark based table benchmark
    if(benchmark_FOUND)
        add_executable(table_benchmark benchmarks/table_benchmark.cpp)
        target_link_libraries(table_benchmark base benchmark::benchmark)
        set_target_properties(table_benchmark PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks")

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

        # Add benchmark adapter example
        add_executable(benchmark_adapter_example benchmarks/benchmark_adapter_example.cpp)
        target_link_libraries(benchmark_adapter_example base benchmark::benchmark)
        set_target_properties(benchmark_adapter_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks")

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

        message(STATUS "Google Benchmark found - building table_benchmark")
    else()
        message(STATUS "Google Benchmark not found - skipping table_benchmark")
    endif()
endif()

if (NOT BUILD_TESTING STREQUAL OFF)
    add_subdirectory(tests)
endif()

target_link_libraries(base
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    asio::asio
)

set_target_properties(base PROPERTIES PUBLIC_HEADER "include/logger.h;include/config.h;include/singleton.h;include/application.h;include/messaging.h;include/cli.h;include/tables.h")
install(TARGETS base)
