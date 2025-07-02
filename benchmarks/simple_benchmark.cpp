/*
 * @file simple_benchmark.cpp
 * @brief Simplified benchmark runner that works with existing build system
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include "messaging.h"
#include "logger.h"
#include "config.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <atomic>
#include <numeric>
#include <algorithm>

using namespace crux;

class SimpleBenchmark {
private:
    struct Result {
        std::string name;
        double throughput;
        double avg_latency_us;
        size_t operations;
        std::chrono::milliseconds duration;
    };

    std::vector<Result> results_;

    template<typename Func>
    Result measure(const std::string& name, size_t iterations, Func&& func) {
        std::vector<double> latencies;
        latencies.reserve(iterations);

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            auto op_start = std::chrono::high_resolution_clock::now();
            func(i);
            auto op_end = std::chrono::high_resolution_clock::now();

            auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(op_end - op_start).count();
            latencies.push_back(latency_ns / 1000.0); // Convert to microseconds
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        double throughput = (iterations * 1000.0) / total_time.count();

        return Result{name, throughput, avg_latency, iterations, total_time};
    }

public:
    void benchmark_logger() {
        std::cout << "\n=== Logger Benchmarks ===" << std::endl;

        // Simple logging - reduce iterations to avoid flooding
        auto result1 = measure("Simple Logging", 5000, [](size_t i) {
            Logger::info("Test message {}", i);
        });
        results_.push_back(result1);

        // Complex logging
        auto result2 = measure("Complex Logging", 2500, [](size_t i) {
            Logger::info("Complex: id={}, val={:.2f}, status={}",
                        i, 3.14159 * i, (i % 2) ? "active" : "inactive");
        });
        results_.push_back(result2);

        // Level filtering - this should be fast as debug messages might be filtered out
        auto result3 = measure("Level Filtering", 10000, [](size_t i) {
            if (i % 4 == 0) Logger::debug("Debug {}", i);
            else if (i % 4 == 1) Logger::info("Info {}", i);
            else if (i % 4 == 2) Logger::warn("Warn {}", i);
            else Logger::error("Error {}", i);
        });
        results_.push_back(result3);
    }

    void benchmark_messaging() {
        std::cout << "\n=== Messaging Benchmarks ===" << std::endl;

        // Message queue throughput
        {
            MessageQueue queue;
            struct TestMsg { int id; std::string data; };

            auto result = measure("Message Queue", 5000, [&queue](size_t i) {
                queue.send(TestMsg{static_cast<int>(i), "test"});
                auto msg = queue.receive();
                volatile auto* ptr = msg.get();
                (void)ptr;
            });
            results_.push_back(result);
        }

        // Lock-free messaging test (same scope as ThreadMsg)
        {
            struct ThreadMsg { size_t id; };
            LockFreeMessageQueue<ThreadMsg> lockfree_queue;
            const size_t msg_count = 100000;

            auto result = measure("Lock-Free Queue", msg_count, [&lockfree_queue](size_t i) {
                lockfree_queue.send(ThreadMsg{i});
                auto msg = lockfree_queue.try_receive();
                volatile auto* ptr = msg.get();
                (void)ptr;
            });
            results_.push_back(result);
        }

        // Cross-thread messaging - Basic throughput
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            struct ThreadMsg { size_t id; };

            auto sender = app.create_worker_thread("sender");
            auto receiver = app.create_worker_thread("receiver");

            std::atomic<size_t> received{0};
            const size_t msg_count = 10000; // Increased back to 10k for better measurement

            receiver->subscribe_to_messages<ThreadMsg>([&received](const Message<ThreadMsg>&) {
                received++;
            });

            auto start = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < msg_count; ++i) {
                app.send_message_to_thread("receiver", ThreadMsg{i});
            }

            // Wait for completion with timeout
            auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (received < msg_count && std::chrono::steady_clock::now() < timeout) {
                std::this_thread::sleep_for(std::chrono::microseconds(10)); // Reduced sleep for better responsiveness
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double throughput = (msg_count * 1000.0) / duration.count();
            double avg_latency = duration.count() * 1000.0 / msg_count;

            results_.push_back(Result{"Cross-Thread Messaging", throughput, avg_latency, msg_count, duration});

            sender->stop();
            receiver->stop();
            sender->join();
            receiver->join();
        }

        // Cross-thread messaging - Latency measurement (improved)
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            struct LatencyMsg {
                size_t id;
                std::chrono::high_resolution_clock::time_point send_time;
            };

            auto sender = app.create_worker_thread("sender");
            auto receiver = app.create_worker_thread("receiver");

            std::vector<double> latencies;
            std::mutex latencies_mutex;
            std::atomic<size_t> received{0};
            const size_t msg_count = 1000;

            receiver->subscribe_to_messages<LatencyMsg>([&](const Message<LatencyMsg>& msg) {
                auto receive_time = std::chrono::high_resolution_clock::now();
                auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    receive_time - msg.data().send_time).count();

                // Only include reasonable latency measurements (filter outliers)
                if (latency_ns > 0 && latency_ns < 100000000) { // Less than 100ms
                    std::lock_guard<std::mutex> lock(latencies_mutex);
                    latencies.push_back(latency_ns / 1000.0); // Convert to microseconds
                }
                received++;
            });

            // Warmup and ensure threads are ready
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            auto start = std::chrono::high_resolution_clock::now();

            // Send messages in a tight loop without artificial delays
            for (size_t i = 0; i < msg_count; ++i) {
                auto send_time = std::chrono::high_resolution_clock::now();
                app.send_message_to_thread("receiver", LatencyMsg{i, send_time});
            }

            // Wait for completion
            auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (received < msg_count && std::chrono::steady_clock::now() < timeout) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double avg_latency = 0.0;
            if (!latencies.empty()) {
                // Calculate median instead of mean for better outlier resistance
                std::sort(latencies.begin(), latencies.end());
                size_t mid = latencies.size() / 2;
                if (latencies.size() % 2 == 0) {
                    avg_latency = (latencies[mid-1] + latencies[mid]) / 2.0;
                } else {
                    avg_latency = latencies[mid];
                }
            }

            double throughput = (msg_count * 1000.0) / duration.count();

            results_.push_back(Result{"Cross-Thread Latency", throughput, avg_latency, msg_count, duration});

            sender->stop();
            receiver->stop();
            sender->join();
            receiver->join();
        }

        // Ping-pong latency test (round-trip measurement)
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            struct PingMsg { size_t id; };
            struct PongMsg { size_t id; std::chrono::high_resolution_clock::time_point ping_time; };

            auto ping_thread = app.create_worker_thread("ping");
            auto pong_thread = app.create_worker_thread("pong");

            std::vector<double> round_trip_latencies;
            std::mutex latencies_mutex;
            std::atomic<size_t> pongs_received{0};
            const size_t ping_count = 500;

            // Pong thread: receives ping, sends pong back immediately
            pong_thread->subscribe_to_messages<PingMsg>([&](const Message<PingMsg>& msg) {
                auto ping_time = std::chrono::high_resolution_clock::now();
                app.send_message_to_thread("ping", PongMsg{msg.data().id, ping_time});
            });

            // Ping thread: receives pong, calculates round-trip time
            ping_thread->subscribe_to_messages<PongMsg>([&](const Message<PongMsg>& msg) {
                auto pong_time = std::chrono::high_resolution_clock::now();
                auto round_trip_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    pong_time - msg.data().ping_time).count();

                if (round_trip_ns > 0 && round_trip_ns < 50000000) { // Less than 50ms
                    std::lock_guard<std::mutex> lock(latencies_mutex);
                    // Divide by 2 for one-way latency estimate
                    round_trip_latencies.push_back((round_trip_ns / 1000.0) / 2.0);
                }
                pongs_received++;
            });

            // Warmup
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            auto start = std::chrono::high_resolution_clock::now();

            // Send pings from main thread
            for (size_t i = 0; i < ping_count; ++i) {
                app.send_message_to_thread("pong", PingMsg{i});
                // Small delay to avoid overwhelming
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            // Wait for all pongs
            auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (pongs_received < ping_count && std::chrono::steady_clock::now() < timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double avg_latency = 0.0;
            if (!round_trip_latencies.empty()) {
                // Use median for better outlier resistance
                std::sort(round_trip_latencies.begin(), round_trip_latencies.end());
                size_t mid = round_trip_latencies.size() / 2;
                if (round_trip_latencies.size() % 2 == 0) {
                    avg_latency = (round_trip_latencies[mid-1] + round_trip_latencies[mid]) / 2.0;
                } else {
                    avg_latency = round_trip_latencies[mid];
                }
            }

            double throughput = (ping_count * 1000.0) / duration.count();

            results_.push_back(Result{"Ping-Pong Latency", throughput, avg_latency, ping_count, duration});

            ping_thread->stop();
            pong_thread->stop();
            ping_thread->join();
            pong_thread->join();
        }

        // Minimal latency test (direct queue measurement)
        {
            struct SimpleMsg {
                size_t id;
                std::chrono::high_resolution_clock::time_point timestamp;
            };

            MessageQueue direct_queue;
            std::vector<double> minimal_latencies;
            const size_t test_count = 1000;

            auto result = measure("Minimal Queue Latency", test_count, [&](size_t i) {
                auto send_time = std::chrono::high_resolution_clock::now();
                direct_queue.send(SimpleMsg{i, send_time});

                auto msg = direct_queue.try_receive();
                if (msg) {
                    auto receive_time = std::chrono::high_resolution_clock::now();
                    if (auto* typed_msg = dynamic_cast<Message<SimpleMsg>*>(msg.get())) {
                        auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            receive_time - typed_msg->data().timestamp).count();
                        minimal_latencies.push_back(latency_ns / 1000.0);
                    }
                }
            });

            // Calculate actual minimal latency
            double minimal_avg = 0.0;
            if (!minimal_latencies.empty()) {
                std::sort(minimal_latencies.begin(), minimal_latencies.end());
                size_t mid = minimal_latencies.size() / 2;
                minimal_avg = minimal_latencies[mid];
            }

            results_.push_back(Result{"Minimal Queue Latency", result.throughput, minimal_avg, test_count, result.duration});
        }

        // Event-driven cross-thread messaging test
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            struct EventMsg {
                size_t id;
                std::chrono::high_resolution_clock::time_point send_time;
            };

            auto sender = app.create_event_driven_thread("sender");
            auto receiver = app.create_event_driven_thread("receiver");

            std::vector<double> event_latencies;
            std::mutex latencies_mutex;
            std::atomic<size_t> received{0};
            const size_t msg_count = 1000;

            receiver->subscribe_to_messages<EventMsg>([&](const Message<EventMsg>& msg) {
                auto receive_time = std::chrono::high_resolution_clock::now();
                auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    receive_time - msg.data().send_time).count();

                if (latency_ns > 0 && latency_ns < 50000000) { // Less than 50ms
                    std::lock_guard<std::mutex> lock(latencies_mutex);
                    event_latencies.push_back(latency_ns / 1000.0); // Convert to microseconds
                }
                received++;
            });

            // Warmup
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto start = std::chrono::high_resolution_clock::now();

            // Send messages directly to event-driven thread
            for (size_t i = 0; i < msg_count; ++i) {
                auto send_time = std::chrono::high_resolution_clock::now();
                receiver->send_message(EventMsg{i, send_time});
            }

            // Wait for completion
            auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (received < msg_count && std::chrono::steady_clock::now() < timeout) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double avg_latency = 0.0;
            if (!event_latencies.empty()) {
                // Use median for better outlier resistance
                std::sort(event_latencies.begin(), event_latencies.end());
                size_t mid = event_latencies.size() / 2;
                if (event_latencies.size() % 2 == 0) {
                    avg_latency = (event_latencies[mid-1] + event_latencies[mid]) / 2.0;
                } else {
                    avg_latency = event_latencies[mid];
                }
            }

            double throughput = (msg_count * 1000.0) / duration.count();

            results_.push_back(Result{"Event-Driven Latency", throughput, avg_latency, msg_count, duration});

            sender->stop();
            receiver->stop();
            sender->join();
            receiver->join();
        }

        // Event-driven vs Polling comparison
        {
            const size_t msg_count = 5000;

            // Test 1: Polling-based (current default)
            {
                ApplicationConfig config;
                config.worker_threads = 1;
                config.enable_health_check = false;
                Application app(config);

                struct TestMsg { size_t id; };

                auto sender = app.create_worker_thread("polling_sender");
                auto receiver = app.create_worker_thread("polling_receiver");

                std::atomic<size_t> received{0};

                receiver->subscribe_to_messages<TestMsg>([&received](const Message<TestMsg>&) {
                    received++;
                });

                auto start = std::chrono::high_resolution_clock::now();

                for (size_t i = 0; i < msg_count; ++i) {
                    app.send_message_to_thread("polling_receiver", TestMsg{i});
                }

                // Wait for completion
                auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                while (received < msg_count && std::chrono::steady_clock::now() < timeout) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }

                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                double throughput = (msg_count * 1000.0) / duration.count();
                double avg_latency = duration.count() * 1000.0 / msg_count;

                results_.push_back(Result{"Polling-Based Messaging", throughput, avg_latency, msg_count, duration});

                sender->stop();
                receiver->stop();
                sender->join();
                receiver->join();
            }

            // Test 2: Event-driven
            {
                ApplicationConfig config;
                config.worker_threads = 1;
                config.enable_health_check = false;
                Application app(config);

                struct TestMsg { size_t id; };

                auto sender = app.create_event_driven_thread("event_sender");
                auto receiver = app.create_event_driven_thread("event_receiver");

                std::atomic<size_t> received{0};

                receiver->subscribe_to_messages<TestMsg>([&received](const Message<TestMsg>&) {
                    received++;
                });

                // Give threads time to start
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                auto start = std::chrono::high_resolution_clock::now();

                for (size_t i = 0; i < msg_count; ++i) {
                    receiver->send_message(TestMsg{i});
                }

                // Wait for completion
                auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
                while (received < msg_count && std::chrono::steady_clock::now() < timeout) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }

                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                double throughput = (msg_count * 1000.0) / duration.count();
                double avg_latency = duration.count() * 1000.0 / msg_count;

                results_.push_back(Result{"Event-Driven Messaging", throughput, avg_latency, msg_count, duration});

                sender->stop();
                receiver->stop();
                sender->join();
                receiver->join();
            }
        }

        // Event-driven ping-pong latency test (round-trip measurement)
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            struct EventPingMsg { size_t id; };
            struct EventPongMsg { size_t id; std::chrono::high_resolution_clock::time_point ping_time; };

            auto event_ping_thread = app.create_event_driven_thread("event_ping");
            auto event_pong_thread = app.create_event_driven_thread("event_pong");

            std::vector<double> event_round_trip_latencies;
            std::mutex event_latencies_mutex;
            std::atomic<size_t> event_pongs_received{0};
            const size_t event_ping_count = 500;

            // Event-driven pong thread: receives ping, sends pong back immediately
            event_pong_thread->subscribe_to_messages<EventPingMsg>([&](const Message<EventPingMsg>& msg) {
                auto ping_time = std::chrono::high_resolution_clock::now();
                event_ping_thread->send_message(EventPongMsg{msg.data().id, ping_time});
            });

            // Event-driven ping thread: receives pong, calculates round-trip time
            event_ping_thread->subscribe_to_messages<EventPongMsg>([&](const Message<EventPongMsg>& msg) {
                auto pong_time = std::chrono::high_resolution_clock::now();
                auto round_trip_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    pong_time - msg.data().ping_time).count();

                if (round_trip_ns > 0 && round_trip_ns < 50000000) { // Less than 50ms
                    std::lock_guard<std::mutex> lock(event_latencies_mutex);
                    // Divide by 2 for one-way latency estimate
                    event_round_trip_latencies.push_back((round_trip_ns / 1000.0) / 2.0);
                }
                event_pongs_received++;
            });

            // Warmup for event-driven threads
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto start = std::chrono::high_resolution_clock::now();

            // Send event-driven pings
            for (size_t i = 0; i < event_ping_count; ++i) {
                event_pong_thread->send_message(EventPingMsg{i});
                // Minimal delay to avoid overwhelming
                std::this_thread::sleep_for(std::chrono::microseconds(5));
            }

            // Wait for all event-driven pongs
            auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (event_pongs_received < event_ping_count && std::chrono::steady_clock::now() < timeout) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double avg_latency = 0.0;
            if (!event_round_trip_latencies.empty()) {
                // Use median for better outlier resistance
                std::sort(event_round_trip_latencies.begin(), event_round_trip_latencies.end());
                size_t mid = event_round_trip_latencies.size() / 2;
                if (event_round_trip_latencies.size() % 2 == 0) {
                    avg_latency = (event_round_trip_latencies[mid-1] + event_round_trip_latencies[mid]) / 2.0;
                } else {
                    avg_latency = event_round_trip_latencies[mid];
                }
            }

            double throughput = (event_ping_count * 1000.0) / duration.count();

            results_.push_back(Result{"Event-Driven Ping-Pong", throughput, avg_latency, event_ping_count, duration});

            event_ping_thread->stop();
            event_pong_thread->stop();
            event_ping_thread->join();
            event_pong_thread->join();
        }
    }

    void benchmark_config() {
        std::cout << "\n=== Configuration Benchmarks ===" << std::endl;

        ConfigManager& config = ConfigManager::instance();
        std::string test_config = R"(
[app]
name = "benchmark"
version = "1.0"
threads = 4

[db]
host = "localhost"
port = 5432
)";

        config.load_from_string(test_config, "benchmark");

        // Config access
        auto result1 = measure("Config Access", 100000, [&config](size_t i) {
            volatile auto name = config.get_app_config("benchmark").name;
            volatile auto version = config.get_app_config("benchmark").version;
            (void)name; (void)version;
        });
        results_.push_back(result1);

        // Custom value lookup
        auto result2 = measure("Custom Lookup", 50000, [&config](size_t i) {
            volatile auto host = config.get_value<std::string>("db.host", "benchmark");
            volatile auto port = config.get_value<int>("db.port", "benchmark");
            (void)host; (void)port;
        });
        results_.push_back(result2);
    }

    void benchmark_threads() {
        std::cout << "\n=== Thread Benchmarks ===" << std::endl;

        // Thread creation/destruction
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;

            auto result = measure("Thread Create/Destroy", 500, [&config](size_t i) {
                Application app(config);
                auto thread = app.create_worker_thread("test_" + std::to_string(i));
                thread->stop();
                thread->join();
            });
            results_.push_back(result);
        }

        // Task posting
        {
            ApplicationConfig config;
            config.worker_threads = 1;
            config.enable_health_check = false;
            Application app(config);

            auto worker = app.create_worker_thread("task_worker");
            std::atomic<size_t> completed{0};

            auto result = measure("Task Posting", 25000, [&worker, &completed](size_t i) {
                worker->post_task([&completed]() { completed++; });
            });

            // Wait for tasks
            while (completed < 25000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            results_.push_back(result);
            worker->stop();
            worker->join();
        }
    }

    void print_results() {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "BENCHMARK RESULTS" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        std::cout << std::left
                  << std::setw(25) << "Benchmark"
                  << std::setw(15) << "Throughput/sec"
                  << std::setw(15) << "Avg Latency(μs)"
                  << std::setw(12) << "Operations"
                  << std::setw(10) << "Duration(ms)"
                  << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& result : results_) {
            std::cout << std::left
                      << std::setw(25) << result.name
                      << std::setw(15) << std::fixed << std::setprecision(0) << result.throughput
                      << std::setw(15) << std::fixed << std::setprecision(2) << result.avg_latency_us
                      << std::setw(12) << result.operations
                      << std::setw(10) << result.duration.count()
                      << std::endl;
        }
        std::cout << std::string(80, '=') << std::endl;
    }

    void run_all() {
        std::cout << "Crux Framework Simple Benchmark Suite" << std::endl;
        std::cout << "=====================================" << std::endl;

        benchmark_logger();
        benchmark_config();
        benchmark_messaging();
        benchmark_threads();

        print_results();
    }
};

int main(int argc, char* argv[]) {
    try {
        SimpleBenchmark bench;

        if (argc > 1) {
            std::string arg = argv[1];
            if (arg == "--logger") bench.benchmark_logger();
            else if (arg == "--messaging") bench.benchmark_messaging();
            else if (arg == "--config") bench.benchmark_config();
            else if (arg == "--threads") bench.benchmark_threads();
            else {
                std::cout << "Usage: " << argv[0] << " [--logger|--messaging|--config|--threads]" << std::endl;
                return 1;
            }
            bench.print_results();
        } else {
            bench.run_all();
        }

    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
