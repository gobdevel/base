/*
 * @file priority_demo.cpp
 * @brief Demonstrates the corrected task priority system
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include <iostream>
#include <chrono>

using namespace base;

class PriorityDemoApp : public Application {
public:
    PriorityDemoApp() : Application(ApplicationConfig{
        .name = "priority_demo",
        .version = "1.0.0",
        .description = "Task Priority System Demo"
    }) {
        // Initialize logger for console output
        Logger::init();
    }

protected:
    bool on_start() override {
        Logger::info("🎯 Task Priority System Demo");
        Logger::info("Demonstrating the difference between priorities and exception handling");

        // Demo 1: All priorities have exception handling
        Logger::info("\n=== Demo 1: Exception Safety Across All Priorities ===");

        // Low priority task that throws (safely handled)
        post_task([]() {
            Logger::info("📋 Low priority task executing...");
            throw std::runtime_error("Simulated error in low priority task");
        }, TaskPriority::Low);

        // Normal priority task that throws (safely handled)
        post_task([]() {
            Logger::info("📝 Normal priority task executing...");
            throw std::runtime_error("Simulated error in normal priority task");
        }, TaskPriority::Normal);

        // High priority task that throws (safely handled)
        post_task([]() {
            Logger::info("⚡ High priority task executing...");
            throw std::runtime_error("Simulated error in high priority task");
        }, TaskPriority::High);

        // Critical priority task that throws (safely handled)
        post_task([]() {
            Logger::info("🚨 Critical priority task executing...");
            throw std::runtime_error("Simulated error in critical priority task");
        }, TaskPriority::Critical);

        // Demo 2: Execution behavior differences
        Logger::info("\n=== Demo 2: Execution Behavior (post vs dispatch) ===");

        post_task([this]() {
            Logger::info("📍 This is posted from the main application thread");

            // From within event loop - Critical uses dispatch (immediate)
            Logger::info("🔥 Posting critical task from event loop (should execute immediately):");
            this->post_task([]() {
                Logger::info("   ⚡ Critical task executed via dispatch (immediate)");
            }, TaskPriority::Critical);

            Logger::info("🔄 Posting high priority task from event loop (queued via post):");
            this->post_task([]() {
                Logger::info("   📋 High priority task executed via post (queued)");
            }, TaskPriority::High);

            Logger::info("📝 After posting tasks (critical already executed, high is queued)");

        }, TaskPriority::Normal);

        // Demo 3: Exception Safety
        Logger::info("\n=== Demo 3: Exception Safety ===");

        // All tasks are always exception-safe
        post_task([]() {
            Logger::info("✅ All tasks are exception-safe by default");
        }, TaskPriority::Normal);

        // Demo 4: Performance characteristics
        Logger::info("\n=== Demo 4: Performance Characteristics ===");

        auto start_time = std::chrono::high_resolution_clock::now();

        // Critical: dispatch (immediate when called from event loop)
        post_task([start_time]() {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
            Logger::info("🚨 Critical priority latency: {}μs (dispatch)", elapsed.count());
        }, TaskPriority::Critical);

        // High: post (queued)
        post_task([start_time]() {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
            Logger::info("⚡ High priority latency: {}μs (post)", elapsed.count());
        }, TaskPriority::High);

        // Normal: post (queued)
        post_task([start_time]() {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
            Logger::info("📝 Normal priority latency: {}μs (post)", elapsed.count());
        }, TaskPriority::Normal);

        // Schedule shutdown
        post_delayed_task([this]() {
            Logger::info("\n✨ Priority demo completed!");
            Logger::info("Key takeaways:");
            Logger::info("  • All priorities provide exception safety");
            Logger::info("  • Critical uses dispatch for minimum latency");
            Logger::info("  • High/Normal/Low use post for fair scheduling");
            Logger::info("  • All tasks are always exception-safe with post_task()");
            shutdown();
        }, std::chrono::milliseconds(500));

        return true;
    }
};

int main() {
    try {
        PriorityDemoApp app;
        return app.run();
    } catch (const std::exception& e) {
        Logger::critical("Application failed: {}", e.what());
        return EXIT_FAILURE;
    }
}
