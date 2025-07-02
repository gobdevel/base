# Crux Framework

A modern C++20 application framework designed for building high-performance, scalable applications with sophisticated thread management and inter-process messaging capabilities.

## Features

### 🚀 Modern C++20 Design

- Type-safe APIs using concepts and templates
- RAII resource management throughout
- Source location support for enhanced debugging
- Standard library integration

### 🔧 Core Components

- **Logger System**: High-performance logging with spdlog integration
- **Configuration Management**: TOML-based configuration with type safety
- **Singleton Utilities**: Thread-safe singleton pattern implementation
- **Application Framework**: Event-driven architecture with ASIO

### 🔄 Advanced Threading

- **Managed Thread System**: Isolated event loops with lifecycle management
- **Custom Thread Creation**: User-defined thread functions with ASIO integration
- **Worker Thread Pool**: Configurable thread pool for task processing
- **Thread-Safe Operations**: Lock-free and mutex-protected operations

### 📡 Messaging System

- **Type-Safe Messaging**: Template-based compile-time type checking
- **Priority Queues**: Critical/High/Normal/Low priority message handling
- **Publish/Subscribe**: Event-driven message routing
- **High Performance**: >600K messages/second throughput
- **Thread-Safe**: All operations are thread-safe

## Quick Start

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.15+
- Conan 2.0+ for dependency management

### Installation

1. **Clone the repository**:

   ```bash
   git clone <repository-url>
   cd crux
   ```

2. **Install dependencies**:

   ```bash
   conan install . --output-folder=build/Release --build=missing
   ```

3. **Configure and build**:

   ```bash
   cmake --preset conan-release
   cmake --build build/Release
   ```

4. **Run tests**:
   ```bash
   ./build/Release/tests/test_crux
   ```

### Basic Usage

```cpp
#include "application.h"
#include "messaging.h"

using namespace crux;

// Define your message types
struct TaskMessage {
    int task_id;
    std::string description;
};

// Create your application
class MyApp : public Application {
protected:
    bool on_start() override {
        // Create worker threads
        auto worker = create_worker_thread("task-processor");

        // Set up message handling
        worker->subscribe_to_messages<TaskMessage>([](const Message<TaskMessage>& msg) {
            Logger::info("Processing task {}: {}",
                        msg.data().task_id, msg.data().description);
        });

        // Send messages
        send_message_to_thread("task-processor",
            TaskMessage{1, "Process data"}, MessagePriority::High);

        return true;
    }
};

int main() {
    MyApp app;
    return app.run();
}
```

## Project Structure

```
crux/
├── include/          # Public headers
│   ├── application.h # Application framework
│   ├── config.h      # Configuration system
│   ├── logger.h      # Logging utilities
│   ├── messaging.h   # Messaging system
│   └── singleton.h   # Singleton pattern
├── src/              # Implementation files
│   ├── application.cpp
│   ├── config.cpp
│   ├── logger.cpp
│   └── messaging.cpp
├── examples/         # Example applications
│   ├── app_example.cpp
│   ├── messaging_example.cpp
│   ├── config_example.cpp
│   └── README.md
├── tests/            # Unit tests
│   ├── test_application.cpp
│   ├── test_messaging.cpp
│   └── ...
└── docs/             # Documentation
    ├── APPLICATION_README.md
    ├── MESSAGING_README.md
    ├── LOGGER_README.md
    └── CONFIG_README.md
```

## Examples

The framework includes comprehensive examples in the `examples/` directory:

### 🎯 Available Examples

1. **config_example** - Configuration system demonstration
2. **color_test** - Logger color output testing
3. **logger_config_demo** - Advanced logger configuration
4. **app_example** - Basic application framework usage
5. **messaging_example** - Inter-thread messaging demonstration

### 🏃 Running Examples

```bash
# Build all examples
make -C build/Release

# Run the messaging system demo
./build/Release/examples/messaging_example

# Test logger colors
./build/Release/examples/color_test

# Configuration system demo
./build/Release/examples/config_example
```

See [`examples/README.md`](examples/README.md) for detailed information about each example.

## Documentation

### 📚 Component Documentation

- [**Application Framework**](docs/APPLICATION_README.md) - Event-driven application architecture
- [**Messaging System**](docs/MESSAGING_README.md) - Inter-thread communication
- [**Logger System**](docs/LOGGER_README.md) - High-performance logging
- [**Configuration System**](docs/CONFIG_README.md) - TOML-based configuration

### 🧪 Testing

The framework includes comprehensive test coverage:

```bash
# Run all tests
./build/Release/tests/test_crux

# Run tests with brief output
./build/Release/tests/test_crux --gtest_brief=1

# Run specific test suites
./build/Release/tests/test_crux --gtest_filter="ApplicationFrameworkTest.*"
```

**Test Coverage**: 62 tests across 5 test suites covering all major components.

## Performance

### ⚡ Benchmarks

- **Messaging Throughput**: >600,000 messages/second
- **Message Latency**: Sub-microsecond sending
- **Thread Creation**: ~1ms per managed thread
- **Memory Usage**: Efficient shared_ptr-based message storage

### 🔧 Configuration

The framework is designed for high-performance applications:

- Lock-free operations where possible
- Minimal memory allocations
- Efficient event loop processing
- ASIO-based asynchronous I/O

## Dependencies

### Required

- **C++20** - Modern C++ features
- **CMake 3.15+** - Build system
- **Conan 2.0+** - Package management

### External Libraries

- **spdlog** - High-performance logging
- **toml++** - TOML configuration parsing
- **asio** - Asynchronous I/O (standalone)
- **Google Test** - Unit testing framework

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

### Code Style

- Follow C++ Core Guidelines
- Use modern C++20 features
- Include comprehensive tests
- Document public APIs with Doxygen

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Roadmap

### Planned Features

- **Network Messaging**: Inter-process communication
- **Message Persistence**: Durable message queues
- **Distributed Tracing**: Cross-service observability
- **Plugin System**: Dynamic component loading
- **Performance Tools**: Built-in profiling and metrics

### Version History

- **v1.0.0** - Initial release with core framework and messaging
- **v0.9.0** - Beta release with application framework
- **v0.8.0** - Alpha release with basic components

## Support

- **Documentation**: See the `docs/` directory
- **Examples**: Check the `examples/` directory
- **Issues**: Submit issues via GitHub
- **Questions**: Use GitHub discussions

## Authors

- **Gobind Prasad** - _Initial work_ - gobdeveloper@gmail.com

## Acknowledgments

- Built with modern C++20 features
- Inspired by enterprise application patterns
- Uses high-quality open-source libraries
- Designed for real-world production use
