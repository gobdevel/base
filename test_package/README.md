# Base Framework Test Package

This comprehensive test package validates and demonstrates the complete functionality of the Base C++20 application framework. It serves as both a verification tool for the Conan package and a comprehensive example for developers getting started with the framework.

## Overview

The test package showcases all major framework components in a realistic application scenario:

- **Application Lifecycle Management**: Graceful startup, shutdown, and signal handling
- **High-Performance Messaging**: Type-safe, multi-threaded message passing with zero-copy optimization
- **Configuration Management**: TOML-based configuration with validation and type safety
- **Structured Logging**: Configurable levels, formatting, and output destinations
- **Singleton Pattern**: Thread-safe lazy initialization with proper cleanup
- **CLI Interface**: Runtime inspection and debugging capabilities
- **Health Monitoring**: Configurable health checks with metrics collection
- **Error Handling**: Comprehensive exception safety and error recovery

## Features Demonstrated

### 1. Advanced Application Framework

```cpp
TestApplication app;
app.run(); // Complete lifecycle management
```

- Multi-threaded worker pool with event-driven message processing
- Graceful shutdown with proper resource cleanup
- Signal handling (SIGINT, SIGTERM) with user notification
- Health monitoring with configurable intervals
- Built-in CLI server for runtime inspection

### 2. High-Performance Messaging System

```cpp
// Type-safe message passing
broadcast_message<StatusMessage>(StatusMessage{"system", "running", 42});

// Event-driven message handlers
thread->subscribe_to_messages<StatusMessage>([](const Message<StatusMessage>& msg) {
    // Process message with zero-copy access
});
```

- Lock-free message queues with cache-aligned data structures
- Type-safe message passing with compile-time validation
- Priority-based message delivery
- Batch processing for optimal performance
- Multi-producer, multi-consumer architecture

### 3. Configuration Management

```cpp
auto& config = ConfigManager::instance();
config.load_from_string(toml_config, "app_name");

auto value = config.get_value<int>("section.key", "app_name");
```

- TOML-based configuration files with full validation
- Type-safe configuration access with default values
- Hierarchical configuration sections
- Runtime configuration updates
- Environment-specific configuration profiles

### 4. Comprehensive Logging

```cpp
Logger::info("Application started: {} v{}", name, version);
Logger::debug("Debug info: {}", debug_data);
Logger::error("Error occurred: {}", error_message);
```

- Structured logging with configurable levels
- Custom formatting patterns
- Multiple output destinations (console, file, network)
- Thread-safe logging with minimal performance impact
- Integration with application lifecycle events

### 5. CLI Interface for Runtime Inspection

The example includes a built-in CLI server (port 8080) with custom commands:

```bash
telnet localhost 8080
> help                    # List available commands
> demo_status            # Show demonstration metrics
> send_test_message      # Send test message
> threads                # Show thread status
> health                 # Show health information
```

### 6. Metrics Collection and Analysis

```cpp
auto& metrics = MetricsCollector::instance();
metrics.record_metric("latency", 45.2, "ms");
metrics.record_event("user_login");
```

- Statistical metrics collection with time-series data
- Event counting and aggregation
- Performance monitoring with automatic cleanup
- Runtime metrics inspection via CLI

## Running the Example

### Option 1: Via Conan Package Test

```bash
conan create . --build=missing
```

This command:

1. Builds the Base framework library
2. Creates a Conan package
3. Builds and runs this test package
4. Validates all framework components

### Option 2: Direct Build and Run

```bash
# Build the framework first
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j

# Build and run test package
cd ../test_package
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
./example
```

### Command Line Options

```bash
./example --help           # Show help message
./example --verbose        # Enable debug logging
./example -v               # Short form of verbose
```

## Expected Output

The example runs for approximately 8 seconds and demonstrates:

1. **Initialization Phase** (0-1s):

   - Framework component initialization
   - Configuration loading and validation
   - CLI server startup (if enabled)

2. **Demonstration Phase** (1-6s):

   - Multi-threaded message processing
   - Real-time metrics collection
   - Health monitoring
   - Task execution simulation
   - CLI command availability

3. **Shutdown Phase** (6-8s):
   - Graceful shutdown sequence
   - Final metrics summary
   - Resource cleanup verification

## Sample Output

```
🚀 Starting Base Framework Comprehensive Test Package Example
   Showcasing: Logging, Configuration, Application Framework, Messaging, CLI, Health Monitoring
   Version: 1.2.0 | Build: Jan 15 2025
   =========================================================================================

🔧 Framework components initialized:
   ✓ Singleton pattern with thread-safe lazy initialization
   ✓ Structured logging with configurable levels and formatting
   ✓ TOML-based configuration management with validation
   ✓ High-performance messaging with zero-copy optimization
   ✓ Application lifecycle with graceful shutdown handling
   ✓ Health monitoring with configurable check intervals
   ✓ CLI interface for runtime inspection and debugging

=== Base Framework Comprehensive Test Package Example ===
Initializing advanced test application components...
✓ Configuration system: Advanced TOML configuration loaded
  - App: base_test_package v1.2.0 (debug: true)
  - Connection limits: 150 max, 30s timeout
  - Performance: batch=128, queue_max=10000
  - Features: CLI=enabled
✓ Messaging system: High-performance typed message queues initialized
✓ CLI interface: Enabled on port 8080

Starting application services and demonstrations...
📊 Status Monitor: system = running (value: 73, age: 2ms)
📈 Metrics Processor: throughput = 67.4 units
🔧 Task Executor: Processing 'Demo task #1' (priority: 2)
...

🎯 Demonstration phase complete - preparing shutdown sequence

📊 =========================
📊 DEMONSTRATION COMPLETED
📊 =========================
Metrics Summary:
  Total events: 89
  Tracked metrics: 15
  - cpu_usage: avg=31.2, latest=28.0 % (samples: 4)
  - memory_usage: avg=445.5, latest=512.0 MB (samples: 4)
  - task_processing_time: avg=32.1, latest=45.0 ms (samples: 12)
...

✅ Base Framework Test Package completed successfully!
   Framework validation: All core components tested and verified
   Ready for: Production deployment and custom application development
```

## Integration Testing

This test package validates:

- ✅ **Build System**: CMake integration with C++20 features
- ✅ **Dependencies**: Proper linking with spdlog, fmt, asio, toml++
- ✅ **Thread Safety**: Multi-threaded message processing without race conditions
- ✅ **Memory Management**: RAII pattern with no memory leaks
- ✅ **Exception Safety**: Proper error handling and resource cleanup
- ✅ **Performance**: Low-latency messaging and efficient resource usage
- ✅ **Portability**: Cross-platform compatibility (Linux, macOS, Windows)

## Architecture Validation

The example demonstrates real-world usage patterns:

1. **Event-Driven Architecture**: Asynchronous message processing
2. **Configuration-Driven Behavior**: Runtime customization without code changes
3. **Observability**: Comprehensive logging and metrics for production monitoring
4. **Graceful Degradation**: Proper error handling and recovery mechanisms
5. **Developer Experience**: Easy-to-use APIs with compile-time safety

## Development Guidance

This example serves as a template for Base framework applications:

- Copy and modify for your specific use case
- Follow the demonstrated patterns for optimal performance
- Use the CLI interface pattern for debugging and monitoring
- Implement health checks for production deployment
- Leverage the configuration system for environment-specific settings

For more information, see the main Base framework documentation and examples in the `examples/` directory.
