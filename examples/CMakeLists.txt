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

    # Add daemon examples to target properties lists only on UNIX
    set_target_properties(
        daemon_example
        simple_daemon_test
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
endif()

# Logger convenience methods test
add_executable(convenience_test convenience_test.cpp)
target_link_libraries(convenience_test PRIVATE base)
set_target_properties(
    convenience_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

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

# Component-level logging demonstration
add_executable(component_logging_example component_logging_example.cpp)
target_link_libraries(component_logging_example base)

# Simple component logging test
add_executable(simple_component_test simple_component_test.cpp)
target_link_libraries(simple_component_test base)

# Debug component logging test
add_executable(debug_component_test debug_component_test.cpp)
target_link_libraries(debug_component_test base)

# Table system demonstration
add_executable(table_example table_example.cpp)
target_link_libraries(table_example base)

# Table dump/print API demonstration
add_executable(table_dump_example table_dump_example.cpp)
target_link_libraries(table_dump_example base)

# App example 2
add_executable(app_example_2 app_example_2.cpp)
target_link_libraries(app_example_2 base)

# Component logger test (automatic component prepending)
add_executable(component_logger_test component_logger_test.cpp)
target_link_libraries(component_logger_test PRIVATE base)
set_target_properties(
    component_logger_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Runtime filter test
add_executable(runtime_filter_test runtime_filter_test.cpp)
target_link_libraries(runtime_filter_test PRIVATE base)
set_target_properties(
    runtime_filter_test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Set output directory for all examples
set_target_properties(
    app_example
    config_example
    logger_config_demo
    messaging_example
    cli_example
    cli_args_example
    component_logging_example
    simple_component_test
    table_example
    table_dump_example
    app_example_2
    component_logger_test
    runtime_filter_test
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
    cli_args_example
    component_logging_example
    simple_component_test
    table_example
    table_dump_example
    app_example_2
    component_logger_test
    runtime_filter_test
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
