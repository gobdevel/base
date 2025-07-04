# Base Framework Examples

This directory contains essential examples demonstrating core features of the Base application framework.

## Examples

### Core Framework

- **`app_example.cpp`** - Basic application framework usage with configuration and lifecycle management
- **`config_example.cpp`** - Configuration system demonstration with TOML file handling
- **`logger_config_demo.cpp`** - Logger configuration and setup examples

### Messaging System

- **`messaging_example.cpp`** - High-performance inter-thread messaging system demonstration

### Command Line Interface

- **`cli_example.cpp`** - Interactive command-line interface with custom commands

## Building Examples

Examples are automatically built when you build the main project:

```bash
# Configure and build the project
conan install . --build missing -pr=default --settings=build_type=Release
cmake --preset conan-release
cmake --build build/Release --config Release

# Examples will be available in:
# build/Release/examples/
```

## Running Examples

Each example is self-contained and can be run independently:

```bash
# Run the messaging system example
./build/Release/examples/messaging_example

# Run the application framework example
./build/Release/examples/app_example

# Run the CLI example
./build/Release/examples/cli_example
```

## Configuration

Some examples use the provided `example_app.toml` configuration file. This demonstrates the configuration system integration.
