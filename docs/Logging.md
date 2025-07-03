# Logger System Documentation

## Overview

The base logger system provides a modern C++20 thread-safe logging wrapper for spdlog with enhanced features including automatic source location, colored console output, structured logging, and seamless configuration integration.

## Features

### 🚀 **Core Features**

- **C++20 Source Location**: Automatic file, line, and function information
- **Thread-Safe Logging**: Built on spdlog's thread-safe foundation
- **Colored Console Output**: Configurable ANSI color support
- **Multiple Output Sinks**: Console and rotating file logging
- **Type-Safe Log Levels**: Enum-based level specification
- **Structured Logging**: Format string with type-safe arguments
- **Configuration Integration**: Seamless TOML configuration support
- **Performance Optimized**: Conditional logging and efficient formatting

### 🎨 **Visual Features**

- **Colored Log Levels**: Different colors for each severity level
- **Customizable Patterns**: Flexible log message formatting
- **Source Location Display**: Optional file/line information
- **Multiple Sink Support**: Console + file logging simultaneously

## Quick Start

### Basic Usage

```cpp
#include "logger.h"
using namespace base;

int main() {
    // Initialize with default console logging
    Logger::init();

    // Log messages at different levels
    Logger::info("Application started successfully");
    Logger::warn("Memory usage is high: {}%", 85);
    Logger::error("Failed to connect to database: {}", error_message);
    Logger::debug("Processing user request: {}", user_id);

    return 0;
}
```

### Advanced Configuration

```cpp
#include "logger.h"
using namespace base;

int main() {
    // Configure logger with custom settings
    LoggerConfig config{
        .app_name = "MyApp",
        .log_file = "logs/app.log",
        .max_file_size = 10 * 1024 * 1024,  // 10MB
        .max_files = 5,
        .level = LogLevel::Debug,
        .enable_console = true,
        .enable_file = true,
        .enable_colors = true,
        .pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%s:%#] %v"
    };

    Logger::init(config);

    // Now logging includes file/line information and colors
    Logger::debug("Debug information with source location");
    Logger::info("Colored info message");
    Logger::error("Error with automatic file:line information");

    return 0;
}
```

## Log Levels

### Available Levels

```cpp
enum class LogLevel : int {
    Trace = 0,      // Detailed trace information
    Debug = 1,      // Debug information for development
    Info = 2,       // General information (default)
    Warn = 3,       // Warning messages
    Error = 4,      // Error messages
    Critical = 5,   // Critical errors
    Off = 6         // Disable all logging
};
```

### Level Colors (Default)

- **TRACE**: Gray/White
- **DEBUG**: Cyan
- **INFO**: Green
- **WARNING**: Yellow
- **ERROR**: Red
- **CRITICAL**: Bright Red/Magenta

### Setting Log Levels

```cpp
// Set level using type-safe enum
Logger::set_level(LogLevel::Debug);

// Get current level
LogLevel current = Logger::get_level();

// Check if logger is initialized
if (Logger::is_initialized()) {
    Logger::info("Logger is ready");
}
```

## Configuration

### LoggerConfig Structure

```cpp
struct LoggerConfig {
    std::string app_name = "base";                           // Logger name
    std::filesystem::path log_file{};                        // Optional log file
    std::size_t max_file_size = 5 * 1024 * 1024;           // 5MB file size limit
    std::size_t max_files = 3;                              // Number of rotating files
    LogLevel level = LogLevel::Info;                         // Minimum log level
    bool enable_console = true;                              // Enable console output
    bool enable_file = false;                                // Enable file output
    bool enable_colors = true;                               // Enable colored output
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"; // Log pattern
};
```

### Initialization Methods

```cpp
// Method 1: Default console logger
Logger::init();

// Method 2: Custom configuration
LoggerConfig config{
    .app_name = "MyService",
    .level = LogLevel::Debug,
    .enable_colors = true
};
Logger::init(config);

// Method 3: Legacy file logger (deprecated)
Logger::init("MyApp", "app.log");  // Will show deprecation warning
```

### Pattern Format Specifiers

| Specifier     | Description            | Example           |
| ------------- | ---------------------- | ----------------- |
| `%Y-%m-%d`    | Date                   | `2025-07-01`      |
| `%H:%M:%S.%e` | Time with milliseconds | `14:30:25.123`    |
| `%n`          | Logger name            | `MyApp`           |
| `%l`          | Log level              | `info`            |
| `%^%l%$`      | Colored log level      | `info` (in green) |
| `%v`          | Message                | `User logged in`  |
| `%s:%#`       | Source file:line       | `main.cpp:42`     |
| `%!`          | Function name          | `main`            |

## Advanced Features

### Source Location (C++20)

The logger automatically captures source location information:

```cpp
Logger::info("This message includes file and line info");
// Output: [2025-07-01 14:30:25.123] [MyApp] [info] [main.cpp:15] This message includes file and line info
```

### Structured Logging

Type-safe format strings with automatic argument handling:

```cpp
// Basic formatting
Logger::info("User {} logged in from IP {}", username, ip_address);

// Complex structures
Logger::debug("Request processed: method={}, url={}, status={}, duration={}ms",
              method, url, status_code, duration);

// Different data types
Logger::warn("Memory usage: {:.2f}% ({} MB / {} MB)",
             percentage, used_mb, total_mb);
```

### Conditional Logging

Logging calls are optimized and only processed if the level is enabled:

```cpp
// This debug call has zero overhead if debug level is disabled
Logger::debug("Expensive operation result: {}", compute_expensive_debug_info());

// Explicit level checking for very expensive operations
if (Logger::get_level() <= LogLevel::Debug) {
    std::string expensive_data = generate_debug_report();
    Logger::debug("Debug report: {}", expensive_data);
}
```

### Multiple Output Sinks

Configure both console and file output simultaneously:

```cpp
LoggerConfig config{
    .app_name = "MultiSink",
    .log_file = "logs/app.log",
    .enable_console = true,    // Console output with colors
    .enable_file = true,       // File output with rotation
    .enable_colors = true      // Colors only for console
};
Logger::init(config);

// This message goes to both console (colored) and file (plain text)
Logger::info("Message logged to both console and file");
```

### File Rotation

Automatic log file rotation when size limits are reached:

```cpp
LoggerConfig config{
    .log_file = "logs/app.log",
    .max_file_size = 10 * 1024 * 1024,  // 10MB per file
    .max_files = 5,                      // Keep 5 files
    .enable_file = true
};
Logger::init(config);

// Files created: app.log, app.1.log, app.2.log, app.3.log, app.4.log
// Oldest files are automatically deleted when limit is reached
```

## Integration with Configuration System

### TOML Configuration

The logger seamlessly integrates with the TOML configuration system:

```toml
[myapp.logging]
level = "debug"
pattern = "[%H:%M:%S] [%^%l%$] %v"
enable_console = true
enable_file = true
file_path = "logs/myapp.log"
max_file_size = 10485760  # 10MB
max_files = 3
flush_immediately = false
enable_colors = true
```

### Automatic Configuration

```cpp
#include "config.h"
#include "logger.h"

int main() {
    // Load TOML configuration
    auto& config = ConfigManager::instance();
    config.load_config("app.toml", "myapp");

    // Automatically configure logger from TOML
    config.configure_logger("myapp");

    // Logger is now configured according to TOML settings
    Logger::info("Logger configured from TOML file");

    return 0;
}
```

## Color Configuration

### Enabling/Disabling Colors

```cpp
// Enable colors (default)
LoggerConfig with_colors{
    .enable_colors = true,
    .pattern = "[%H:%M:%S] [%^%l%$] %v"  // %^%l%$ for colored level
};

// Disable colors
LoggerConfig no_colors{
    .enable_colors = false,
    .pattern = "[%H:%M:%S] [%l] %v"      // Plain level without color codes
};
```

### Testing Color Support

Run the included color test to verify your terminal supports colors:

```bash
cd build/Release && ./color_test
```

The test will show:

1. Direct ANSI color codes
2. Logger output with colors enabled
3. Logger output with colors disabled
4. All log levels with different colors

## Best Practices

### 1. Initialize Early

```cpp
int main() {
    // Initialize logger as early as possible
    Logger::init();

    Logger::info("Application starting");
    // ... rest of application
}
```

### 2. Use Appropriate Log Levels

```cpp
// Good: Use appropriate levels
Logger::debug("Variable value: x={}", x);           // Development info
Logger::info("User {} logged in", username);        // Important events
Logger::warn("Disk space low: {}% remaining", pct); // Potential issues
Logger::error("Failed to save file: {}", filename); // Errors
Logger::critical("System out of memory");           // Critical failures

// Avoid: Misusing levels
Logger::error("User clicked button");  // This is info, not error
Logger::info("Critical system failure"); // This should be critical
```

### 3. Use Structured Logging

```cpp
// Good: Structured with context
Logger::info("HTTP request processed: method={}, url={}, status={}, duration={}ms",
             request.method, request.url, response.status, duration);

// Avoid: Unstructured messages
Logger::info("Request done");  // Not enough context
```

### 4. Performance Considerations

```cpp
// Good: Efficient logging
Logger::debug("Processing item {}", item.id);

// Avoid: Expensive operations in log calls
Logger::debug("Item details: {}", item.generate_full_report()); // Expensive!

// Better: Conditional expensive logging
if (Logger::get_level() <= LogLevel::Debug) {
    Logger::debug("Item details: {}", item.generate_full_report());
}
```

### 5. Error Handling

```cpp
// Always check logger initialization in critical applications
if (!Logger::is_initialized()) {
    std::cerr << "Failed to initialize logger!" << std::endl;
    return -1;
}

// Proper cleanup
void cleanup() {
    Logger::flush();    // Ensure all messages are written
    Logger::shutdown(); // Clean shutdown
}
```

## Thread Safety

The logger is fully thread-safe and can be used from multiple threads:

```cpp
#include <thread>
#include <vector>

void worker_thread(int id) {
    Logger::info("Worker thread {} started", id);

    for (int i = 0; i < 100; ++i) {
        Logger::debug("Thread {} processing item {}", id, i);
    }

    Logger::info("Worker thread {} finished", id);
}

int main() {
    Logger::init();

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
```

## Testing

### Running Logger Tests

```bash
cd build/Release && ./tests/test_base
```

The test suite covers:

- ✅ Basic logging functionality
- ✅ Log level management
- ✅ Source location integration
- ✅ Configuration-based initialization
- ✅ Structured logging
- ✅ Thread safety
- ✅ Color output

### Color Testing

```bash
cd build/Release && ./color_test
```

This test helps diagnose color output issues and verifies:

- Terminal ANSI color support
- Logger color configuration
- Different log level colors
- Color enable/disable functionality

## Troubleshooting

### Colors Not Showing

1. **Check terminal support**: Run `./color_test` to verify ANSI support
2. **Force colors**: Set `enable_colors = true` in configuration
3. **Check pattern**: Use `%^%l%$` for colored log levels
4. **Environment variables**: Try setting `FORCE_COLOR=1`

### File Logging Issues

1. **Directory permissions**: Ensure log directory is writable
2. **Disk space**: Check available disk space for log files
3. **File rotation**: Verify `max_file_size` and `max_files` settings

### Performance Issues

1. **Log level**: Set appropriate minimum level for production
2. **Async logging**: Consider spdlog's async mode for high-throughput
3. **Expensive formatting**: Use conditional logging for expensive operations

### Memory Issues

1. **Proper shutdown**: Call `Logger::shutdown()` before exit
2. **Flush regularly**: Use `Logger::flush()` for important messages
3. **File handle limits**: Monitor file descriptor usage with many loggers

## Examples

### Complete Application Example

```cpp
#include "logger.h"
#include "config.h"
#include <iostream>

class Application {
public:
    bool initialize() {
        // Configure logger from TOML
        auto& config = ConfigManager::instance();
        if (!config.load_config("app.toml", "myapp")) {
            std::cerr << "Failed to load configuration" << std::endl;
            return false;
        }

        config.configure_logger("myapp");
        Logger::info("Application initialized");
        return true;
    }

    void run() {
        Logger::info("Application starting main loop");

        try {
            // Simulate work
            for (int i = 0; i < 10; ++i) {
                Logger::debug("Processing iteration {}", i);

                if (i == 5) {
                    Logger::warn("Halfway point reached");
                }

                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            Logger::info("Main loop completed successfully");

        } catch (const std::exception& e) {
            Logger::error("Exception in main loop: {}", e.what());
            throw;
        }
    }

    void shutdown() {
        Logger::info("Application shutting down");
        Logger::flush();
        Logger::shutdown();
    }
};

int main() {
    Application app;

    try {
        if (!app.initialize()) {
            return -1;
        }

        app.run();
        app.shutdown();

    } catch (const std::exception& e) {
        Logger::critical("Unhandled exception: {}", e.what());
        return -1;
    }

    return 0;
}
```

### Sample Configuration File

```toml
[myapp]

[myapp.logging]
level = "info"
pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v"
enable_console = true
enable_file = true
file_path = "logs/myapp.log"
max_file_size = 10485760  # 10MB
max_files = 5
flush_immediately = false
enable_colors = true
```

## Migration Guide

### From printf/cout

```cpp
// Old style
printf("User %s logged in with ID %d\n", username.c_str(), user_id);
std::cout << "Processing item " << item_id << std::endl;

// New style
Logger::info("User {} logged in with ID {}", username, user_id);
Logger::debug("Processing item {}", item_id);
```

### From Other Logging Libraries

```cpp
// From spdlog directly
spdlog::info("Message: {}", value);
// To base::Logger
Logger::info("Message: {}", value);

// From other libraries
LOG(INFO) << "Message: " << value;
// To base::Logger
Logger::info("Message: {}", value);
```

The base logger system provides a modern, efficient, and feature-rich logging solution that scales from simple console output to complex production logging scenarios with multiple sinks, rotation, and configuration management.
