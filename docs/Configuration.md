# Configuration System Documentation

## Overview

The base configuration system provides a comprehensive, type-safe, and thread-safe configuration management solution using TOML files. It supports per-application configuration, automatic logger integration, and flexible value retrieval with defaults.

## Features

### 🚀 **Core Features**

- **TOML-based configuration** using toml++ library
- **Per-application configuration** support
- **Thread-safe** configuration access
- **Automatic logger integration** with spdlog
- **Type-safe value retrieval** with compile-time type checking
- **Configuration templates** generation
- **Hot reloading** support
- **Singleton pattern** for global access

### 📋 **Built-in Configuration Sections**

#### Application Configuration (`AppConfig`)

```cpp
struct AppConfig {
    std::string name = "base_app";
    std::string version = "1.0.0";
    std::string description;
    bool debug_mode = false;
    int worker_threads = 1;
    std::string working_directory;
};
```

#### Logging Configuration (`LoggingConfig`)

```cpp
struct LoggingConfig {
    LogLevel level = LogLevel::Info;
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v";
    std::string file_path;
    bool enable_console = true;
    bool enable_file = false;
    size_t max_file_size = 10 * 1024 * 1024; // 10MB
    size_t max_files = 5;
    bool flush_immediately = false;
};
```

#### Network Configuration (`NetworkConfig`)

```cpp
struct NetworkConfig {
    std::string host = "localhost";
    int port = 8080;
    int timeout_seconds = 30;
    int max_connections = 100;
    bool enable_ssl = false;
    std::string ssl_cert_path;
    std::string ssl_key_path;
};
```

## Usage Examples

### Basic Configuration Loading

```cpp
#include "config.h"
using namespace base;

// Get the configuration manager instance
auto& config = ConfigManager::instance();

// Load configuration from file
if (!config.load_config("app.toml", "myapp")) {
    std::cerr << "Failed to load configuration!" << std::endl;
    return -1;
}

// Get application configuration
auto app_config = config.get_app_config("myapp");
std::cout << "App: " << app_config.name << " v" << app_config.version << std::endl;
```

### Configuration from String

```cpp
std::string toml_config = R"(
[myapp]

[myapp.app]
name = "my_application"
version = "2.0.0"
debug_mode = true

[myapp.logging]
level = "debug"
enable_console = true
enable_file = true
file_path = "logs/app.log"
)";

auto& config = ConfigManager::instance();
config.load_from_string(toml_config, "myapp");
```

### Logger Integration

```cpp
auto& config = ConfigManager::instance();
config.load_config("app.toml", "myapp");

// Automatically configure logger with config settings
config.configure_logger("myapp");

// Now use the logger with configured settings
Logger::info("Application started");
Logger::debug("Debug message (only shown if debug level enabled)");
```

### Custom Value Retrieval

```cpp
auto& config = ConfigManager::instance();

// Get values with automatic type conversion
auto db_host = config.get_value<std::string>("database.host", "myapp");
auto db_port = config.get_value<int>("database.port", "myapp");
auto enable_cache = config.get_value<bool>("cache.enabled", "myapp");

// Get values with defaults
auto timeout = config.get_value_or<int>("network.timeout", 30, "myapp");
auto max_conn = config.get_value_or<int>("database.max_connections", 10, "myapp");

if (db_host.has_value()) {
    std::cout << "Database: " << *db_host << ":" << db_port.value_or(5432) << std::endl;
}
```

### Multi-Application Configuration

```cpp
auto& config = ConfigManager::instance();

// Load configurations for different applications
config.load_config("config.toml", "web_server");
config.load_config("config.toml", "worker_service");
config.load_config("config.toml", "background_job");

// Configure loggers for each application
config.configure_logger("web_server");
config.configure_logger("worker_service");
config.configure_logger("background_job");

// Get different configurations
auto web_config = config.get_network_config("web_server");
auto worker_config = config.get_app_config("worker_service");
```

## Configuration File Format

### Basic Structure

```toml
# Configuration for 'myapp'
[myapp]

# Application settings
[myapp.app]
name = "my_application"
version = "1.0.0"
description = "My awesome application"
debug_mode = false
worker_threads = 4
working_directory = "/var/lib/myapp"

# Logging configuration
[myapp.logging]
level = "info"  # debug, info, warn, error, critical
pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v"
enable_console = true
enable_file = true
file_path = "logs/myapp.log"
max_file_size = 10485760  # 10MB
max_files = 5
flush_immediately = false

# Network configuration
[myapp.network]
host = "0.0.0.0"
port = 8080
timeout_seconds = 30
max_connections = 100
enable_ssl = true
ssl_cert_path = "/etc/ssl/certs/myapp.crt"
ssl_key_path = "/etc/ssl/private/myapp.key"

# Custom sections
[myapp.database]
host = "db.example.com"
port = 5432
name = "myapp_db"
user = "myapp_user"
password = "secure_password"
max_connections = 20
connection_timeout = 30

[myapp.cache]
redis_host = "cache.example.com"
redis_port = 6379
ttl_seconds = 3600
max_memory = "256mb"

[myapp.features]
enable_metrics = true
enable_tracing = true
enable_profiling = false
```

### Multiple Applications in One File

```toml
# Web server configuration
[webserver]

[webserver.app]
name = "web_server"
version = "1.0.0"
worker_threads = 8

[webserver.network]
host = "0.0.0.0"
port = 80

[webserver.logging]
level = "info"
enable_file = true
file_path = "logs/webserver.log"

# Background worker configuration
[worker]

[worker.app]
name = "background_worker"
version = "1.0.0"
worker_threads = 2

[worker.logging]
level = "debug"
enable_console = false
enable_file = true
file_path = "logs/worker.log"
```

## API Reference

### ConfigManager Class

#### Loading Configuration

```cpp
// Load from file
bool load_config(const std::filesystem::path& config_path,
                std::string_view app_name = "default");

// Load from string
bool load_from_string(std::string_view toml_content,
                     std::string_view app_name = "default");

// Reload from last file
bool reload_config();
```

#### Getting Configuration Sections

```cpp
LoggingConfig get_logging_config(std::string_view app_name = "default") const;
AppConfig get_app_config(std::string_view app_name = "default") const;
NetworkConfig get_network_config(std::string_view app_name = "default") const;
```

#### Custom Value Retrieval

```cpp
template<typename T>
std::optional<T> get_value(std::string_view key,
                          std::string_view app_name = "default") const;

template<typename T>
T get_value_or(std::string_view key, const T& default_value,
               std::string_view app_name = "default") const;
```

#### Utility Methods

```cpp
bool has_app_config(std::string_view app_name) const;
std::vector<std::string> get_app_names() const;
bool configure_logger(std::string_view app_name = "default",
                     std::string_view logger_name = "") const;
static bool create_config_template(const std::filesystem::path& config_path,
                                  std::string_view app_name = "default");
void clear();
```

## Best Practices

### 1. Use Meaningful Application Names

```cpp
// Good
config.load_config("app.toml", "user_service");
config.load_config("app.toml", "payment_processor");

// Avoid generic names
config.load_config("app.toml", "app1");
```

### 2. Validate Configuration on Startup

```cpp
auto& config = ConfigManager::instance();
if (!config.load_config("app.toml", "myapp")) {
    std::cerr << "Failed to load configuration - exiting" << std::endl;
    return EXIT_FAILURE;
}

// Validate required settings
auto db_host = config.get_value<std::string>("database.host", "myapp");
if (!db_host.has_value()) {
    std::cerr << "Database host not configured - exiting" << std::endl;
    return EXIT_FAILURE;
}
```

### 3. Use Configuration Templates

```cpp
// Generate template for new deployments
ConfigManager::create_config_template("config/production.toml", "myapp");
```

### 4. Separate Environments

```cpp
// Different configs for different environments
#ifdef DEBUG
    config.load_config("config/development.toml", "myapp");
#else
    config.load_config("config/production.toml", "myapp");
#endif
```

### 5. Thread-Safe Access

```cpp
// ConfigManager is thread-safe - safe to call from multiple threads
void worker_thread() {
    auto& config = ConfigManager::instance();
    auto timeout = config.get_value_or<int>("worker.timeout", 30, "myapp");
    // Use timeout...
}
```

## Error Handling

The configuration system provides robust error handling:

```cpp
auto& config = ConfigManager::instance();

// File loading errors
if (!config.load_config("nonexistent.toml", "myapp")) {
    // Handle file not found, parse errors, etc.
}

// Invalid TOML syntax
if (!config.load_from_string("invalid toml [", "myapp")) {
    // Handle parse errors
}

// Missing values return std::nullopt
auto missing = config.get_value<std::string>("nonexistent.key", "myapp");
if (!missing.has_value()) {
    // Handle missing configuration
}

// Use defaults for missing values
auto safe_value = config.get_value_or<int>("missing.value", 42, "myapp");
```

## Integration with Other Systems

### Logger Integration

The configuration system seamlessly integrates with the crux logger system. See [LOGGER_README.md](LOGGER_README.md) for detailed logger documentation.

```cpp
// Automatic logger configuration from TOML
auto& config = ConfigManager::instance();
config.load_config("app.toml", "myapp");
config.configure_logger("myapp");  // Logger now uses TOML settings

Logger::info("Logger configured with TOML settings");
```

### CMake Integration

```cmake
find_package(tomlplusplus REQUIRED)
target_link_libraries(your_target tomlplusplus::tomlplusplus crux)
```

### Conan Integration

```python
def requirements(self):
    self.requires("tomlplusplus/3.4.0")
    self.requires("spdlog/1.15.3", transitive_headers=True)
```

## Performance Considerations

- **Lazy Loading**: Configuration is loaded only when requested
- **Thread Safety**: Uses shared_mutex for optimal read performance
- **Memory Efficient**: TOML parsing is done once, cached in memory
- **Type Safety**: Template-based value retrieval avoids runtime type errors

## Testing

The configuration system includes comprehensive tests covering:

- ✅ Configuration loading from files and strings
- ✅ TOML parsing and error handling
- ✅ Multi-application support
- ✅ Thread safety
- ✅ Custom value retrieval
- ✅ Logger integration
- ✅ Template generation
- ✅ Configuration reloading

Run tests with:

```bash
cd build/Release && ./tests/test_crux
```

## Example Application

See `config_example.cpp` for a complete demonstration of all features:

```bash
cd build/Release && ./config_example
```

This creates a `demo_config.toml` template and demonstrates all configuration features.
