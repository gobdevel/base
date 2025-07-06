/\*

- @file component_logging_documentation.md
- @brief Documentation for Component-Level Logging and Filtering
-
- Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
- SPDX-License-Identifier: MIT
  \*/

# Component-Level Logging and Filtering

## Overview

The enhanced logger infrastructure supports component-level logging and filtering, allowing you to:

- Tag log messages with component names (e.g., "database", "network", "auth")
- Filter log output to show only specific components
- Enable/disable components dynamically at runtime
- Maintain separate filtering rules for different parts of your application

## Configuration

### LoggerConfig Options

```cpp
struct LoggerConfig {
    // Component-level logging configuration
    bool enable_component_logging = true;           ///< Enable component-based logging
    std::vector<std::string> enabled_components{};  ///< Only log these components (empty = all)
    std::vector<std::string> disabled_components{};  ///< Exclude these components from logging
    std::string component_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v";  ///< Pattern for component logging
};
```

### Component Filtering Logic

- **enable_component_logging**: Master switch for component features
- **enabled_components**: Whitelist of components (empty = allow all)
- **disabled_components**: Blacklist of components (takes precedence over whitelist)
- **component_pattern**: Log pattern when component logging is enabled

## API Usage

### Basic Component Logging

```cpp
// Initialize logger with component logging
LoggerConfig config{
    .enable_component_logging = true
};
Logger::init(config);

// Log with component tags
Logger::info("database", "Connection established to {}", server_name);
Logger::warn("network", "Timeout occurred after {}ms", timeout_ms);
Logger::error("auth", "Login failed for user {}", username);

// Regular logging (no component)
Logger::info("Application started successfully");
```

### Component Filtering

```cpp
// Disable specific components
Logger::disable_components({"database", "cache"});

// Enable only specific components (whitelist mode)
Logger::enable_components({"auth", "security"});

// Clear all filters (enable all components)
Logger::clear_component_filters();

// Check if a component is enabled
if (Logger::is_component_enabled("database")) {
    // Expensive logging operation
}

// Get current filter state
auto enabled = Logger::get_enabled_components();
auto disabled = Logger::get_disabled_components();
```

### Runtime Component Management

```cpp
// Dynamically adjust component filters based on debug level
if (debug_mode) {
    Logger::enable_components({"database", "network", "cache"});
} else {
    Logger::disable_components({"database", "cache"});
}

// Component-specific log levels can be simulated
if (Logger::is_component_enabled("verbose_component")) {
    Logger::trace("verbose_component", "Detailed trace information");
}
```

## Log Output Format

### With Component Logging Enabled

```
[2025-07-05 23:30:21.987] [info] [TestApp] [database] Connection established
[2025-07-05 23:30:21.988] [warn] [TestApp] [network] Timeout after 5000ms
[2025-07-05 23:30:21.989] [error] [TestApp] [auth] Invalid credentials
```

### Component Filtering in Action

```cpp
Logger::disable_components({"database"});

Logger::info("database", "This will NOT appear");
Logger::info("network", "This will appear");
Logger::info("auth", "This will appear");
```

## Best Practices

### Component Naming

- Use lowercase, descriptive names: `database`, `network`, `auth`, `cache`
- Use consistent naming across your application
- Consider hierarchical names for complex systems: `db.connection`, `db.query`, `net.http`, `net.tcp`

### Performance Considerations

- Component filtering happens at log call time (early exit)
- Use `is_component_enabled()` for expensive logging operations
- Component state is stored in static variables for fast access

### Integration with Application Framework

```cpp
class MyApp : public Application {
protected:
    bool on_initialize() override {
        LoggerConfig config{
            .app_name = "MyApp",
            .enable_component_logging = true,
            .enabled_components = get_debug_components(),  // From config
            .disabled_components = get_disabled_components()
        };
        Logger::init(config);
        return true;
    }

private:
    std::vector<std::string> get_debug_components() {
        if (config().debug_mode) {
            return {"database", "network", "auth", "cache"};
        }
        return {"auth", "security"};  // Production components only
    }
};
```

### Command Line Integration

```cpp
// In application command line parsing
if (args.has("--debug-components")) {
    auto components = parse_component_list(args.get("--debug-components"));
    Logger::enable_components(components);
}

if (args.has("--disable-components")) {
    auto components = parse_component_list(args.get("--disable-components"));
    Logger::disable_components(components);
}
```

## Implementation Notes

- Component filtering is thread-safe
- Filter state persists until explicitly changed
- Component names are case-sensitive
- Empty component name ("") logs without component tag
- Component logging can be disabled entirely with `enable_component_logging = false`

## Future Enhancements

- Per-component log levels
- Pattern-based component filtering (regex)
- Runtime configuration reload
- Component hierarchy support
- Performance metrics per component
