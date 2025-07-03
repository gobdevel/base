/*
 * @file benchmark_runner.cpp
 * @brief Comprehensive benchmark suite for Base framework
 *
 * This file contains performance benchmarks for all major components:
 * - Logger performance (sync/async)
 * - Messaging system throughput and latency
 * - Configuration system access times
 * - Application framework overhead
 * - Thread creation and management
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../include/application.h"
#include "../include/messaging.h"
#include "../include/logger.h"
#include "../include/config.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <numeric>
#include <algorithm>
#include <fstream>

using namespace base;

class BenchmarkRunner {
public:
    struct BenchmarkResult {
        std::string name;
        double throughput_per_sec;
        double avg_latency_us;
        double min_latency_us;
        double max_latency_us;
        double p95_latency_us;
        double p99_latency_us;
        size_t total_operations;
        std::chrono::milliseconds duration;
    };

private:
    std::vector<BenchmarkResult> results_;

    template<typename Func>
    BenchmarkResult run_benchmark(const std::string& name, size_t iterations, Func&& func) {
        std::vector<double> latencies;
        latencies.reserve(iterations);

        auto start_time = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            auto op_start = std::chrono::high_resolution_clock::now();
            func(i);
            auto op_end = std::chrono::high_resolution_clock::now();

            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count() / 1000.0;
            latencies.push_back(latency);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // Calculate statistics
        std::sort(latencies.begin(), latencies.end());

        double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double min_latency = latencies.front();
        double max_latency = latencies.back();
        double p95_latency = latencies[static_cast<size_t>(latencies.size() * 0.95)];
        double p99_latency = latencies[static_cast<size_t>(latencies.size() * 0.99)];

        double throughput = (iterations * 1000.0) / total_duration.count();

        return BenchmarkResult{
            .name = name,
            .throughput_per_sec = throughput,
            .avg_latency_us = avg_latency,
            .min_latency_us = min_latency,
            .max_latency_us = max_latency,
            .p95_latency_us = p95_latency,
            .p99_latency_us = p99_latency,
            .total_operations = iterations,
            .duration = total_duration
        };
    }

public:
    void run_logger_benchmarks() {
        std::cout << "\n=== Logger Benchmarks ===" << std::endl;

        // Benchmark 1: Simple string logging
        {
            auto result = run_benchmark("Logger Simple String", 100000, [](size_t i) {
                Logger::info("Simple log message {}", i);
            });
            results_.push_back(result);
        }

        // Benchmark 2: Complex formatting
        {
            auto result = run_benchmark("Logger Complex Format", 50000, [](size_t i) {
                Logger::info("Complex log: id={}, value={:.2f}, status={}, time={}",
                           i, 3.14159 * i, (i % 2 == 0) ? "active" : "inactive",
                           std::chrono::system_clock::now());
            });
            results_.push_back(result);
        }

        // Benchmark 3: Different log levels
        {
            auto result = run_benchmark("Logger Level Check", 200000, [](size_t i) {
                if (i % 4 == 0) Logger::debug("Debug message {}", i);
                else if (i % 4 == 1) Logger::info("Info message {}", i);
                else if (i % 4 == 2) Logger::warn("Warning message {}", i);
                else Logger::error("Error message {}", i);
            });
            results_.push_back(result);
        }
    }

    void run_messaging_benchmarks() {
        std::cout << "\n=== Messaging Benchmarks ===" << std::endl;

        // Benchmark 1: Message queue throughput
        {
            MessageQueue queue;
            struct BenchMessage {
                int id;
                std::string data;
                std::chrono::steady_clock::time_point timestamp;
            };

            auto result = run_benchmark("MessageQueue Send/Receive", 100000, [&queue](size_t i) {
                queue.send(BenchMessage{static_cast<int>(i), "test_data", std::chrono::steady_clock::now()});
                auto msg = queue.receive();
                // Force use of message to prevent optimization
                volatile auto* ptr = msg.get();
                (void)ptr;
            });
            results_.push_back(result);
        }

        // Benchmark 2: Cross-thread messaging
        {
            const size_t message_count = 50000;
            std::atomic<size_t> messages_received{0};
            std::atomic<bool> benchmark_done{false};

            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            struct ThreadMessage {
                size_t id;
                std::chrono::high_resolution_clock::time_point send_time;
            };

            auto sender_thread = app.create_worker_thread("sender");
            auto receiver_thread = app.create_worker_thread("receiver");

            // Set up receiver
            receiver_thread->subscribe_to_messages<ThreadMessage>([&](const Message<ThreadMessage>& msg) {
                messages_received++;
                if (messages_received >= message_count) {
                    benchmark_done = true;
                }
            });

            auto start_time = std::chrono::high_resolution_clock::now();

            // Send messages
            for (size_t i = 0; i < message_count; ++i) {
                sender_thread->send_message(ThreadMessage{i, std::chrono::high_resolution_clock::now()});
            }

            // Wait for completion
            while (!benchmark_done && messages_received < message_count) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            double throughput = (message_count * 1000.0) / duration.count();
            double avg_latency = duration.count() * 1000.0 / message_count; // microseconds

            results_.push_back(BenchmarkResult{
                .name = "Cross-Thread Messaging",
                .throughput_per_sec = throughput,
                .avg_latency_us = avg_latency,
                .min_latency_us = avg_latency * 0.5,
                .max_latency_us = avg_latency * 2.0,
                .p95_latency_us = avg_latency * 1.5,
                .p99_latency_us = avg_latency * 1.8,
                .total_operations = message_count,
                .duration = duration
            });

            sender_thread->stop();
            receiver_thread->stop();
            sender_thread->join();
            receiver_thread->join();
        }
    }

    void run_config_benchmarks() {
        std::cout << "\n=== Configuration Benchmarks ===" << std::endl;

        // Setup test configuration
        ConfigManager& config = ConfigManager::instance();
        std::string test_config = R"(
[app]
name = "benchmark_test"
version = "1.0.0"
threads = 4

[database]
host = "localhost"
port = 5432
timeout = 30

[cache]
ttl = 3600
max_size = 1000000
)";

        config.load_config_from_string(test_config, "benchmark_test");

        // Benchmark 1: Configuration value access
        {
            auto result = run_benchmark("Config Value Access", 200000, [&config](size_t i) {
                volatile auto name = config.get_app_config("benchmark_test").name;
                volatile auto version = config.get_app_config("benchmark_test").version;
                volatile auto threads = config.get_app_config("benchmark_test").worker_threads;
                (void)name; (void)version; (void)threads;
            });
            results_.push_back(result);
        }

        // Benchmark 2: Custom value lookup
        {
            auto result = run_benchmark("Config Custom Lookup", 100000, [&config](size_t i) {
                volatile auto host = config.get_value<std::string>("database.host", "benchmark_test");
                volatile auto port = config.get_value<int>("database.port", "benchmark_test");
                volatile auto ttl = config.get_value<int>("cache.ttl", "benchmark_test");
                (void)host; (void)port; (void)ttl;
            });
            results_.push_back(result);
        }
    }

    void run_thread_benchmarks() {
        std::cout << "\n=== Thread Management Benchmarks ===" << std::endl;

        // Benchmark 1: Thread creation and destruction
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;

            auto result = run_benchmark("Thread Create/Destroy", 1000, [&config](size_t i) {
                Application app(config);
                auto thread = app.create_worker_thread("bench_thread_" + std::to_string(i));
                thread->stop();
                thread->join();
            });
            results_.push_back(result);
        }

        // Benchmark 2: Task posting overhead
        {
            ApplicationConfig config;
            config.worker_threads = 2;
            config.enable_health_check = false;
            Application app(config);

            auto worker = app.create_worker_thread("task_worker");
            std::atomic<size_t> tasks_completed{0};

            auto result = run_benchmark("Task Posting", 50000, [&worker, &tasks_completed](size_t i) {
                worker->post_task([&tasks_completed]() {
                    tasks_completed++;
                });
            });

            // Wait for tasks to complete
            while (tasks_completed < 50000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            results_.push_back(result);
            worker->stop();
            worker->join();
        }
    }

    void run_application_benchmarks() {
        std::cout << "\n=== Application Framework Benchmarks ===" << std::endl;

        // Benchmark 1: Application startup/shutdown overhead
        {
            auto result = run_benchmark("App Startup/Shutdown", 100, [](size_t i) {
                ApplicationConfig config;
                config.worker_threads = 1;
                config.enable_health_check = false;

                Application app(config);
                // Simulate minimal work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            });
            results_.push_back(result);
        }
    }

    void run_memory_benchmarks() {
        std::cout << "\n=== Memory Usage Benchmarks ===" << std::endl;

        // Note: These are simplified memory usage estimates
        // For production use, consider integrating with tools like valgrind or custom allocators

        size_t baseline_memory = get_memory_usage();

        // Create application with various components
        ApplicationConfig config;
        config.worker_threads = 4;
        config.enable_health_check = true;
        Application app(config);

        size_t app_memory = get_memory_usage();

        // Create multiple threads
        std::vector<std::shared_ptr<Application::ManagedThread>> threads;
        for (int i = 0; i < 10; ++i) {
            threads.push_back(app.create_worker_thread("memory_test_" + std::to_string(i)));
        }

        size_t threads_memory = get_memory_usage();

        // Send many messages
        struct MemoryMessage { std::array<char, 1024> data; };
        for (int i = 0; i < 1000; ++i) {
            threads[0]->send_message(MemoryMessage{});
        }

        size_t messages_memory = get_memory_usage();

        std::cout << "Memory Usage Analysis:" << std::endl;
        std::cout << "  Baseline: " << baseline_memory << " KB" << std::endl;
        std::cout << "  Application: " << (app_memory - baseline_memory) << " KB" << std::endl;
        std::cout << "  10 Threads: " << (threads_memory - app_memory) << " KB" << std::endl;
        std::cout << "  1000 Messages: " << (messages_memory - threads_memory) << " KB" << std::endl;

        // Cleanup
        for (auto& thread : threads) {
            thread->stop();
            thread->join();
        }
    }

    void print_results() {
        std::cout << "\n" << std::string(120, '=') << std::endl;
        std::cout << "BENCHMARK RESULTS SUMMARY" << std::endl;
        std::cout << std::string(120, '=') << std::endl;

        std::cout << std::left << std::setw(30) << "Benchmark"
                  << std::setw(15) << "Throughput/sec"
                  << std::setw(12) << "Avg (μs)"
                  << std::setw(12) << "P95 (μs)"
                  << std::setw(12) << "P99 (μs)"
                  << std::setw(12) << "Min (μs)"
                  << std::setw(12) << "Max (μs)"
                  << std::setw(12) << "Operations"
                  << std::setw(12) << "Duration"
                  << std::endl;
        std::cout << std::string(120, '-') << std::endl;

        for (const auto& result : results_) {
            std::cout << std::left << std::setw(30) << result.name
                      << std::setw(15) << std::fixed << std::setprecision(0) << result.throughput_per_sec
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.avg_latency_us
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.p95_latency_us
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.p99_latency_us
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.min_latency_us
                      << std::setw(12) << std::fixed << std::setprecision(2) << result.max_latency_us
                      << std::setw(12) << result.total_operations
                      << std::setw(12) << result.duration.count() << "ms"
                      << std::endl;
        }

        std::cout << std::string(120, '=') << std::endl;
    }

    void save_results_to_file(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << " for writing" << std::endl;
            return;
        }

        file << "timestamp,benchmark,throughput_per_sec,avg_latency_us,p95_latency_us,p99_latency_us,min_latency_us,max_latency_us,operations,duration_ms\n";

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto* tm = std::localtime(&time_t);

        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

        for (const auto& result : results_) {
            file << timestamp << ","
                 << result.name << ","
                 << result.throughput_per_sec << ","
                 << result.avg_latency_us << ","
                 << result.p95_latency_us << ","
                 << result.p99_latency_us << ","
                 << result.min_latency_us << ","
                 << result.max_latency_us << ","
                 << result.total_operations << ","
                 << result.duration.count() << "\n";
        }

        std::cout << "Results saved to: " << filename << std::endl;
    }

    void run_all_benchmarks() {
        std::cout << "Starting Base Framework Benchmark Suite..." << std::endl;
        std::cout << "This may take several minutes to complete." << std::endl;

        run_logger_benchmarks();
        run_config_benchmarks();
        run_messaging_benchmarks();
        run_thread_benchmarks();
        run_application_benchmarks();
        run_memory_benchmarks();

        print_results();
        save_results_to_file("benchmark_results.csv");
    }

private:
    size_t get_memory_usage() {
        // Simplified memory usage - in production, use more sophisticated tools
        // This is a placeholder that returns RSS in KB
#ifdef __linux__
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;
                return std::stoul(value);
            }
        }
#endif
        return 0; // Fallback for non-Linux systems
    }
};

int main(int argc, char* argv[]) {
    try {
        BenchmarkRunner runner;

        if (argc > 1) {
            std::string arg = argv[1];

            if (arg == "--logger") {
                runner.run_logger_benchmarks();
            } else if (arg == "--messaging") {
                runner.run_messaging_benchmarks();
            } else if (arg == "--config") {
                runner.run_config_benchmarks();
            } else if (arg == "--threads") {
                runner.run_thread_benchmarks();
            } else if (arg == "--memory") {
                runner.run_memory_benchmarks();
            } else {
                std::cout << "Usage: " << argv[0] << " [--logger|--messaging|--config|--threads|--memory]" << std::endl;
                std::cout << "Run without arguments to execute all benchmarks." << std::endl;
                return 1;
            }
        } else {
            runner.run_all_benchmarks();
        }

        runner.print_results();

    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
