/*
 * @file app_example.cpp
 * @brief Example application using the Base application framework
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include <iostream>
#include <chrono>

using namespace base;

/**
 * @brief Example HTTP server component
 */
class HttpServerComponent : public ApplicationComponent {
public:
    bool initialize(Application& app) override {
        Logger::info("Initializing HTTP server on port {}", port_);
        app_ref_ = &app;

        // Schedule a task to simulate server activity
        app.schedule_recurring_task([this]() {
            request_count_++;
            if (request_count_.load() % 10 == 0) {
                Logger::info("Processed {} requests", request_count_.load());
            }

            // Auto-shutdown after 50 requests for demo purposes
            if (request_count_.load() >= 50) {
                Logger::info("Demo complete, shutting down...");
                app_ref_->shutdown();
            }
        }, std::chrono::milliseconds(100));  // Faster for demo

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

    std::string_view name() const override {
        return "HttpServer";
    }

    bool health_check() const override {
        return is_running_;
    }

private:
    int port_ = 8080;
    std::atomic<bool> is_running_{false};
    std::atomic<int> request_count_{0};
    Application* app_ref_ = nullptr;
};

/**
 * @brief Example database connection component
 */
class DatabaseComponent : public ApplicationComponent {
public:
    bool initialize(Application& app) override {
        Logger::info("Initializing database connection to {}", connection_string_);

        // Simulate connecting to database
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        is_connected_ = true;

        return true;
    }

    bool start() override {
        Logger::info("Starting database component");
        return is_connected_;
    }

    bool stop() override {
        Logger::info("Stopping database component");
        is_connected_ = false;
        return true;
    }

    std::string_view name() const override {
        return "Database";
    }

    bool health_check() const override {
        return is_connected_;
    }

private:
    std::string connection_string_ = "postgresql://localhost:5432/mydb";
    std::atomic<bool> is_connected_{false};
};

/**
 * @brief Example application class
 */
class ExampleApplication : public Application {
public:
    ExampleApplication() : Application(create_config()) {
        // Add components
        add_component(std::make_unique<HttpServerComponent>());
        add_component(std::make_unique<DatabaseComponent>());

        // Set custom signal handler for SIGUSR1
        set_signal_handler(SIGUSR1, [this](int signal) {
            Logger::info("Custom SIGUSR1 handler: dumping application stats");
            dump_stats();
        });

        // Set custom error handler
        set_error_handler([this](const std::exception& e) {
            Logger::error("Custom error handler: {}", e.what());
            // Could implement custom error recovery here
        });
    }

protected:
    bool on_initialize() override {
        Logger::info("Custom application initialization");

        // Example: Initialize application-specific resources
        start_time_ = std::chrono::steady_clock::now();

        return true;
    }

    bool on_start() override {
        Logger::info("Custom application startup");

        // Create dedicated background processing thread
        background_processor_ = create_thread("background-processor", [](asio::io_context& io_ctx) {
            Logger::info("Background processor thread started");

            // Setup periodic background work
            auto timer = std::make_shared<asio::steady_timer>(io_ctx);

            struct BackgroundWorker {
                std::shared_ptr<asio::steady_timer> timer;

                void operator()() {
                    Logger::debug("Performing background processing...");

                    // Simulate some background work
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                    // Schedule next execution
                    timer->expires_after(std::chrono::seconds(2));
                    timer->async_wait([this](const asio::error_code& ec) {
                        if (!ec) {
                            (*this)();
                        }
                    });
                }
            };

            BackgroundWorker worker{timer};
            worker();
        });

        // Create a dedicated file I/O thread
        file_io_thread_ = create_worker_thread("file-io");

        // Post some file I/O work to the dedicated thread
        file_io_thread_->post_task([]() {
            Logger::info("Performing file I/O operations on dedicated thread");
            // Simulate file operations that shouldn't block main thread
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            Logger::debug("File I/O operations completed");
        });

        // Create a network processing thread
        network_thread_ = create_thread("network-handler", [](asio::io_context& io_ctx) {
            Logger::info("Network handler thread started with dedicated event loop");

            // This thread could handle network connections, HTTP requests, etc.
            // Each thread has its own event loop for maximum isolation
        });

        Logger::info("Created {} managed threads", managed_thread_count());

        // Schedule periodic stats reporting
        stats_task_id_ = schedule_recurring_task([this]() {
            dump_stats();
        }, std::chrono::seconds(30));

        // Post a welcome message
        post_delayed_task([]() {
            Logger::info("Welcome! Application is now running. Send SIGUSR1 for stats.");
        }, std::chrono::milliseconds(1000));

        return true;
    }

    bool on_stop() override {
        Logger::info("Custom application shutdown");

        // Cancel stats reporting
        if (stats_task_id_ != 0) {
            cancel_recurring_task(stats_task_id_);
        }

        return true;
    }

    void on_cleanup() override {
        Logger::info("Custom application cleanup");

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time_);
        Logger::info("Application ran for {} seconds", duration.count());
    }

    void on_signal(int signal) override {
        Logger::info("Custom signal handler for signal {}", signal);

        switch (signal) {
            case SIGUSR2:
                Logger::info("SIGUSR2 received - performing custom action");
                // Example: trigger some custom action
                break;
        }
    }

private:
    static ApplicationConfig create_config() {
        ApplicationConfig config;
        config.name = "ExampleApp";
        config.version = "1.0.0";
        config.description = "Example application demonstrating the Base framework";
        config.worker_threads = 2;
        config.enable_health_check = true;
        config.health_check_interval = std::chrono::milliseconds(5000);
        config.config_file = "example_app.toml";
        config.config_app_name = "example_app";
        return config;
    }

    void dump_stats() {
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);

        Logger::info("=== Application Statistics ===");
        Logger::info("Uptime: {} seconds", uptime.count());
        Logger::info("State: {}", static_cast<int>(state()));
        Logger::info("Components: {}", get_component_count());
        Logger::info("Managed threads: {}", managed_thread_count());
        Logger::info("Background processor running: {}",
                    background_processor_ && background_processor_->is_running());
        Logger::info("File I/O thread running: {}",
                    file_io_thread_ && file_io_thread_->is_running());
        Logger::info("Network thread running: {}",
                    network_thread_ && network_thread_->is_running());
        Logger::info("==============================");
    }

    size_t get_component_count() const {
        size_t count = 0;
        if (get_component("HttpServer")) count++;
        if (get_component("Database")) count++;
        return count;
    }

    std::chrono::steady_clock::time_point start_time_;
    std::size_t stats_task_id_ = 0;

    // Managed threads for different purposes
    std::shared_ptr<ManagedThread> background_processor_;
    std::shared_ptr<ManagedThread> file_io_thread_;
    std::shared_ptr<ManagedThread> network_thread_;
};

// Use the convenience macro to create main function
BASE_APPLICATION_MAIN(ExampleApplication)
