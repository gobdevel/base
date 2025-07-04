# Examples CMakeLists.txt
# Essential examples for the Base messaging framework

# === Core Examples ===

# Basic application framework usage
add_executable(app_example app_example.cpp)
target_link_libraries(app_example base)

# Configuration system demonstration
add_executable(config_example config_example.cpp)
target_link_libraries(config_example base)

# Logger configuration and setup
add_executable(logger_config_demo logger_config_demo.cpp)
target_link_libraries(logger_config_demo base)

# === Messaging System Examples ===

# Main messaging system demonstration - shows inter-thread communication
add_executable(messaging_example messaging_example.cpp)
target_link_libraries(messaging_example base)

# === CLI Examples ===

# Command-line interface demonstration
add_executable(cli_example cli_example.cpp)
target_link_libraries(cli_example base)

# Set output directory for all examples
set_target_properties(
    app_example
    config_example
    logger_config_demo
    messaging_example
    cli_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)

# Ensure examples compile with the same standards as the main project
set_target_properties(
    app_example
    config_example
    logger_config_demo
    messaging_example
    cli_example
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
