/*
 * @file cli_example.cpp
 * @brief Example demonstrating the Base CLI feature for runtime inspection and debugging
 *
 * This example shows how to:
 * 1. Enable CLI with both stdin and TCP interfaces
 * 2. Register custom CLI commands
 * 3. Use CLI to inspect application internals
 * 4. Interact with the application via CLI
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include "cli.h"
#include "logger.h"
#include "config.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace base;

/**
 * @brief Example application with CLI features
 */
class CLIExampleApp : public Application {
private:
    std::shared_ptr<ManagedThreadBase> worker_thread_;
    std::atomic<int> task_counter_{0};
    std::atomic<bool> worker_running_{false};

public:
    CLIExampleApp() : Application({
        .name = "CLI Example App",
        .version = "1.0.0",
        .description = "Demonstration of Base CLI features",
        .worker_threads = 2,
        .enable_health_check = true,
        .health_check_interval = std::chrono::milliseconds(2000),
        .enable_cli = true,           // Enable CLI automatically
        .cli_enable_stdin = true,     // Enable stdin interface
        .cli_enable_tcp = true,       // Enable TCP interface
        .cli_bind_address = "127.0.0.1",
        .cli_port = 8080
    }) {}

protected:
    bool on_initialize() override {
        Logger::info("Initializing CLI example application");

        // Register custom CLI commands
        register_custom_commands();

        return true;
    }

    bool on_start() override {
        Logger::info("Starting CLI example application");

        // Create a worker thread to demonstrate thread inspection
        worker_thread_ = create_thread("example_worker", [this](ManagedThreadBase& thread_base) {
            worker_running_.store(true);
            Logger::info("Worker thread started");

            // Cast to Application::ManagedThread to access io_context
            auto& thread = static_cast<Application::ManagedThread&>(thread_base);

            // Schedule periodic work to demonstrate activity
            auto timer = std::make_shared<asio::steady_timer>(thread.io_context());
            schedule_worker_task(timer);
        });

        // Schedule some recurring tasks
        schedule_recurring_task([this]() {
            task_counter_++;
            Logger::debug("Recurring task executed, counter: {}", task_counter_.load());
        }, std::chrono::milliseconds(5000));

        Logger::info("Application started. CLI available on:");
        Logger::info("  - stdin: Type commands directly");
        Logger::info("  - TCP: telnet localhost 8080");
        Logger::info("Type 'help' to see available commands");

        return true;
    }

    bool on_stop() override {
        Logger::info("Stopping CLI example application");
        worker_running_.store(false);
        return true;
    }

private:
    void register_custom_commands() {
        auto& cli = this->cli();

        // Custom command to show task counter
        cli.register_command("task-count", "Show current task counter", "task-count",
            [this](const CLIContext& context) -> CLIResult {
                std::ostringstream oss;
                oss << "Task counter: " << task_counter_.load();
                return CLIResult::ok(oss.str());
            });

        // Custom command to control worker thread
        cli.register_command("worker", "Control worker thread", "worker [start|stop|status]",
            [this](const CLIContext& context) -> CLIResult {
                if (context.args.size() < 2) {
                    std::ostringstream oss;
                    oss << "Worker status: " << (worker_running_.load() ? "Running" : "Stopped");
                    return CLIResult::ok(oss.str());
                }

                const std::string& action = context.args[1];
                if (action == "status") {
                    std::ostringstream oss;
                    oss << "Worker status: " << (worker_running_.load() ? "Running" : "Stopped");
                    return CLIResult::ok(oss.str());
                } else if (action == "start") {
                    worker_running_.store(true);
                    return CLIResult::ok("Worker started");
                } else if (action == "stop") {
                    worker_running_.store(false);
                    return CLIResult::ok("Worker stopped");
                } else {
                    return CLIResult::error("Invalid action. Use: start, stop, or status");
                }
            });

        // Custom command to simulate load
        cli.register_command("load", "Simulate load on the system", "load <tasks>",
            [this](const CLIContext& context) -> CLIResult {
                if (context.args.size() < 2) {
                    return CLIResult::error("Usage: load <number_of_tasks>");
                }

                try {
                    int num_tasks = std::stoi(context.args[1]);
                    if (num_tasks <= 0 || num_tasks > 1000) {
                        return CLIResult::error("Number of tasks must be between 1 and 1000");
                    }

                    for (int i = 0; i < num_tasks; ++i) {
                        post_task([this, i]() {
                            // Simulate some work
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            task_counter_++;
                        });
                    }

                    std::ostringstream oss;
                    oss << "Scheduled " << num_tasks << " tasks";
                    return CLIResult::ok(oss.str());

                } catch (const std::exception& e) {
                    return CLIResult::error("Invalid number: " + std::string(e.what()));
                }
            });
    }

    void schedule_worker_task(std::shared_ptr<asio::steady_timer> timer) {
        if (!worker_running_.load()) {
            return;
        }

        timer->expires_after(std::chrono::milliseconds(1000));
        timer->async_wait([this, timer](const asio::error_code& ec) {
            if (!ec && worker_running_.load()) {
                // Do some work
                task_counter_++;
                Logger::trace("Worker thread task completed, counter: {}", task_counter_.load());

                // Schedule next task
                schedule_worker_task(timer);
            }
        });
    }
};

int main() {
    try {
        // Configure logger
        Logger::set_level(LogLevel::Info);
        Logger::info("Starting CLI Example Application");

        CLIExampleApp app;

        std::cout << "\n=== Base CLI Example ===\n";
        std::cout << "The application will start with CLI enabled.\n";
        std::cout << "You can interact with it using:\n";
        std::cout << "1. Direct stdin commands (type here)\n";
        std::cout << "2. TCP connection: telnet localhost 8080\n\n";
        std::cout << "Available commands:\n";
        std::cout << "  help          - Show all commands\n";
        std::cout << "  status        - Show application status\n";
        std::cout << "  threads       - Show thread information\n";
        std::cout << "  config        - Show configuration\n";
        std::cout << "  health        - Run health check\n";
        std::cout << "  messaging     - Show messaging statistics\n";
        std::cout << "  log-level     - Change log level\n";
        std::cout << "  task-count    - Show task counter (custom)\n";
        std::cout << "  worker        - Control worker thread (custom)\n";
        std::cout << "  load <n>      - Simulate load with n tasks (custom)\n";
        std::cout << "  shutdown      - Graceful shutdown\n";
        std::cout << "  exit          - Exit CLI (app continues)\n\n";

        return app.run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
