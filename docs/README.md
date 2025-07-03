# Base Framework Documentation

This directory contains detailed documentation for each component of the Base framework.

## Component Documentation

### 🏗️ [Application Framework](APPLICATION_README.md)

Complete guide to the event-driven application architecture, including:

- Event loop management
- Thread lifecycle
- Signal handling
- Worker thread pools
- Custom thread creation

### 📡 [Messaging System](MESSAGING_README.md)

Comprehensive documentation for the type-safe messaging system:

- Message types and priorities
- Publish/subscribe patterns
- Thread-to-thread communication
- Performance characteristics
- Integration with managed threads

### ⚡ [Event-Driven Messaging](EVENT_DRIVEN_README.md)

Advanced event-driven messaging with immediate processing:

- Condition variable-based notifications
- Sub-microsecond latency
- 100K+ messages/second throughput
- Zero polling overhead
- Real-time processing capabilities

### 📝 [Logger System](LOGGER_README.md)

In-depth guide to the high-performance logging system:

- Log levels and formatting
- Color output support
- Async logging capabilities
- Configuration options
- spdlog integration

### ⚙️ [Configuration System](CONFIG_README.md)

Detailed documentation for TOML-based configuration:

- Configuration file management
- Type-safe access patterns
- Runtime configuration updates
- Environment integration
- Default value handling

### 🔧 [CLI - Runtime Inspection & Debugging](CLI_README.md)

Complete guide to the runtime CLI for application inspection:

- Built-in monitoring commands
- Custom command registration
- TCP and stdin interfaces
- Application state inspection
- Runtime debugging capabilities

- Type-safe configuration access
- Nested configuration handling
- Default value management
- Error handling
- Live configuration reloading

## Quick Navigation

| Component                              | Purpose                    | Key Features                      |
| -------------------------------------- | -------------------------- | --------------------------------- |
| [Application](APPLICATION_README.md)   | Framework foundation       | Event loops, threads, signals     |
| [Messaging](MESSAGING_README.md)       | Inter-thread communication | Type safety, priorities, pub/sub  |
| [Event-Driven](EVENT_DRIVEN_README.md) | Real-time messaging        | Immediate processing, low latency |
| [Logger](LOGGER_README.md)             | High-performance logging   | Async, colors, structured output  |
| [Config](CONFIG_README.md)             | Configuration management   | TOML, type safety, validation     |

## Additional Resources

- [Main README](../README.md) - Project overview and quick start
- [Examples](../examples/README.md) - Practical usage examples
- [Tests](../tests/) - Unit tests and integration tests

## API Documentation

For detailed API documentation, see the Doxygen comments in the header files:

- [`include/application.h`](../include/application.h)
- [`include/messaging.h`](../include/messaging.h)
- [`include/logger.h`](../include/logger.h)
- [`include/config.h`](../include/config.h)
- [`include/singleton.h`](../include/singleton.h)

## Contributing to Documentation

When contributing to the framework:

1. Update relevant component documentation
2. Add examples to demonstrate new features
3. Include performance notes for significant changes
4. Update this index if adding new components

## Documentation Standards

- Use clear, concise language
- Include code examples for all major features
- Document performance characteristics
- Provide troubleshooting guidance
- Link between related concepts

---

_Last updated: Documentation reflects the current state of the Base framework with full C++20 modernization and messaging system integration._
