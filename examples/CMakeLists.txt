# Examples CMakeLists.txt
# One comprehensive example per feature

# === Core Framework Examples ===

# Comprehensive application framework demonstration
add_executable(application_example application_example.cpp)
target_link_libraries(application_example base)

# Comprehensive component logging demonstration
add_executable(component_logging_example component_logging_example.cpp)
target_link_libraries(component_logging_example base)

# Configuration system demonstration
add_executable(config_example config_example.cpp)
target_link_libraries(config_example base)

# === System Integration Examples ===

# Inter-thread messaging demonstration
add_executable(messaging_example messaging_example.cpp)
target_link_libraries(messaging_example base)

# Command-line interface demonstration
add_executable(cli_example cli_example.cpp)
target_link_libraries(cli_example base)

# Comprehensive table system demonstration
add_executable(table_example table_example.cpp)
target_link_libraries(table_example base)

# === Platform-Specific Examples ===

# Daemon application example (UNIX only)
if(UNIX)
    add_executable(daemon_example daemon_example.cpp)
    target_link_libraries(daemon_example base)
endif()

# === Set Common Properties ===

set(ALL_EXAMPLES
    application_example
    component_logging_example
    config_example
    messaging_example
    cli_example
    table_example
)

if(UNIX)
    list(APPEND ALL_EXAMPLES daemon_example)
endif()

# Set common properties for all examples
set_target_properties(
    ${ALL_EXAMPLES}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
