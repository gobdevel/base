/*
 * @file simple_performance_test.cpp
 * @brief Simple performance testing example for Base framework
 *
 * This demonstrates how to quickly measure performance of your applications
 * using the Base framework's built-in capabilities.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "../include/application.h"
#include "../include/messaging.h"
#include "../include/logger.h"

#include <chrono>
#include <iostream>
#include <atomic>

using namespace base;

// Simple performance measurement utility
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::string name_;

public:
    explicit PerformanceTimer(std::string name) : name_(std::move(name)) {
        start_time_ = std::chrono::high_resolution_clock::now();
        Logger::info("Started: {}", name_);
    }

    ~PerformanceTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        Logger::info("Completed: {} in {}μs", name_, duration.count());
    }
};

// Example: Measuring logger performance
void test_logger_performance() {
    std::cout << "\n=== Logger Performance Test ===" << std::endl;

    const int log_count = 10000;

    {
        PerformanceTimer timer("Logger Performance Test");
        for (int i = 0; i < log_count; ++i) {
            Logger::info("Test log message #{} with some data: {}", i, 42.5);
        }
    }

    Logger::info("Logged {} messages", log_count);
}

// Example: Measuring messaging performance
void test_messaging_performance() {
    std::cout << "\n=== Messaging Performance Test ===" << std::endl;

    ApplicationConfig config;
    config.worker_threads = 2;
    config.enable_health_check = false;
    Application app(config);

    struct TestMessage {
        int id;
        std::string data;
        std::chrono::high_resolution_clock::time_point timestamp;
    };

    auto sender = app.create_worker_thread("sender");
    auto receiver = app.create_worker_thread("receiver");

    std::atomic<int> messages_received{0};
    const int message_count = 5000;

    // Set up receiver
    receiver->subscribe_to_messages<TestMessage>([&messages_received](const Message<TestMessage>& msg) {
        messages_received++;
        if (messages_received % 1000 == 0) {
            Logger::debug("Received {} messages", messages_received.load());
        }
    });

    // Measure sending performance
    {
        PerformanceTimer timer("Messaging Send Performance");
        for (int i = 0; i < message_count; ++i) {
            sender->send_message(TestMessage{
                i,
                "test_data_" + std::to_string(i),
                std::chrono::high_resolution_clock::now()
            });
        }
    }

    // Wait for all messages to be received
    Logger::info("Waiting for {} messages to be processed...", message_count);
    auto start_wait = std::chrono::steady_clock::now();

    while (messages_received < message_count) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Timeout after 10 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start_wait).count() > 10) {
            Logger::error("Timeout waiting for messages. Received: {}/{}",
                         messages_received.load(), message_count);
            break;
        }
    }

    auto total_time = std::chrono::steady_clock::now() - start_wait;
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count();

    Logger::info("Total messaging performance: {} messages in {}ms ({:.1f} msg/sec)",
                messages_received.load(), total_ms,
                (messages_received.load() * 1000.0) / total_ms);

    // Cleanup
    sender->stop();
    receiver->stop();
    sender->join();
    receiver->join();
}

// Example: Measuring thread creation performance
void test_thread_performance() {
    std::cout << "\n=== Thread Performance Test ===" << std::endl;

    ApplicationConfig config;
    config.worker_threads = 1;
    config.enable_health_check = false;

    const int thread_count = 100;

    {
        PerformanceTimer timer("Thread Creation Performance");

        for (int i = 0; i < thread_count; ++i) {
            Application app(config);
            auto thread = app.create_worker_thread("test_thread_" + std::to_string(i));

            // Do some minimal work
            std::atomic<bool> work_done{false};
            thread->post_task([&work_done]() {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                work_done = true;
            });

            // Wait for work to complete
            while (!work_done) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            thread->stop();
            thread->join();
        }
    }

    Logger::info("Created and destroyed {} threads", thread_count);
}

// Example: Memory usage monitoring
void test_memory_usage() {
    std::cout << "\n=== Memory Usage Test ===" << std::endl;

    // This is a simplified example - for production use, integrate with
    // tools like valgrind, AddressSanitizer, or custom memory allocators

    Logger::info("Creating application with multiple threads...");

    ApplicationConfig config;
    config.worker_threads = 4;
    Application app(config);

    std::vector<std::shared_ptr<Application::ManagedThread>> threads;

    // Create many threads
    for (int i = 0; i < 20; ++i) {
        threads.push_back(app.create_worker_thread("memory_test_" + std::to_string(i)));
    }

    Logger::info("Created {} threads", threads.size());

    // Send many messages to test memory usage
    struct MemoryTestMessage {
        std::array<char, 1024> data; // 1KB per message
        int sequence;
    };

    const int messages_per_thread = 1000;

    {
        PerformanceTimer timer("Memory Stress Test");

        for (int thread_idx = 0; thread_idx < 10; ++thread_idx) {
            for (int msg_idx = 0; msg_idx < messages_per_thread; ++msg_idx) {
                threads[thread_idx]->send_message(MemoryTestMessage{{}, msg_idx});
            }
        }

        // Let messages process
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    Logger::info("Sent {} messages ({} KB) across {} threads",
                10 * messages_per_thread,
                (10 * messages_per_thread * sizeof(MemoryTestMessage)) / 1024,
                10);

    // Cleanup
    for (auto& thread : threads) {
        thread->stop();
        thread->join();
    }

    Logger::info("Memory test completed - check system monitor for actual usage");
}

// Simple benchmark comparison utility
template<typename Func>
void benchmark_function(const std::string& name, int iterations, Func&& func) {
    std::cout << "\nBenchmarking: " << name << " (" << iterations << " iterations)" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        func(i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double ops_per_sec = (iterations * 1000000.0) / duration.count();
    double avg_latency = duration.count() / static_cast<double>(iterations);

    std::cout << "Results:" << std::endl;
    std::cout << "  Total time: " << duration.count() << "μs" << std::endl;
    std::cout << "  Operations/sec: " << std::fixed << std::setprecision(0) << ops_per_sec << std::endl;
    std::cout << "  Average latency: " << std::fixed << std::setprecision(2) << avg_latency << "μs" << std::endl;
}

int main() {
    try {
        std::cout << "Base Framework - Simple Performance Testing Example" << std::endl;
        std::cout << "=================================================" << std::endl;

        // Test different components
        test_logger_performance();
        test_messaging_performance();
        test_thread_performance();
        test_memory_usage();

        // Example of using the benchmark utility
        std::cout << "\n=== Custom Benchmark Examples ===" << std::endl;

        // Benchmark string operations
        benchmark_function("String Creation", 100000, [](int i) {
            volatile std::string s = "test_string_" + std::to_string(i);
            (void)s;
        });

        // Benchmark logger calls
        benchmark_function("Logger Calls", 50000, [](int i) {
            Logger::debug("Debug message {}", i);
        });

        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "Performance testing completed!" << std::endl;
        std::cout << "For comprehensive benchmarks, use the benchmark_runner tool." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Performance test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
