# Crux Application Framework

A modern C++20 event-driven application framework built on top of standalone ASIO, providing a complete foundation for building robust, scalable applications with comprehensive infrastructure management.

## Features

### 🚀 **Core Framework Features**

- **Modern C++20 Design**: Uses latest C++ features for type safety and performance
- **Event-Driven Architecture**: Built on standalone ASIO for high-performance I/O operations
- **Thread Management API**: Managed threads with isolated event loops for specialized workloads
- **Component-Based System**: Modular architecture with pluggable components
- **Signal Handling**: Graceful shutdown and custom signal handling
- **Health Monitoring**: Built-in health checks and monitoring
- **Configuration Integration**: Seamless TOML configuration support
- **Thread Pool Management**: Configurable worker thread pools
- **Task Scheduling**: Support for immediate, delayed, and recurring tasks
- **Lifecycle Management**: Complete application lifecycle with proper cleanup

### 🛠 **Infrastructure Features**

- **Automatic Resource Management**: RAII-based resource lifecycle
- **Cross-Thread Communication**: Safe task posting between managed threads
- **Named Thread Support**: Easy thread identification for debugging and monitoring
- **Error Recovery**: Comprehensive exception handling and recovery
- **Graceful Shutdown**: Clean shutdown with configurable timeouts
- **Background Tasks**: Support for background and daemon-style applications
- **Metrics and Monitoring**: Built-in health checks and statistics
- **Hot Configuration Reload**: Runtime configuration updates via signals

## Quick Start

### Basic Application

```cpp
#include "application.h"

class MyApplication : public crux::Application {
protected:
    bool on_initialize() override {
        // Custom initialization logic
        Logger::info("Initializing MyApplication");
        return true;
    }

    bool on_start() override {
        // Start application services
        Logger::info("Starting MyApplication services");

        // Schedule a recurring task
        schedule_recurring_task([]() {
            Logger::info("Heartbeat - application is running");
        }, std::chrono::seconds(30));

        return true;
    }

    bool on_stop() override {
        // Custom shutdown logic
        Logger::info("Shutting down MyApplication");
        return true;
    }
};

// Use the convenience macro for main function
CRUX_APPLICATION_MAIN(MyApplication)
```

### Component-Based Application

```cpp
#include "application.h"

// Example HTTP Server Component
class HttpServerComponent : public crux::ApplicationComponent {
public:
    bool initialize(crux::Application& app) override {
        Logger::info("Initializing HTTP server on port {}", port_);
        // Initialize HTTP server
        return true;
    }

    bool start() override {
        Logger::info("Starting HTTP server");
        is_running_ = true;
        // Start accepting connections
        return true;
    }

    bool stop() override {
        Logger::info("Stopping HTTP server");
        is_running_ = false;
        // Stop server and close connections
        return true;
    }

    std::string_view name() const override {
        return "HttpServer";
    }

    bool health_check() const override {
        return is_running_;
    }

private:
    int port_ = 8080;
    std::atomic<bool> is_running_{false};
};

class WebApplication : public crux::Application {
public:
    WebApplication() : Application(create_config()) {
        // Add components
        add_component(std::make_unique<HttpServerComponent>());

        // Set custom signal handlers
        set_signal_handler(SIGUSR1, [this](int signal) {
            Logger::info("Received SIGUSR1 - dumping server stats");
            dump_stats();
        });
    }

private:
    static crux::ApplicationConfig create_config() {
        crux::ApplicationConfig config;
        config.name = "WebApp";
        config.version = "1.0.0";
        config.description = "Modern web application";
        config.worker_threads = 4;
        config.enable_health_check = true;
        config.config_file = "webapp.toml";
        return config;
    }

    void dump_stats() {
        Logger::info("=== Server Statistics ===");
        // Log server metrics
        Logger::info("=========================");
    }
};

CRUX_APPLICATION_MAIN(WebApplication)
```

## Configuration

### ApplicationConfig Structure

```cpp
struct ApplicationConfig {
    std::string name = "crux_app";              // Application name
    std::string version = "1.0.0";              // Application version
    std::string description = "Crux Application"; // Description

    // Threading
    std::size_t worker_threads = std::thread::hardware_concurrency();
    bool use_dedicated_io_thread = true;

    // Signal handling
    std::vector<int> handled_signals = {SIGINT, SIGTERM, SIGUSR1, SIGUSR2};

    // Timeouts
    std::chrono::milliseconds startup_timeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds shutdown_timeout = std::chrono::milliseconds(10000);

    // Health monitoring
    bool enable_health_check = true;
    std::chrono::milliseconds health_check_interval = std::chrono::milliseconds(5000);

    // Configuration file
    std::string config_file = "app.toml";
    std::string config_app_name = "default";
};
```

### TOML Configuration Integration

```toml
[myapp]

[myapp.app]
name = "MyApplication"
version = "2.0.0"
description = "Production application"
worker_threads = 8

[myapp.logging]
level = "info"
enable_console = true
enable_file = true
file_path = "logs/app.log"
enable_colors = true

[myapp.network]
host = "0.0.0.0"
port = 8080
max_connections = 1000

[myapp.features]
enable_health_endpoint = true
enable_metrics = true
enable_admin_api = true
```

## Thread Management API

The framework provides a powerful thread management system with isolated event loops for high-performance, concurrent applications.

### ManagedThread Overview

The `ManagedThread` class provides:

- **Dedicated Event Loop**: Each thread has its own `asio::io_context`
- **Task Posting**: Thread-safe task posting between threads
- **Lifecycle Management**: Automatic start/stop/join coordination
- **Named Threads**: Easy identification for debugging and monitoring

### Creating Managed Threads

#### Worker Threads

Simple threads for general task processing:

```cpp
// Create a dedicated worker thread
auto worker = app.create_worker_thread("file-processor");

// Post tasks to the worker thread
worker->post_task([]() {
    Logger::info("Processing files on dedicated thread");
    // Perform CPU-intensive or blocking operations
});

// Thread automatically handles the event loop
```

#### Custom Threads

Threads with custom initialization and setup:

```cpp
// Create thread with custom setup function
auto network_thread = app.create_thread("network-handler",
    [](asio::io_context& io_ctx) {
        Logger::info("Network thread started with dedicated event loop");

        // Setup custom async operations
        auto timer = std::make_shared<asio::steady_timer>(io_ctx);
        timer->expires_after(std::chrono::seconds(1));
        timer->async_wait([](const asio::error_code& ec) {
            if (!ec) {
                Logger::debug("Network heartbeat");
            }
        });

        // Setup network listeners, etc.
    });
```

### Thread Management Operations

```cpp
// Get thread count
size_t count = app.managed_thread_count();
Logger::info("Managing {} threads", count);

// Find thread by name
auto* worker = app.get_managed_thread("file-processor");
if (worker) {
    worker->post_task([]() {
        Logger::info("Posted task to named thread");
    });
}

// Stop all managed threads gracefully
app.stop_all_managed_threads();
app.join_all_managed_threads();
```

### Thread API Reference

```cpp
class ManagedThread {
public:
    // Get the thread's event loop
    asio::io_context& io_context();
    asio::any_io_executor executor();

    // Thread identification
    const std::string& name() const;
    bool is_running() const;

    // Task posting
    void post_task(std::function<void()> task);

    // Lifecycle control
    void stop();    // Signal thread to stop
    void join();    // Wait for thread completion
};
```

### Advanced Thread Patterns

#### Background Processing Thread

```cpp
auto background = app.create_thread("background-processor",
    [](asio::io_context& io_ctx) {
        Logger::info("Background processor started");

        // Setup periodic background work
        auto timer = std::make_shared<asio::steady_timer>(io_ctx);

        struct BackgroundWorker {
            std::shared_ptr<asio::steady_timer> timer;

            void operator()() {
                // Perform background processing
                Logger::debug("Processing background tasks...");

                // Schedule next execution
                timer->expires_after(std::chrono::seconds(5));
                timer->async_wait([this](const asio::error_code& ec) {
                    if (!ec) (*this)();
                });
            }
        };

        BackgroundWorker worker{timer};
        worker();
    });
```

#### File I/O Thread

```cpp
auto file_thread = app.create_worker_thread("file-io");

// Post file operations to dedicated thread
file_thread->post_task([]() {
    Logger::info("Performing file I/O on dedicated thread");

    // Blocking file operations won't affect main thread
    std::ifstream file("large_file.dat");
    // Process file...

    Logger::debug("File I/O completed");
});
```

#### Network Processing Thread

```cpp
auto network = app.create_thread("network", [](asio::io_context& io_ctx) {
    // Each thread has isolated event loop for network operations
    // Perfect for handling multiple connections without interference

    Logger::info("Network thread ready for connections");

    // Setup TCP acceptors, HTTP servers, etc.
    // All network I/O happens on this dedicated thread
});
```

### Thread Safety

All thread management operations are thread-safe:

```cpp
// Safe to call from any thread
app.create_worker_thread("dynamic-worker");

// Safe cross-thread task posting
auto* worker = app.get_managed_thread("worker-1");
if (worker) {
    worker->post_task([data = capture_data()]() {
        process_data(data);
    });
}
```

### Best Practices

1. **Use Worker Threads** for simple task processing
2. **Use Custom Threads** when you need specific async setup
3. **Name Threads Meaningfully** for debugging and monitoring
4. **Avoid Blocking Operations** on the main thread
5. **Post Tasks Cross-Thread** instead of sharing state
6. **Let Application Manage Lifecycle** - don't manually join threads

## Task Scheduling

The framework provides flexible task scheduling capabilities:

### Immediate Tasks

```cpp
// Post a task to run immediately
app.post_task([]() {
    Logger::info("Immediate task executed");
});

// Post with priority
app.post_task([]() {
    Logger::critical("High priority task");
}, crux::TaskPriority::Critical);
```

### Delayed Tasks

```cpp
// Run task after delay
app.post_delayed_task([]() {
    Logger::info("Delayed task executed");
}, std::chrono::seconds(5));
```

### Recurring Tasks

```cpp
// Schedule recurring task
auto task_id = app.schedule_recurring_task([]() {
    Logger::info("Recurring task executed");
}, std::chrono::minutes(1));

// Cancel recurring task
app.cancel_recurring_task(task_id);
```

## Component System

Components provide modular functionality that can be easily added and managed:

### Creating Components

```cpp
class DatabaseComponent : public crux::ApplicationComponent {
public:
    bool initialize(crux::Application& app) override {
        Logger::info("Connecting to database: {}", connection_string_);
        // Initialize database connection
        return connect_to_database();
    }

    bool start() override {
        Logger::info("Starting database component");
        return is_connected_;
    }

    bool stop() override {
        Logger::info("Stopping database component");
        disconnect_from_database();
        return true;
    }

    std::string_view name() const override {
        return "Database";
    }

    bool health_check() const override {
        return is_connected_ && ping_database();
    }

private:
    std::string connection_string_ = "postgresql://localhost/mydb";
    std::atomic<bool> is_connected_{false};

    bool connect_to_database() { /* implementation */ return true; }
    void disconnect_from_database() { /* implementation */ }
    bool ping_database() const { /* implementation */ return true; }
};
```

### Using Components

```cpp
class MyApplication : public crux::Application {
public:
    MyApplication() {
        // Add components in dependency order
        add_component(std::make_unique<DatabaseComponent>());
        add_component(std::make_unique<HttpServerComponent>());
        add_component(std::make_unique<MetricsComponent>());
    }

protected:
    bool on_start() override {
        // Access components
        auto* db = get_component("Database");
        auto* server = get_component("HttpServer");

        if (db && server) {
            Logger::info("All components initialized successfully");
        }

        return true;
    }
};
```

## Signal Handling

The framework provides comprehensive signal handling:

### Default Signal Handling

- **SIGINT/SIGTERM**: Graceful shutdown
- **SIGUSR1**: Health check and status dump
- **SIGUSR2**: Configuration reload

### Custom Signal Handlers

```cpp
class MyApplication : public crux::Application {
public:
    MyApplication() {
        // Set custom handler for specific signal
        set_signal_handler(SIGHUP, [this](int signal) {
            Logger::info("SIGHUP received - rotating logs");
            rotate_logs();
        });

        // Override virtual signal handler
        set_signal_handler(SIGUSR1, [this](int signal) {
            Logger::info("Custom SIGUSR1 handler");
            dump_detailed_stats();
        });
    }

protected:
    void on_signal(int signal) override {
        Logger::info("Application-specific signal handling for: {}", signal);

        switch (signal) {
            case SIGPIPE:
                Logger::warn("Broken pipe detected");
                handle_broken_pipe();
                break;
        }
    }

private:
    void rotate_logs() { /* implementation */ }
    void dump_detailed_stats() { /* implementation */ }
    void handle_broken_pipe() { /* implementation */ }
};
```

## Error Handling and Recovery

### Global Error Handler

```cpp
class RobustApplication : public crux::Application {
public:
    RobustApplication() {
        set_error_handler([this](const std::exception& e) {
            Logger::error("Global error handler: {}", e.what());

            // Implement recovery logic
            attempt_recovery(e);
        });
    }

protected:
    void on_error(const std::exception& error) override {
        Logger::error("Application error: {}", error.what());

        // Application-specific error handling
        if (should_restart_on_error(error)) {
            schedule_restart();
        }
    }

private:
    void attempt_recovery(const std::exception& e) { /* implementation */ }
    bool should_restart_on_error(const std::exception& e) { return false; }
    void schedule_restart() { /* implementation */ }
};
```

## Lifecycle Management

### Application States

- **Created**: Initial state after construction
- **Initialized**: After successful initialization
- **Starting**: During startup process
- **Running**: Normal operation
- **Stopping**: During shutdown process
- **Stopped**: After clean shutdown
- **Failed**: Error state

### Lifecycle Hooks

```cpp
class LifecycleApplication : public crux::Application {
protected:
    bool on_initialize() override {
        Logger::info("Custom initialization");

        // Initialize application resources
        if (!initialize_resources()) {
            Logger::error("Failed to initialize resources");
            return false;
        }

        return true;
    }

    bool on_start() override {
        Logger::info("Custom startup");

        // Start application services
        return start_services();
    }

    bool on_stop() override {
        Logger::info("Custom shutdown");

        // Stop services gracefully
        stop_services();
        return true;
    }

    void on_cleanup() override {
        Logger::info("Custom cleanup");

        // Final cleanup
        cleanup_resources();
    }

private:
    bool initialize_resources() { return true; }
    bool start_services() { return true; }
    void stop_services() { }
    void cleanup_resources() { }
};
```

## Health Monitoring

### Built-in Health Checks

```cpp
// Health monitoring is enabled by default
ApplicationConfig config;
config.enable_health_check = true;
config.health_check_interval = std::chrono::seconds(30);
```

### Component Health Checks

```cpp
class MonitoredComponent : public crux::ApplicationComponent {
public:
    bool health_check() const override {
        // Custom health check logic
        return is_operational() &&
               resource_usage_ok() &&
               external_dependencies_ok();
    }

private:
    bool is_operational() const { return operational_; }
    bool resource_usage_ok() const {
        return memory_usage_ < max_memory_threshold_;
    }
    bool external_dependencies_ok() const {
        return database_healthy_ && cache_healthy_;
    }

    std::atomic<bool> operational_{false};
    std::atomic<size_t> memory_usage_{0};
    std::atomic<bool> database_healthy_{false};
    std::atomic<bool> cache_healthy_{false};
    size_t max_memory_threshold_ = 1024 * 1024 * 1024; // 1GB
};
```

## Best Practices

### 1. Proper Resource Management

```cpp
class ResourceManagedApp : public crux::Application {
protected:
    bool on_initialize() override {
        // Initialize resources that need cleanup
        database_pool_ = std::make_unique<DatabasePool>();
        thread_pool_ = std::make_unique<ThreadPool>();

        return database_pool_->initialize() &&
               thread_pool_->initialize();
    }

    void on_cleanup() override {
        // Cleanup in reverse order
        thread_pool_.reset();
        database_pool_.reset();
    }

private:
    std::unique_ptr<DatabasePool> database_pool_;
    std::unique_ptr<ThreadPool> thread_pool_;
};
```

### 2. Component Dependencies

```cpp
class DependencyAwareApp : public crux::Application {
public:
    DependencyAwareApp() {
        // Add components in dependency order
        add_component(std::make_unique<ConfigComponent>());    // First
        add_component(std::make_unique<DatabaseComponent>());  // Depends on config
        add_component(std::make_unique<CacheComponent>());     // Depends on config
        add_component(std::make_unique<HttpComponent>());      // Depends on DB & cache
        add_component(std::make_unique<MetricsComponent>());   // Last
    }
};
```

### 3. Graceful Shutdown

```cpp
class GracefulApp : public crux::Application {
protected:
    bool on_stop() override {
        Logger::info("Initiating graceful shutdown");

        // Stop accepting new requests
        stop_accepting_requests();

        // Wait for current requests to complete
        wait_for_requests_completion();

        // Save state
        save_application_state();

        return true;
    }

private:
    void stop_accepting_requests() { /* implementation */ }
    void wait_for_requests_completion() { /* implementation */ }
    void save_application_state() { /* implementation */ }
};
```

### 4. Configuration-Driven Behavior

```cpp
class ConfigurableApp : public crux::Application {
protected:
    bool on_start() override {
        auto& config = ConfigManager::instance();

        // Configure based on settings
        auto worker_count = config.get_value_or<int>("app.workers", 4, config_.config_app_name);
        auto enable_metrics = config.get_value_or<bool>("features.metrics", true, config_.config_app_name);

        if (enable_metrics) {
            start_metrics_collection();
        }

        scale_workers(worker_count);

        return true;
    }

private:
    void start_metrics_collection() { /* implementation */ }
    void scale_workers(int count) { /* implementation */ }
};
```

## Integration Examples

The application framework integrates seamlessly with the crux logging and configuration systems, and provides powerful thread management:

```cpp
class IntegratedApp : public crux::Application {
public:
    IntegratedApp() : Application(create_config()) {
        setup_logging();
        setup_components();
        setup_threads();
    }

private:
    static ApplicationConfig create_config() {
        ApplicationConfig config;
        config.name = "IntegratedApp";
        config.config_file = "app.toml";
        config.config_app_name = "integrated_app";
        config.worker_threads = 4;
        return config;
    }

    void setup_logging() {
        // Logging is automatically configured from TOML config
        Logger::info("Application framework ready with thread management");
    }

    void setup_components() {
        add_component(std::make_unique<HttpServerComponent>());
        add_component(std::make_unique<DatabaseComponent>());
    }

    void setup_threads() {
        // Create specialized threads for different workloads
        file_processor_ = create_worker_thread("file-processor");
        background_worker_ = create_thread("background", [this](asio::io_context& io_ctx) {
            setup_background_tasks(io_ctx);
        });

        Logger::info("Created {} managed threads", managed_thread_count());
    }

    void setup_background_tasks(asio::io_context& io_ctx) {
        // Setup periodic background processing
        auto timer = std::make_shared<asio::steady_timer>(io_ctx);
        timer->expires_after(std::chrono::seconds(10));
        timer->async_wait([this, timer](const asio::error_code& ec) {
            if (!ec) {
                Logger::debug("Background maintenance task executing");
                // Perform maintenance...
            }
        });
    }

    std::shared_ptr<ManagedThread> file_processor_;
    std::shared_ptr<ManagedThread> background_worker_;
};
```

## Building and Running

### CMake Integration

```cmake
find_package(Boost REQUIRED COMPONENTS system)

add_executable(myapp src/main.cpp)
target_link_libraries(myapp crux Boost::system)
```

### Dependencies

The framework uses modern, lightweight dependencies:

- **Standalone ASIO**: High-performance async I/O (header-only, no Boost required)
- **spdlog**: Fast structured logging
- **toml++**: Modern TOML configuration parsing
- **Google Test**: Comprehensive unit testing (dev dependency)

The framework is designed to minimize dependencies while providing maximum functionality.

### Example Build

```bash
# Install dependencies
conan install . --build=missing -s build_type=Release

# Configure and build
cmake --preset=default-release
cmake --build build/Release

# Run application
./build/Release/myapp
```

### Signal Testing

```bash
# Send signals to running application
kill -USR1 $(pidof myapp)  # Trigger health check
kill -USR2 $(pidof myapp)  # Reload configuration
kill -TERM $(pidof myapp)  # Graceful shutdown
```

## Performance Considerations

### Threading Model

- **Main Thread**: Application lifecycle and signal handling
- **Dedicated I/O Thread**: All async I/O operations (configurable)
- **Worker Pool**: CPU-intensive tasks distributed across worker threads
- **Managed Threads**: Custom threads with isolated event loops
- **Component Isolation**: Components run independently without blocking each other

### Thread Management Benefits

- **Isolation**: Each managed thread has its own event loop
- **Scalability**: Create threads on-demand for specific workloads
- **Performance**: No contention between different types of operations
- **Debugging**: Named threads for easy identification and monitoring
- **Resource Control**: Automatic lifecycle management and cleanup

### Memory Management

- **RAII**: Automatic resource cleanup
- **Smart Pointers**: Safe memory management with shared_ptr for thread handles
- **Component Lifecycle**: Controlled initialization and cleanup order
- **Thread-Safe Cleanup**: Automatic thread stopping and joining on shutdown

### Scalability

- **Async I/O**: Non-blocking operations using standalone ASIO
- **Thread Pool**: Configurable worker thread count
- **Managed Threads**: Dynamic thread creation for specialized workloads
- **Component Architecture**: Easy to add/remove functionality
- **Cross-Thread Communication**: Safe task posting between threads

---

The Crux Application Framework provides a solid foundation for building modern, production-ready C++ applications with comprehensive infrastructure management, robust error handling, and excellent observability.

For more information:

- [Logger Documentation](LOGGER_README.md)
- [Configuration Documentation](CONFIG_README.md)
