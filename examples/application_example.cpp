/*
 * @file application_example.cpp
 * @brief Comprehensive application framework demonstration
 *
 * Features demonstrated:
 * - Application lifecycle management
 * - Component-based architecture
 * - Task scheduling (one-time, delayed, recurring)
 * - Signal handling
 * - Error handling and recovery
 * - Thread management
 * - Configuration integration
 * - Health monitoring
 * - Messaging between components
 * - CLI integration
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <application.h>
#include <logger.h>
#include <config.h>
#include <iostream>
#include <chrono>
#include <atomic>

using namespace base;

/**
 * @brief Example HTTP server component
 */
class HttpServerComponent : public ApplicationComponent {
public:
    bool initialize(Application& app) override {
        Logger::info("Initializing HTTP server on port {}", port_);
        app_ref_ = &app;

        // Schedule tasks to simulate server activity
        app.schedule_recurring_task([this]() {
            handle_request();
        }, std::chrono::milliseconds(200));

        return true;
    }

    bool start() override {
        Logger::info("Starting HTTP server");
        is_running_ = true;
        return true;
    }

    bool stop() override {
        Logger::info("Stopping HTTP server");
        is_running_ = false;
        return true;
    }

    std::string_view name() const override { return "HttpServer"; }

    bool health_check() const override {
        bool healthy = is_running_ && request_count_.load() < 1000; // Simulate health condition
        if (!healthy) {
            Logger::warn("HTTP server health check failed");
        }
        return healthy;
    }

private:
    void handle_request() {
        request_count_++;
        if (request_count_ % 10 == 0) {
            Logger::info("HTTP server processed {} requests", request_count_.load());
        }

        // Simulate different types of requests
        if (request_count_ % 25 == 0) {
            Logger::warn("Slow request detected: {}ms", 150 + (request_count_ % 100));
        }
        if (request_count_ % 50 == 0) {
            Logger::error("Request failed: timeout");
        }

        // Auto-shutdown after demo
        if (request_count_ >= 100) {
            Logger::info("HTTP server demo complete");
            app_ref_->shutdown();
        }
    }

    Application* app_ref_ = nullptr;
    std::atomic<int> request_count_{0};
    int port_ = 8080;
    bool is_running_ = false;
};

/**
 * @brief Example database component
 */
class DatabaseComponent : public ApplicationComponent {
public:
    bool initialize(Application& app) override {
        Logger::info("Initializing database connection pool");

        // Simulate connection initialization
        app.post_delayed_task([this]() {
            connection_pool_size_ = 10;
            Logger::info("Database connection pool ready with {} connections", connection_pool_size_);
        }, std::chrono::milliseconds(500));

        return true;
    }

    bool start() override {
        Logger::info("Starting database component");
        is_connected_ = true;
        return true;
    }

    bool stop() override {
        Logger::info("Stopping database component");
        is_connected_ = false;
        return true;
    }

    std::string_view name() const override { return "Database"; }

    bool health_check() const override {
        return is_connected_ && connection_pool_size_ > 0;
    }

private:
    bool is_connected_ = false;
    int connection_pool_size_ = 0;
};

/**
 * @brief Example worker thread component
 */
class WorkerComponent : public ApplicationComponent {
public:
    bool initialize(Application& app) override {
        Logger::info("Initializing worker component");

        // Create worker threads
        for (int i = 0; i < 3; ++i) {
            auto worker = app.create_worker_thread(fmt::format("Worker-{}", i + 1));
            workers_.push_back(worker);

            // Schedule work on each thread
            worker->post_task([i]() {
                Logger::debug("Worker-{} started processing", i + 1);
            });
        }

        return true;
    }

    bool start() override {
        Logger::info("Starting worker component with {} threads", workers_.size());

        // Distribute work to workers
        for (size_t i = 0; i < workers_.size(); ++i) {
            schedule_worker_tasks(i);
        }

        return true;
    }

    bool stop() override {
        Logger::info("Stopping worker component");
        workers_.clear();
        return true;
    }

    std::string_view name() const override { return "Worker"; }

private:
    void schedule_worker_tasks(size_t worker_id) {
        if (worker_id < workers_.size()) {
            auto& worker = workers_[worker_id];

            // Schedule periodic work
            for (int task = 0; task < 5; ++task) {
                worker->post_task([worker_id, task]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100 + (task * 50)));
                    Logger::debug("Worker-{} completed task {}", worker_id + 1, task + 1);
                });
            }
        }
    }

    std::vector<std::shared_ptr<Application::ManagedThread>> workers_;
};

/**
 * @brief Comprehensive application example
 */
class ExampleApplication : public Application {
public:
    ExampleApplication() : Application(create_config()) {}

protected:
    bool on_initialize() override {
        Logger::info("=== Application Framework Comprehensive Demo ===");
        Logger::info("Demonstrating: lifecycle, components, tasks, threading, messaging");

        // Add components
        add_component(std::make_unique<HttpServerComponent>());
        add_component(std::make_unique<DatabaseComponent>());
        add_component(std::make_unique<WorkerComponent>());

        // Demonstrate various task scheduling
        demonstrate_task_scheduling();

        // Set up messaging demonstration
        demonstrate_messaging();

        return true;
    }

    bool on_start() override {
        Logger::info("Application started successfully");
        Logger::info("Components: HttpServer, Database, Worker threads");
        Logger::info("Monitoring health, processing tasks...");

        // Schedule periodic status reports
        schedule_recurring_task([this]() {
            log_application_status();
        }, std::chrono::seconds(5));

        return true;
    }

    bool on_stop() override {
        Logger::info("Application stopping gracefully");
        return true;
    }

    void on_cleanup() override {
        Logger::info("Application cleanup completed");
    }

    void on_signal(int signal) override {
        switch (signal) {
            case SIGINT:
                Logger::info("Received SIGINT - initiating graceful shutdown");
                shutdown();
                break;
            case SIGTERM:
                Logger::info("Received SIGTERM - initiating graceful shutdown");
                shutdown();
                break;
            case SIGHUP:
                Logger::info("Received SIGHUP - reloading configuration");
                reload_config();
                break;
            case SIGUSR1:
                Logger::info("Received SIGUSR1 - dumping application status");
                dump_detailed_status();
                break;
            case SIGUSR2:
                Logger::info("Received SIGUSR2 - toggling debug mode");
                toggle_debug_mode();
                break;
            default:
                Logger::debug("Received signal: {}", signal);
                break;
        }
    }

    void on_error(const std::exception& error) override {
        Logger::error("Application error: {}", error.what());
        // Could implement recovery logic here
    }

private:
    static ApplicationConfig create_config() {
        return ApplicationConfig{
            .name = "ExampleApp",
            .version = "1.0.0",
            .description = "Comprehensive application framework demonstration",
            .worker_threads = 4,
            .enable_health_check = true,
            .health_check_interval = std::chrono::milliseconds(3000)
        };
    }

    void demonstrate_task_scheduling() {
        Logger::info("Demonstrating task scheduling...");

        // Immediate task
        post_task([]() {
            Logger::info("Immediate task executed");
        });

        // High priority task
        post_task([]() {
            Logger::info("High priority task executed");
        }, TaskPriority::High);

        // Delayed task
        post_delayed_task([]() {
            Logger::info("Delayed task executed (after 2 seconds)");
        }, std::chrono::milliseconds(2000));

        // Recurring task (will run a few times)
        auto recurring_id = schedule_recurring_task([]() {
            static int count = 0;
            ++count;
            Logger::info("Recurring task #{} executed", count);
            if (count >= 3) {
                Logger::info("Recurring task completing after {} executions", count);
            }
        }, std::chrono::milliseconds(1500));

        // Cancel the recurring task after some executions
        post_delayed_task([this, recurring_id]() {
            cancel_recurring_task(recurring_id);
            Logger::info("Cancelled recurring task");
        }, std::chrono::milliseconds(6000));
    }

    void demonstrate_messaging() {
        Logger::info("Setting up inter-thread messaging...");

        // Create event-driven thread for message processing
        message_processor_ = create_event_driven_thread("MessageProcessor");

        // Subscribe to different message types
        message_processor_->subscribe_to_messages<std::string>([](const Message<std::string>& msg) {
            Logger::info("Message processor received string: '{}'", msg.data());
        });

        message_processor_->subscribe_to_messages<int>([](const Message<int>& msg) {
            Logger::info("Message processor received int: {}", msg.data());
        });

        // Send messages from main thread
        post_delayed_task([this]() {
            send_message_to_thread("MessageProcessor", std::string("Hello from main thread!"));
            send_message_to_thread("MessageProcessor", 42);
            broadcast_message(std::string("Broadcast message to all threads"));
        }, std::chrono::milliseconds(1000));
    }

    void log_application_status() {
        static int status_count = 0;
        ++status_count;

        Logger::info("=== Application Status #{} ===", status_count);
        Logger::info("State: {}", static_cast<int>(state()));
        Logger::info("Managed threads: {}", managed_thread_count());

        // Check component health
        auto* http_server = get_component("HttpServer");
        auto* database = get_component("Database");
        auto* worker = get_component("Worker");

        Logger::info("Component health - HttpServer: {}, Database: {}, Worker: {}",
                    http_server ? http_server->health_check() : false,
                    database ? database->health_check() : false,
                    worker ? worker->health_check() : false);
    }

    void dump_detailed_status() {
        Logger::info("=== Detailed Application Status ===");
        Logger::info("Application: {} v{}", config().name, config().version);
        Logger::info("Worker threads: {}", config().worker_threads);
        Logger::info("Health check enabled: {}", config().enable_health_check);
        Logger::info("Managed threads: {}", managed_thread_count());

        // Component details
        if (auto* comp = get_component("HttpServer")) {
            Logger::info("HttpServer component: active, health: {}", comp->health_check());
        }
        if (auto* comp = get_component("Database")) {
            Logger::info("Database component: active, health: {}", comp->health_check());
        }
        if (auto* comp = get_component("Worker")) {
            Logger::info("Worker component: active, health: {}", comp->health_check());
        }
    }

    void toggle_debug_mode() {
        static bool debug_enabled = false;
        debug_enabled = !debug_enabled;

        if (debug_enabled) {
            Logger::set_level(LogLevel::Debug);
            Logger::info("Debug mode enabled");
        } else {
            Logger::set_level(LogLevel::Info);
            Logger::info("Debug mode disabled");
        }
    }

    // Store message processor thread to keep it alive
    std::shared_ptr<Application::EventDrivenManagedThread> message_processor_;
};

int main(int argc, char* argv[]) {
    try {
        // Initialize logger first
        Logger::init();

        ExampleApplication app;

        Logger::info("Starting comprehensive application framework demo");
        Logger::info("Use signals to interact:");
        Logger::info("  SIGINT/SIGTERM - Graceful shutdown");
        Logger::info("  SIGHUP - Reload configuration");
        Logger::info("  SIGUSR1 - Dump status");
        Logger::info("  SIGUSR2 - Toggle debug mode");

        std::cout << "Starting application..." << std::endl;
        int result = app.run(argc, argv);
        std::cout << "Application finished with result: " << result << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return EXIT_FAILURE;
    }
}
