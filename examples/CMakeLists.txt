# Examples CMakeLists.txt
# Essential examples for the Base messaging framework

# === Core Examples ===

# Basic application framework usage
add_executable(app_example app_example.cpp)
target_link_libraries(app_example base)

# Daemon application example (UNIX only)
if(UNIX)
    add_executable(daemon_example daemon_example.cpp)
    target_link_libraries(daemon_example base)

    add_executable(simple_daemon_test simple_daemon_test.cpp)
    target_link_libraries(simple_daemon_test base)
endif()

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

# Command-line arguments demonstration
add_executable(cli_args_example cli_args_example.cpp)
target_link_libraries(cli_args_example base)

# Table system demonstration
add_executable(table_example table_example.cpp)
target_link_libraries(table_example base)

# Table dump/print API demonstration
add_executable(table_dump_example table_dump_example.cpp)
target_link_libraries(table_dump_example base)

# App example 2
add_executable(app_example_2 app_example_2.cpp)
target_link_libraries(app_example_2 base)

# Set output directory for all examples
set_target_properties(
    app_example
    config_example
    logger_config_demo
    messaging_example
    cli_example
    table_example
    table_dump_example
    app_example_2
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
    table_example
    table_dump_example
    app_example_2
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
