# ComponentLogger: Automatic Component Prepending

## Overview

The `ComponentLogger` provides a solution to your exact request: **automatic component name prepending without having to pass the component name to every log call**. This approach eliminates the repetitive task of manually specifying component names for each log message.

## Problem Solved

Instead of writing:

```cpp
Logger::info(Logger::Component("Database"), "Connection established");
Logger::debug(Logger::Component("Database"), "Query executed in {}ms", 45);
Logger::warn(Logger::Component("Database"), "Connection pool full");
```

You can now write:

```cpp
auto database = Logger::get_component_logger("Database");
database.info("Connection established");
database.debug("Query executed in {}ms", 45);
database.warn("Connection pool full");
```

## ComponentLogger Features

### 1. Automatic Component Prepending

- Each `ComponentLogger` instance is bound to a specific component name
- All log messages automatically include the component name
- No need to pass component information to each log call

### 2. Same API as Logger

- All standard log levels: `trace()`, `debug()`, `info()`, `warn()`, `error()`, `critical()`
- Same format string support and argument forwarding
- C++20 source location support for file/line information
- Component filtering still applies

### 3. Zero Performance Overhead

- Component name is stored once during logger creation
- Same underlying logging infrastructure as the main Logger
- Level checks and component filtering happen before message formatting

## Usage Patterns

### Basic Usage

```cpp
// Initialize main logger
Logger::init();

// Create component-specific loggers
auto database = Logger::get_component_logger("Database");
auto network = Logger::get_component_logger("Network");
auto auth = Logger::get_component_logger("Authentication");

// Use them like regular loggers - component name is automatic
database.info("Connected to primary server");
database.debug("Query: SELECT * FROM users WHERE active = 1");
database.warn("Query took {}ms (expected <100ms)", 250);

network.info("Starting HTTP server on port {}", 8080);
network.error("Connection timeout after {}s", 30);

auth.info("User '{}' logged in successfully", username);
auth.critical("Multiple failed login attempts from IP {}", client_ip);
```

### Convenience Macros

```cpp
// Create component loggers with clean syntax
COMPONENT_LOGGER(cache);         // Creates: auto cache = Logger::get_component_logger("cache");
COMPONENT_LOGGER(storage);       // Creates: auto storage = Logger::get_component_logger("storage");

// Custom names
COMPONENT_LOGGER_NAMED(db, "Database");     // Creates: auto db = Logger::get_component_logger("Database");
COMPONENT_LOGGER_NAMED(net, "Networking");  // Creates: auto net = Logger::get_component_logger("Networking");

// Use the loggers
cache.info("Cache hit rate: {}%", 92.5);
storage.warn("Disk usage: {}% (approaching limit)", 85);
db.debug("Transaction committed in {}ms", 12);
net.error("Failed to resolve hostname: {}", host);
```

### Class-based Usage Pattern

```cpp
class DatabaseManager {
private:
    base::ComponentLogger logger_;

public:
    DatabaseManager() : logger_(base::Logger::get_component_logger("Database")) {}

    void connect() {
        logger_.info("Connecting to database server");
        // ... connection logic ...
        logger_.info("Database connection established");
    }

    void execute_query(const std::string& query) {
        logger_.debug("Executing query: {}", query);
        // ... query logic ...
        logger_.info("Query executed successfully");
    }
};

class NetworkService {
private:
    base::ComponentLogger logger_;

public:
    NetworkService() : logger_(base::Logger::get_component_logger("Network")) {}

    void start_server(int port) {
        logger_.info("Starting HTTP server on port {}", port);
        // ... server logic ...
        logger_.info("Server started successfully");
    }
};
```

## Component Filtering

ComponentLoggers respect all the same filtering rules as manual component logging:

```cpp
auto database = Logger::get_component_logger("Database");
auto network = Logger::get_component_logger("Network");
auto auth = Logger::get_component_logger("Authentication");

// Enable only specific components
Logger::enable_components({"Database", "Authentication"});

database.info("This will appear");      // ✓ Database is enabled
network.info("This will NOT appear");   // ✗ Network is not enabled
auth.info("This will appear");          // ✓ Authentication is enabled

// Disable specific components
Logger::clear_component_filters();
Logger::disable_components({"Network"});

database.info("This will appear");      // ✓ Database is not disabled
network.info("This will NOT appear");   // ✗ Network is disabled
auth.info("This will appear");          // ✓ Authentication is not disabled
```

## Output Format

ComponentLogger produces the same output format as manual component logging:

```
[2025-07-06 00:05:03.264] [base] [info] [Database] Connection established
[2025-07-06 00:05:03.264] [base] [error] [Network] Connection timeout after 30 seconds
[2025-07-06 00:05:03.264] [base] [critical] [Authentication] Unauthorized access detected!
```

## Best Practices

### 1. Create Component Loggers Once

```cpp
// Good: Create once, use many times
class ServiceManager {
private:
    ComponentLogger logger_;
public:
    ServiceManager() : logger_(Logger::get_component_logger("ServiceManager")) {}

    void method1() { logger_.info("Method 1 called"); }
    void method2() { logger_.debug("Method 2 processing"); }
};

// Avoid: Creating logger each time
void some_function() {
    auto logger = Logger::get_component_logger("SomeFunction");  // Creates every call
    logger.info("Function called");
}
```

### 2. Use Descriptive Component Names

```cpp
// Good: Clear, hierarchical names
auto api_logger = Logger::get_component_logger("API::UserService");
auto db_logger = Logger::get_component_logger("Database::Connection");
auto cache_logger = Logger::get_component_logger("Cache::Redis");

// Okay: Simple names
auto user_logger = Logger::get_component_logger("User");
auto auth_logger = Logger::get_component_logger("Auth");
```

### 3. Combine with Regular Logger When Appropriate

```cpp
// Use ComponentLogger for component-specific messages
auto db_logger = Logger::get_component_logger("Database");
db_logger.info("Query executed in {}ms", duration);

// Use regular Logger for application-level messages
Logger::info("Application starting up");
Logger::critical("System shutdown initiated");
```

## Migration from Manual Component Logging

### Before (Manual)

```cpp
Logger::info(Logger::Component("Database"), "Connection established");
Logger::debug(Logger::Component("Database"), "Query: {}", sql);
Logger::warn(Logger::Component("Database"), "Slow query: {}ms", time);
Logger::error(Logger::Component("Database"), "Connection failed: {}", error);
```

### After (Automatic)

```cpp
auto database = Logger::get_component_logger("Database");
database.info("Connection established");
database.debug("Query: {}", sql);
database.warn("Slow query: {}ms", time);
database.error("Connection failed: {}", error);
```

### Migration Benefits

- **Reduced code repetition**: No need to specify component for every call
- **Cleaner syntax**: More readable and maintainable code
- **Same functionality**: All filtering and formatting features preserved
- **Better performance**: Component name stored once instead of passed each time
- **Type safety**: ComponentLogger provides the same type safety as Logger

## Implementation Details

- `ComponentLogger` is a lightweight wrapper around the main `Logger` class
- Component name is stored as a `std::string` member (small memory overhead)
- All logging methods delegate to `Logger::internal_log_with_location()`
- Component filtering is checked before message formatting for efficiency
- Thread-safe like the underlying Logger implementation

## Compatibility

- **Full compatibility** with existing component filtering APIs
- **Coexists** with manual component logging (can use both approaches)
- **Same configuration**: Uses the same `LoggerConfig` settings
- **Same output format**: Produces identical log message format
- **Same performance characteristics**: No additional overhead beyond storing component name

This ComponentLogger solution provides exactly what you requested: a way for logs from each component to automatically prepend the component name without having to provide it to every log call, while maintaining all the power and flexibility of the existing logging system.
