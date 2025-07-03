/*
 * @file simple_task_example.cpp
 * @brief Demonstrates the simple, user-friendly task posting API
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include <iostream>

using namespace base;

class SimpleTaskApp : public Application {
public:
    SimpleTaskApp() : Application(ApplicationConfig{
        .name = "simple_task_demo",
        .version = "1.0.0",
        .description = "Simple Task Posting Demo",
        .worker_threads = 2
    }) {
        // Initialize logger for console output
        Logger::init();
    }

protected:
    bool on_start() override {
        Logger::info("Demonstrating simple task posting API");

        // Example 1: Basic task posting (safe, recommended for most use cases)
        post_task([]() {
            Logger::info("✓ Basic task executed safely with exception handling");
        });

        // Example 2: High-priority task posting (for performance-critical operations)
        post_task([]() {
            Logger::info("✓ High-priority task executed with maximum performance");
        }, TaskPriority::High);

        // Example 3: Critical task posting (ultra-fast, no exception handling)
        post_task([]() {
            Logger::info("✓ Critical task executed with zero overhead");
        }, TaskPriority::Critical);

        // Example 4: Delayed task
        post_delayed_task([]() {
            Logger::info("✓ Delayed task executed after 100ms");
        }, std::chrono::milliseconds(100));

        // Example 5: Worker thread tasks
        auto worker = create_worker_thread("demo_worker");
        worker->post_task([]() {
            Logger::info("✓ Worker thread task executed");
        });

        // Example 6: Multiple tasks with different priorities
        for (int i = 0; i < 5; ++i) {
            auto priority = (i % 2 == 0) ? TaskPriority::Normal : TaskPriority::Critical;
            post_task([i]() {
                Logger::info("✓ Task {} completed", i + 1);
            }, priority);
        }

        // Schedule shutdown
        post_delayed_task([this]() {
            Logger::info("Demo completed! Shutting down...");
            shutdown();
        }, std::chrono::milliseconds(500));

        return true;
    }
};

int main() {
    try {
        SimpleTaskApp app;
        return app.run();
    } catch (const std::exception& e) {
        Logger::critical("Application failed: {}", e.what());
        return EXIT_FAILURE;
    }
}
