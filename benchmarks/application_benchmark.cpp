/*
 * @file application_benchmark.cpp
 * @brief Comprehensive benchmarks for application.h framework
 *
 * Benchmarks the core application framework components including:
 * - Application lifecycle operations
 * - Task scheduling and execution
 * - Thread management and messaging
 * - Component management
 * - Signal handling performance
 * - Memory usage and efficiency
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <benchmark/benchmark.h>
#include "benchmark_adapter.h"
#include <application.h>
#include <logger.h>
#include <chrono>
#include <memory>
#include <atomic>
#include <vector>
#include <future>
#include <thread>
#include <random>

using namespace base;
using namespace base::benchmark_adapter;

// ============================================================================
// Logger Initialization - Lazy Initialization to Avoid Static Order Issues
// ============================================================================

bool ensure_logger_initialized() {
    static bool initialized = false;
    if (!initialized) {
        // Initialize logger with minimal output for benchmarks
        base::LoggerConfig config;
        config.level = base::LogLevel::Info; // Only show errors
        config.enable_console = false;        // Disable console output
        config.enable_colors = false;        // Disable colors
        base::Logger::init(config);
        initialized = true;
    }
    return true;
}

// ============================================================================
// Test Application Classes for Benchmarking
// ============================================================================

class MinimalTestApp : public Application {
public:
    MinimalTestApp() : Application(create_minimal_config()) {}

    static ApplicationConfig create_minimal_config() {
        ApplicationConfig config;
        config.name = "benchmark_test_app";
        config.worker_threads = 1;
        config.enable_health_check = false;
        config.parse_command_line = false;
        config.daemonize = false;
        config.enable_cli = false;
        return config;
    }

    void start_app() {
        // Start application in a separate thread and
        // return future for startup status
        startup_future_ = std::async(std::launch::async, [this]() {
            return run();
        });

        // Wait for application to reach running state
        while (state() != ApplicationState::Running) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    void stop_app() {
        // Shutdown the application gracefully
        shutdown();
        // Wait for the application to stop
        while (state() != ApplicationState::Stopped) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        // Ensure the startup future is valid and get the result
        if (startup_future_.valid()) {
            try {
                startup_future_.get(); // Ensure any exceptions are propagated
            } catch (const std::exception& e) {
                // Log any exceptions that occurred during startup
                base::Logger::error("Application startup failed: {}", e.what());
            }
        }
    }

protected:
    bool on_initialize() override { initialized_ = true; return true; }
    bool on_start() override { started_ = true; return true; }
    bool on_stop() override { stopped_ = true; return true; }

public:
    std::atomic<bool> initialized_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> stopped_{false};
    std::future<int> startup_future_;
};

class MultiThreadTestApp : public Application {
public:
    explicit MultiThreadTestApp(size_t num_threads) : Application(create_config(num_threads)) {}

    static ApplicationConfig create_config(size_t num_threads) {
        ApplicationConfig config;
        config.name = "multithread_benchmark_app";
        config.worker_threads = num_threads;
        config.enable_health_check = false;
        config.parse_command_line = false;
        config.daemonize = false;
        config.enable_cli = false;
        return config;
    }

    void start_app() {
        // Start application in a separate thread and
        // return future for startup status
        startup_future_ = std::async(std::launch::async, [this]() {
            return run();
        });

        // Wait for application to reach running state
        while (state() != ApplicationState::Running) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    void stop_app() {
        // Shutdown the application gracefully
        shutdown();
        // Wait for the application to stop
        while (state() != ApplicationState::Stopped) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        // Ensure the startup future is valid and get the result
        if (startup_future_.valid()) {
            try {
                startup_future_.get(); // Ensure any exceptions are propagated
            } catch (const std::exception& e) {
                // Log any exceptions that occurred during startup
                base::Logger::error("Application startup failed: {}", e.what());
            }
        }
    }
    private:
        std::future<int> startup_future_;
};

class TestComponent : public ApplicationComponent {
public:
    TestComponent(std::string name) : name_(std::move(name)) {}

    bool initialize(Application& app) override {
        init_count_++;
        return true;
    }

    bool start() override {
        start_count_++;
        return true;
    }

    bool stop() override {
        stop_count_++;
        return true;
    }

    std::string_view name() const override { return name_; }

    bool health_check() const override { return true; }

    std::atomic<int> init_count_{0};
    std::atomic<int> start_count_{0};
    std::atomic<int> stop_count_{0};

private:
    std::string name_;
};

// ============================================================================
// Helper Functions for Safe Benchmark Cleanup
// ============================================================================



// ============================================================================
// Application Lifecycle Benchmarks
// ============================================================================

static void BM_ApplicationCreation(benchmark::State& state) {
    ensure_logger_initialized();

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();
        benchmark::DoNotOptimize(app);
        // Application destructor will handle cleanup
    }

    state.counters["AppCreationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}
static void BM_ApplicationCreation_WithMemory(benchmark::State& state) {
    ensure_logger_initialized();

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();
        benchmark::DoNotOptimize(app);
        // Application destructor will handle cleanup
    }

    // Estimate memory usage per application instance
    size_t estimated_app_size = sizeof(MinimalTestApp) + 1024; // rough estimate including overhead
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(estimated_app_size));
    state.counters["AppCreationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.counters["EstimatedAppSize_KB"] = benchmark::Counter(static_cast<double>(estimated_app_size) / 1024);
}
BENCHMARK(BM_ApplicationCreation_WithMemory)->Unit(benchmark::kMicrosecond);

static void BM_ApplicationInitialization(benchmark::State& state) {
    ensure_logger_initialized();

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();

        auto start_time = std::chrono::high_resolution_clock::now();

        app->start_app();

        auto end_time = std::chrono::high_resolution_clock::now();

        // Proper shutdown sequence
        app->stop_app();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);
    }

    state.counters["InitializationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_ApplicationInitialization)->Unit(benchmark::kMicrosecond)->Iterations(10);

static void BM_ApplicationStartupShutdown(benchmark::State& state) {
    ensure_logger_initialized();

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();

        auto start_time = std::chrono::high_resolution_clock::now();
        app->start_app();

        app->stop_app();

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);
    }

    state.counters["LifecyclesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_ApplicationStartupShutdown)->Unit(benchmark::kMicrosecond)->Iterations(5);

// ============================================================================
// Task Scheduling Benchmarks
// ============================================================================

static void BM_TaskScheduling(benchmark::State& state) {
    ensure_logger_initialized();

    const int tasks_per_iteration = state.range(0);

    for (auto _ : state) {
        // Create a fresh application instance for each iteration
        auto app = std::make_unique<MinimalTestApp>();

        app->start_app();
        std::atomic<int> completed_tasks{0};

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < tasks_per_iteration; ++i) {
            app->post_task([&completed_tasks]() {
                completed_tasks.fetch_add(1, std::memory_order_relaxed);
            });
        }

        // Wait for all tasks to complete
        while (completed_tasks.load() < tasks_per_iteration) {
            std::this_thread::yield();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);

        // Clean up
        app->stop_app();
    }

    state.counters["TasksPerIteration"] = benchmark::Counter(tasks_per_iteration);
    state.counters["TasksPerSec"] = benchmark::Counter(tasks_per_iteration * state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_TaskScheduling)->Range(1, 100)->Unit(benchmark::kMicrosecond);

static void BM_TaskPriorityScheduling(benchmark::State& state) {
    ensure_logger_initialized();

    const int tasks_per_priority = state.range(0);

    for (auto _ : state) {
        // Create a fresh application instance for each iteration
        auto app = std::make_unique<MinimalTestApp>();

        app->start_app();
        std::atomic<int> completed_tasks{0};

        auto start_time = std::chrono::high_resolution_clock::now();

        // Schedule tasks with different priorities
        for (int i = 0; i < tasks_per_priority; ++i) {
            app->post_task([&completed_tasks]() {
                completed_tasks.fetch_add(1, std::memory_order_relaxed);
            }, TaskPriority::Low);

            app->post_task([&completed_tasks]() {
                completed_tasks.fetch_add(1, std::memory_order_relaxed);
            }, TaskPriority::Normal);

            app->post_task([&completed_tasks]() {
                completed_tasks.fetch_add(1, std::memory_order_relaxed);
            }, TaskPriority::High);

            app->post_task([&completed_tasks]() {
                completed_tasks.fetch_add(1, std::memory_order_relaxed);
            }, TaskPriority::Critical);
        }

        while (completed_tasks.load() < tasks_per_priority * 4) {
            std::this_thread::yield();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);

        // Clean up
        app->stop_app();
    }

    state.counters["TotalTasks"] = benchmark::Counter(tasks_per_priority * 4);
    state.counters["TasksPerSec"] = benchmark::Counter(tasks_per_priority * 4 * state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_TaskPriorityScheduling)->Range(1, 10)->Unit(benchmark::kMicrosecond);

static void BM_RecurringTaskScheduling(benchmark::State& state) {
    ensure_logger_initialized();

    const int num_recurring_tasks = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        auto app = std::make_unique<MinimalTestApp>();
        app->start_app();

        std::vector<std::size_t> task_ids;
        std::atomic<int> execution_count{0};
        state.ResumeTiming();

        auto start_time = std::chrono::high_resolution_clock::now();

        // Schedule recurring tasks
        for (int i = 0; i < num_recurring_tasks; ++i) {
            auto task_id = app->schedule_recurring_task([&execution_count]() {
                execution_count.fetch_add(1, std::memory_order_relaxed);
            }, std::chrono::milliseconds(1));
            task_ids.push_back(task_id);
        }

        // Let them run for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Cancel all tasks
        for (auto task_id : task_ids) {
            app->cancel_recurring_task(task_id);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);

        state.PauseTiming();

        app->stop_app();
        state.ResumeTiming();
    }

    state.counters["RecurringTasks"] = benchmark::Counter(num_recurring_tasks);
    state.counters["TaskSetupPerSec"] = benchmark::Counter(num_recurring_tasks * state.iterations(), benchmark::Counter::kIsRate);

}
BENCHMARK(BM_RecurringTaskScheduling)->Range(1, 100)->Unit(benchmark::kMicrosecond);

/*
// ============================================================================
// Thread Management Benchmarks - Temporarily Disabled
// ============================================================================

static void BM_ManagedThreadCreation(benchmark::State& state) {
    // ... disabled for debugging hang issues
}

static void BM_ThreadMessaging(benchmark::State& state) {
    // ... disabled for debugging hang issues
}

static void BM_EventDrivenThreadPerformance(benchmark::State& state) {
    // ... disabled for debugging hang issues
}
*/

// ============================================================================
// Component Management Benchmarks
// ============================================================================

static void BM_ComponentManagement(benchmark::State& state) {
    ensure_logger_initialized();

    const int components_per_iteration = state.range(0);

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();

        auto start_time = std::chrono::high_resolution_clock::now();

        // Add components
        for (int i = 0; i < components_per_iteration; ++i) {
            app->add_component(std::make_unique<TestComponent>("component_" + std::to_string(i)));
        }

        // Start application in a separate thread
        app->start_app();

        // A brief sleep to allow the application to stabilize
        std::this_thread::sleep_for(std::chrono::microseconds(10));

        // stop the application
        app->stop_app();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);
    }

    state.counters["ComponentsPerIteration"] = benchmark::Counter(components_per_iteration);
    state.counters["ComponentOpsPerSec"] = benchmark::Counter(components_per_iteration * state.iterations(), benchmark::Counter::kIsRate);

}
BENCHMARK(BM_ComponentManagement)->Range(1, 100)->Unit(benchmark::kMicrosecond);

static void BM_ComponentLookup(benchmark::State& state) {
    ensure_logger_initialized();

    const int num_components = state.range(0);

    // Setup once before all iterations
    auto app = std::make_unique<MinimalTestApp>();

    // Add components
    for (int i = 0; i < num_components; ++i) {
        app->add_component(std::make_unique<TestComponent>("component_" + std::to_string(i)));
    }

    // Start application in a separate thread
    app->start_app();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_components - 1);

    for (auto _ : state) {
        int component_idx = dis(gen);
        std::string component_name = "component_" + std::to_string(component_idx);

        auto component = app->get_component(component_name);
        benchmark::DoNotOptimize(component);
    }

    // Cleanup after all iterations
    app->stop_app();

    state.counters["ComponentCount"] = benchmark::Counter(num_components);
    state.counters["LookupsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_ComponentLookup)->Range(10, 1000)->Unit(benchmark::kNanosecond);

// ============================================================================
// Memory and Scalability Benchmarks
// ============================================================================

static void BM_ApplicationMemoryUsage(benchmark::State& state) {
    ensure_logger_initialized();

    const int worker_threads = state.range(0);

    // Estimate memory usage for reporting
    size_t estimated_memory_per_iteration = worker_threads * 1024 * 1024; // Rough estimate: 1MB per worker thread

    for (auto _ : state) {
        auto app = std::make_unique<MultiThreadTestApp>(worker_threads);

        // Add some components and threads
        for (int i = 0; i < 10; ++i) {
            app->add_component(std::make_unique<TestComponent>("comp_" + std::to_string(i)));
        }

        app->start_app();

        // Create additional managed threads
        std::vector<std::shared_ptr<ManagedThreadBase>> threads;
        for (int i = 0; i < 5; ++i) {
            threads.push_back(app->create_thread("worker_" + std::to_string(i), [](ManagedThreadBase& thread) {
                // Simple worker thread that just runs the event loop
            }));
        }

        // Schedule some recurring tasks
        std::vector<std::size_t> task_ids;
        for (int i = 0; i < 10; ++i) {
            task_ids.push_back(app->schedule_recurring_task([]() {}, std::chrono::milliseconds(100)));
        }

        // Let it run briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Clean up
        for (auto task_id : task_ids) {
            app->cancel_recurring_task(task_id);
        }

        for (auto& thread : threads) {
            thread->request_stop();
            // No manual join needed - std::jthread auto-joins on destruction
        }

        app->stop_app();

        benchmark::DoNotOptimize(app);
    }

    // Use Google Benchmark's built-in memory tracking
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(estimated_memory_per_iteration));

    state.counters["WorkerThreads"] = benchmark::Counter(worker_threads);
    state.counters["ApplicationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.counters["EstimatedMemoryMB"] = benchmark::Counter(static_cast<double>(estimated_memory_per_iteration) / (1024*1024));

}
BENCHMARK(BM_ApplicationMemoryUsage)->Range(1, 16)->Unit(benchmark::kMillisecond);

static void BM_ConcurrentTaskExecution(benchmark::State& state) {
    ensure_logger_initialized();

    const int concurrent_tasks = state.range(0);

    for (auto _ : state) {
        auto app = std::make_unique<MultiThreadTestApp>(8);
        app->start_app();

        std::atomic<int> completed_tasks{0};

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < concurrent_tasks; ++i) {
            app->post_task([&completed_tasks]() {
                // Simulate some work
                auto end = std::chrono::steady_clock::now() + std::chrono::microseconds(100);
                while (std::chrono::steady_clock::now() < end) {
                    // Busy wait
                }
                completed_tasks.fetch_add(1, std::memory_order_relaxed);
            });
        }

        // Wait for all tasks to complete
        while (completed_tasks.load() < concurrent_tasks) {
            std::this_thread::yield();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);

        app->stop_app();
    }

    state.counters["ConcurrentTasks"] = benchmark::Counter(concurrent_tasks);
    state.counters["ConcurrentOpsPerSec"] = benchmark::Counter(concurrent_tasks * state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_ConcurrentTaskExecution)->Range(10, 1000)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Signal Handling and Error Resilience Benchmarks
// ============================================================================

static void BM_SignalHandlerSetup(benchmark::State& state) {
    ensure_logger_initialized();

    const int signal_handlers = state.range(0);
    std::vector<int> signals = {SIGUSR1, SIGUSR2, SIGTERM, SIGINT, SIGHUP};

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();

        auto start_time = std::chrono::high_resolution_clock::now();

        // Set up signal handlers
        for (int i = 0; i < signal_handlers && i < signals.size(); ++i) {
            app->set_signal_handler(signals[i], [](int sig) {
                // Simple handler
            });
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);
    }

    state.counters["SignalHandlers"] = benchmark::Counter(signal_handlers);
    state.counters["HandlerSetupsPerSec"] = benchmark::Counter(signal_handlers * state.iterations(), benchmark::Counter::kIsRate);

}
BENCHMARK(BM_SignalHandlerSetup)->Range(1, 5)->Unit(benchmark::kNanosecond);

static void BM_ErrorHandlerPerformance(benchmark::State& state) {
    ensure_logger_initialized();

    const int error_tasks = state.range(0);

    for (auto _ : state) {
        auto app = std::make_unique<MinimalTestApp>();

        std::atomic<int> error_count{0};
        app->set_error_handler([&error_count](const std::exception& e) {
            error_count.fetch_add(1, std::memory_order_relaxed);
        });

        app->start_app();

        auto start_time = std::chrono::high_resolution_clock::now();

        // Post tasks that will throw exceptions
        for (int i = 0; i < error_tasks; ++i) {
            app->post_task([]() {
                throw std::runtime_error("Benchmark exception");
            });
        }

        // Wait for error handling to complete
        while (error_count.load() < error_tasks) {
            std::this_thread::yield();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        benchmark::DoNotOptimize(duration);

        app->stop_app();
    }

    state.counters["ErrorTasks"] = benchmark::Counter(error_tasks);
    state.counters["ErrorsHandledPerSec"] = benchmark::Counter(error_tasks * state.iterations(), benchmark::Counter::kIsRate);
}
// BENCHMARK(BM_ErrorHandlerPerformance)->Range(1, 100)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark Registration Complete
// ============================================================================

// Run the benchmarks
BENCHMARK_MAIN();
