# Examples CMakeLists.txt
# This file builds all the example applications for the Crux framework

# Config example - demonstrates configuration system
add_executable(config_example config_example.cpp)
target_link_libraries(config_example crux)

# Color test - demonstrates logger color output
add_executable(color_test color_test.cpp)
target_link_libraries(color_test crux)

# Logger configuration demo - shows logger setup from config files
add_executable(logger_config_demo logger_config_demo.cpp)
target_link_libraries(logger_config_demo crux)

# Application framework example - basic application setup
add_executable(app_example app_example.cpp)
target_link_libraries(app_example crux)

# Messaging system example - demonstrates inter-thread messaging
add_executable(messaging_example messaging_example.cpp)
target_link_libraries(messaging_example crux)

# CLI example - demonstrates runtime inspection and debugging CLI
add_executable(cli_example cli_example.cpp)
target_link_libraries(cli_example crux)

# CLI comprehensive test - consolidated test suite for all CLI functionality
add_executable(cli_comprehensive_test cli_comprehensive_test.cpp)
target_link_libraries(cli_comprehensive_test crux)

# Minimal test - basic application framework test without CLI
add_executable(minimal_test minimal_test.cpp)
target_link_libraries(minimal_test crux)

# Simple performance test - demonstrates performance measurement
add_executable(simple_performance_test simple_performance_test.cpp)
target_link_libraries(simple_performance_test crux)

# Set output directory for examples
set_target_properties(config_example color_test logger_config_demo app_example messaging_example cli_example cli_comprehensive_test minimal_test simple_performance_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
