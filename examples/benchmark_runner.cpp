/*
 * @file benchmark_runner.cpp
 * @brief Simple benchmark runner for performance testing
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include "performance_benchmark.cpp"
#include <iostream>

using namespace base;

class BenchmarkApp : public Application {
public:
    BenchmarkApp() : Application(ApplicationConfig{
        .name = "benchmark_app",
        .version = "1.0.0",
        .description = "Performance Benchmark Application",
        .worker_threads = 4,
        .message_processing_interval = std::chrono::microseconds(500),
        .enable_low_latency_mode = true
    }) {}

protected:
    bool on_start() override {
        Logger::info("Starting performance benchmarks...");

        // Schedule benchmarks to run after application is fully started
        post_delayed_task([this]() {
            run_benchmarks();
        }, std::chrono::milliseconds(100));

        return true;
    }

private:
    void run_benchmarks() {
        std::vector<PerformanceBenchmark::BenchmarkResult> results;

        Logger::info("Running cross-thread latency benchmark...");
        results.push_back(PerformanceBenchmark::benchmark_cross_thread_latency(*this, 500000));

        Logger::info("Running ping-pong latency benchmark...");
        results.push_back(PerformanceBenchmark::benchmark_ping_pong_latency(*this, 55556));

        PerformanceBenchmark::print_benchmark_results(results);

        // Shutdown after benchmarks complete
        post_delayed_task([this]() {
            shutdown();
        }, std::chrono::milliseconds(100));
    }
};

int main() {
    try {
        BenchmarkApp app;
        return app.run();
    } catch (const std::exception& e) {
        Logger::critical("Benchmark application failed: {}", e.what());
        return EXIT_FAILURE;
    }
}
