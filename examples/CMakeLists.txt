# Examples CMakeLists.txt
# This file builds all the example applications for the Base framework

# Config example - demonstrates configuration system
add_executable(config_example config_example.cpp)
target_link_libraries(config_example base)

# Color test - demonstrates logger color output
add_executable(color_test color_test.cpp)
target_link_libraries(color_test base)

# Logger configuration demo - shows logger setup from config files
add_executable(logger_config_demo logger_config_demo.cpp)
target_link_libraries(logger_config_demo base)

# Application framework example - basic application setup
add_executable(app_example app_example.cpp)
target_link_libraries(app_example base)

# Messaging system example - demonstrates inter-thread messaging
add_executable(messaging_example messaging_example.cpp)
target_link_libraries(messaging_example base)

# CLI example - demonstrates runtime inspection and debugging CLI
add_executable(cli_example cli_example.cpp)
target_link_libraries(cli_example base)

# CLI comprehensive test - consolidated test suite for all CLI functionality
add_executable(cli_comprehensive_test cli_comprehensive_test.cpp)
target_link_libraries(cli_comprehensive_test base)

# Minimal test - basic application framework test without CLI
add_executable(minimal_test minimal_test.cpp)
target_link_libraries(minimal_test base)

# Simple performance test - demonstrates performance measurement
add_executable(simple_performance_test simple_performance_test.cpp)
target_link_libraries(simple_performance_test base)

# Performance benchmark - comprehensive latency and throughput benchmarking
add_executable(benchmark_runner benchmark_runner.cpp)
target_link_libraries(benchmark_runner base)

# Simple task example - demonstrates user-friendly task posting API
add_executable(simple_task_example simple_task_example.cpp)
target_link_libraries(simple_task_example base)

# Ultra simple example - demonstrates the simplest possible API usage
add_executable(ultra_simple_example ultra_simple_example.cpp)
target_link_libraries(ultra_simple_example base)

# Priority demo - demonstrates the corrected task priority system
add_executable(priority_demo priority_demo.cpp)
target_link_libraries(priority_demo base)

# Set output directory for examples
set_target_properties(config_example color_test logger_config_demo app_example messaging_example cli_example cli_comprehensive_test minimal_test simple_performance_test benchmark_runner simple_task_example ultra_simple_example priority_demo
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
