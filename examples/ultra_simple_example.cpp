/*
 * @file ultra_simple_example.cpp
 * @brief Demonstrates the ultra-simple task posting API
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include <iostream>

using namespace base;

class UltraSimpleApp : public Application {
public:
    UltraSimpleApp() : Application(ApplicationConfig{
        .name = "ultra_simple_demo",
        .version = "1.0.0",
        .description = "Ultra Simple API Demo"
    }) {
        // Initialize logger for console output
        Logger::init();
    }

protected:
    bool on_start() override {
        Logger::info("🚀 Ultra-simple task posting API demo");

        // Example 1: Simplest possible usage (uses default Normal priority)
        post_task([]() {
            Logger::info("✅ Task 1: Default priority (safe & fast)");
        });

        // Example 2: Still simple, but explicit priority
        post_task([]() {
            Logger::info("✅ Task 2: High priority (safe & fast)");
        }, TaskPriority::High);

        // Example 3: Performance-critical tasks
        post_task([]() {
            Logger::info("⚡ Task 3: Critical priority (ultra-fast)");
        }, TaskPriority::Critical);

        // Example 4: Multiple simple tasks
        for (int i = 1; i <= 5; ++i) {
            post_task([i]() {
                Logger::info("📋 Simple task #{}", i);
            });
        }

        // Example 5: Worker thread is just as simple
        auto worker = create_worker_thread("simple_worker");
        worker->post_task([]() {
            Logger::info("🧵 Worker thread task (always safe)");
        });

        // Schedule shutdown
        post_delayed_task([this]() {
            Logger::info("✨ Demo completed! API is now ultra-simple!");
            shutdown();
        }, std::chrono::milliseconds(200));

        return true;
    }
};

int main() {
    try {
        UltraSimpleApp app;
        return app.run();
    } catch (const std::exception& e) {
        Logger::critical("Application failed: {}", e.what());
        return EXIT_FAILURE;
    }
}
