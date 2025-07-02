# Crux Framework Examples

This directory contains example applications demonstrating various features of the Crux framework.

## Building Examples

Examples are built automatically when you build the main project:

```bash
cd build/Release
make
```

The example executables will be created in `build/Release/examples/`.

## Available Examples

### 1. config_example

**File**: `config_example.cpp`
**Configuration**: `example_app.toml`

Demonstrates the configuration system:

- Loading TOML configuration files
- Accessing application, logging, and network settings
- Type-safe configuration retrieval
- Default value handling

**Usage**:

```bash
./build/Release/examples/config_example
```

### 2. color_test

**File**: `color_test.cpp`

Tests the logger's color output capabilities:

- Different log levels with colors
- Console output formatting
- Color-coded severity levels

**Usage**:

```bash
./build/Release/examples/color_test
```

### 3. logger_config_demo

**File**: `logger_config_demo.cpp`

Shows advanced logger configuration:

- Setting up loggers from configuration files
- Multiple log outputs (console, file)
- Different log levels and formatting
- Integration with the config system

**Usage**:

```bash
./build/Release/examples/logger_config_demo
```

### 4. app_example

**File**: `app_example.cpp`

Basic application framework demonstration:

- Application lifecycle management
- Component system usage
- Signal handling
- Configuration integration
- Thread management

**Usage**:

```bash
./build/Release/examples/app_example
```

### 5. messaging_example

**File**: `messaging_example.cpp`

Comprehensive messaging system demonstration:

- Inter-thread communication
- Message types and priorities
- Microservice-style architecture
- Order processing workflow
- Service coordination

**Features Demonstrated**:

- Order processing service
- Payment processing service
- Inventory management service
- Notification service
- Cross-service messaging

**Usage**:

```bash
./build/Release/examples/messaging_example
```

## Example Output

The messaging example produces output like this:

```
[2025-07-01 22:44:38.180] [crux] [info] === Starting Messaging System Demo ===
[2025-07-01 22:44:38.181] [crux] [info] Created 4 specialized service threads
[2025-07-01 22:44:38.191] [crux] [info] Processing order #1: 2 x Laptop @ $150.00
[2025-07-01 22:44:38.203] [crux] [info] Payment successful for order #2: $750.00
[2025-07-01 22:44:43.698] [crux] [info] Demo completed successfully!
```

## Configuration Files

### example_app.toml

Provides configuration for various examples:

- Application metadata (name, version, description)
- Logging configuration (levels, outputs, formatting)
- Network settings (host, port, timeouts)
- Custom settings for specific examples

## Learning Path

Recommended order for exploring the examples:

1. **color_test** - See basic logging output
2. **config_example** - Learn configuration system
3. **logger_config_demo** - Advanced logging setup
4. **app_example** - Basic application framework
5. **messaging_example** - Advanced inter-thread messaging

## Integration Examples

Each example demonstrates integration between different framework components:

- **Configuration + Logging**: How to configure loggers from TOML files
- **Application + Components**: Component lifecycle management
- **Threading + Messaging**: Safe inter-thread communication
- **Signals + Shutdown**: Graceful application shutdown

## Extending Examples

You can use these examples as starting points for your own applications:

1. Copy an example file
2. Modify it for your use case
3. Add it to the CMakeLists.txt
4. Build and test

## Troubleshooting

If examples don't build:

1. Ensure all dependencies are installed (see main README)
2. Check that C++20 support is available
3. Verify Conan packages are properly installed
4. Check compiler error messages for missing includes

For runtime issues:

1. Check console output for error messages
2. Ensure configuration files are in the correct location
3. Verify file permissions for log outputs
4. Check available system resources (threads, memory)
