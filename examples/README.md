# Base Framework Examples

This directory contains comprehensive examples demonstrating the core features of the Base application framework. Each example is designed to be standalone and focuses on a specific feature or use case.

## Examples Overview

### Core Framework

- **`application_example.cpp`** - Comprehensive application framework demonstration including configuration, lifecycle management, threading, and CLI integration
- **`component_logging_example.cpp`** - Complete component-level logging system with runtime filtering, SIGHUP reload, and automatic component tagging
- **`config_example.cpp`** - Configuration system demonstration with TOML file handling and validation

### System Integration

- **`messaging_example.cpp`** - High-performance inter-thread messaging system with event-driven processing
- **`cli_example.cpp`** - Interactive command-line interface with custom commands and runtime inspection
- **`table_example.cpp`** - Comprehensive table system with storage, querying, indexing, and dump/print functionality

### Platform-Specific

- **`daemon_example.cpp`** - Full-featured daemonization example with privilege dropping, PID files, and signal handling (UNIX only)

## Configuration Files

- **`component_demo.toml`** - Configuration for component logging demonstrations
- **`component_demo_selective.toml`** - Selective component filtering configuration
- **`example_app.toml`** - General application configuration template

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
# Application framework with threading and CLI
./build/Release/examples/application_example

# Component logging with runtime filtering
./build/Release/examples/component_logging_example

# Configuration system
./build/Release/examples/config_example

# Inter-thread messaging
./build/Release/examples/messaging_example

# Command-line interface
./build/Release/examples/cli_example

# Table system with dump/print
./build/Release/examples/table_example

# Daemon application (UNIX only)
./build/Release/examples/daemon_example
```

## Example Features

### Application Example

- Multi-threaded application with configurable worker pools
- Component lifecycle management
- Signal handling (SIGINT, SIGTERM, SIGHUP)
- CLI integration for runtime inspection
- Configuration file support with command line overrides

### Component Logging Example

- Component-level log filtering and tagging
- Runtime configuration reload via SIGHUP signal
- Automatic component name prepending
- Multiple log level demonstrations
- Configuration file integration

### Configuration Example

- TOML configuration file parsing
- Nested configuration structures
- Configuration validation and error handling
- Runtime configuration access

### Messaging Example

- Type-safe inter-thread messaging
- Message priority handling
- Event-driven message processing
- Thread-safe message queues

### CLI Example

- Interactive command-line interface
- Custom command registration
- Runtime application inspection
- TCP and stdin interface support

### Table Example

- Table creation with typed schemas
- Data insertion and indexing
- Query system with filtering
- Multiple dump formats (ASCII, CSV, JSON)
- Pagination for large datasets
- Statistics and performance metrics

### Daemon Example (UNIX)

- Complete daemonization process
- Privilege dropping and security
- PID file management
- Signal handling for graceful shutdown
- File descriptor management

## Getting Started

1. **Start with `application_example.cpp`** to understand the basic framework structure
2. **Explore `component_logging_example.cpp`** to see the logging system in action
3. **Try `config_example.cpp`** to understand configuration management
4. **Run `daemon_example.cpp`** on UNIX systems to see daemonization in action

Each example includes detailed comments and demonstrates best practices for production applications.

Some examples use the provided `example_app.toml` configuration file. This demonstrates the configuration system integration.
