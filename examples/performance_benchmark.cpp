/*
 * @file performance_benchmark.cpp
 * @brief Performance benchmarking utility for the Base framework
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include <chrono>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <algorithm>

namespace base {

class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::string name;
        std::size_t iterations;
        double avg_latency_ns;
        double min_latency_ns;
        double max_latency_ns;
        double p50_latency_ns;
        double p95_latency_ns;
        double p99_latency_ns;
        double throughput_ops_per_sec;
    };

    static BenchmarkResult benchmark_cross_thread_latency(Application& app, std::size_t iterations = 500000) {
        std::vector<double> latencies;
        latencies.reserve(iterations);

        auto worker_thread = app.create_worker_thread("benchmark_worker");
        std::atomic<std::size_t> completed{0};
        std::atomic<bool> ready{false};

        auto start_time = std::chrono::high_resolution_clock::now();        for (std::size_t i = 0; i < iterations; ++i) {
            auto task_start = std::chrono::high_resolution_clock::now();

            worker_thread->post_task([&, task_start]() {
                auto task_end = std::chrono::high_resolution_clock::now();
                auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(task_end - task_start).count();
                latencies.push_back(static_cast<double>(latency));
                completed.fetch_add(1);
            });

            // Rate limiting to prevent overwhelming the system
            if (i % 1000 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }

        // Wait for all tasks to complete
        while (completed.load() < iterations) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

        return calculate_statistics("Cross-Thread Latency", latencies, total_duration);
    }

    static BenchmarkResult benchmark_ping_pong_latency(Application& app, std::size_t iterations = 55556) {
        std::vector<double> latencies;
        latencies.reserve(iterations);

        auto thread1 = app.create_worker_thread("ping_thread");
        auto thread2 = app.create_worker_thread("pong_thread");

        std::atomic<std::size_t> completed{0};
        auto start_time = std::chrono::high_resolution_clock::now();        for (std::size_t i = 0; i < iterations; ++i) {
            auto ping_start = std::chrono::high_resolution_clock::now();

            thread1->post_task([&, ping_start, thread2]() {
                auto pong_time = std::chrono::high_resolution_clock::now();

                thread2->post_task([&, ping_start, pong_time]() {
                    auto pong_end = std::chrono::high_resolution_clock::now();
                    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(pong_end - ping_start).count();
                    latencies.push_back(static_cast<double>(latency));
                    completed.fetch_add(1);
                });
            });

            // Rate limiting
            if (i % 500 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }

        // Wait for all ping-pongs to complete
        while (completed.load() < iterations) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

        return calculate_statistics("Ping-Pong Latency", latencies, total_duration);
    }

    static void print_benchmark_results(const std::vector<BenchmarkResult>& results) {
        std::cout << "\n=== BASE FRAMEWORK PERFORMANCE BENCHMARK RESULTS ===\n\n";

        std::cout << std::left << std::setw(20) << "Benchmark"
                  << std::setw(12) << "Iterations"
                  << std::setw(12) << "Avg (ns)"
                  << std::setw(12) << "Min (ns)"
                  << std::setw(12) << "Max (ns)"
                  << std::setw(12) << "P50 (ns)"
                  << std::setw(12) << "P95 (ns)"
                  << std::setw(12) << "P99 (ns)"
                  << std::setw(15) << "Throughput (ops/s)"
                  << std::endl;

        std::cout << std::string(140, '-') << std::endl;

        for (const auto& result : results) {
            std::cout << std::left << std::setw(20) << result.name
                      << std::setw(12) << result.iterations
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.avg_latency_ns
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.min_latency_ns
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.max_latency_ns
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.p50_latency_ns
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.p95_latency_ns
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.p99_latency_ns
                      << std::setw(15) << std::fixed << std::setprecision(0) << result.throughput_ops_per_sec
                      << std::endl;
        }

        std::cout << std::endl;
    }

private:
    static BenchmarkResult calculate_statistics(const std::string& name,
                                                std::vector<double>& latencies,
                                                double total_duration_ns) {
        BenchmarkResult result;
        result.name = name;
        result.iterations = latencies.size();

        if (latencies.empty()) {
            return result;
        }

        // Sort for percentile calculations
        std::sort(latencies.begin(), latencies.end());

        // Calculate statistics
        result.avg_latency_ns = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        result.min_latency_ns = latencies.front();
        result.max_latency_ns = latencies.back();

        result.p50_latency_ns = latencies[latencies.size() * 50 / 100];
        result.p95_latency_ns = latencies[latencies.size() * 95 / 100];
        result.p99_latency_ns = latencies[latencies.size() * 99 / 100];

        result.throughput_ops_per_sec = (static_cast<double>(latencies.size()) * 1e9) / total_duration_ns;

        return result;
    }
};

} // namespace base
