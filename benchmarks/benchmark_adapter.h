/*
 * @file benchmark_adapter.h
 * @brief Adapter layer that adds our custom features on top of Google Benchmark
 *
 * This provides our profile system, custom metrics, and domain-specific
 * features while leveraging Google Benchmark's mature statistical engine.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <benchmark/benchmark.h>
#include <memory>
#include <string>

namespace base {
namespace benchmark_adapter {

enum class Profile {
    Quick,       // 10-100 operations
    Development, // 1K-10K operations
    CI,         // 10K operations
    Performance, // 100K+ operations
    Stress      // 1M+ operations
};

class ProfileManager {
public:
    static size_t get_scale_factor(Profile profile) {
        switch (profile) {
            case Profile::Quick: return 10;
            case Profile::Development: return 1000;
            case Profile::CI: return 10000;
            case Profile::Performance: return 100000;
            case Profile::Stress: return 1000000;
        }
        return 1000;
    }

    static void configure_benchmark(::benchmark::internal::Benchmark* bench, Profile profile) {
        auto scale = get_scale_factor(profile);

        switch (profile) {
            case Profile::Quick:
                bench->Iterations(3)->Unit(::benchmark::kMillisecond);
                break;
            case Profile::Development:
                bench->Iterations(5)->Unit(::benchmark::kMicrosecond);
                break;
            case Profile::CI:
                bench->Iterations(10)->Unit(::benchmark::kMicrosecond);
                break;
            case Profile::Performance:
                bench->MinTime(1.0)->Unit(::benchmark::kNanosecond);
                break;
            case Profile::Stress:
                bench->MinTime(5.0)->Unit(::benchmark::kNanosecond);
                break;
        }
    }
};

// Custom counter types for our domain
class TableMetrics {
public:
    static void add_table_metrics(::benchmark::State& state, size_t rows, size_t columns = 0) {
        state.counters["Rows"] = ::benchmark::Counter(rows);
        if (columns > 0) {
            state.counters["Columns"] = ::benchmark::Counter(columns);
            state.counters["Cells"] = ::benchmark::Counter(rows * columns);
        }
        state.counters["RowsPerSec"] = ::benchmark::Counter(rows, ::benchmark::Counter::kIsRate);
    }

    static void add_throughput_metrics(::benchmark::State& state, size_t operations, size_t data_size = 0) {
        state.counters["OpsPerSec"] = ::benchmark::Counter(operations, ::benchmark::Counter::kIsRate);
        if (data_size > 0) {
            state.counters["MBPerSec"] = ::benchmark::Counter(data_size, ::benchmark::Counter::kIsRate, ::benchmark::Counter::kIs1024);
        }
    }

    // Enhanced table metrics with memory tracking (uses Google Benchmark's SetBytesProcessed)
    static void add_table_memory_metrics(::benchmark::State& state, size_t rows, size_t columns, size_t total_bytes_processed) {
        add_table_metrics(state, rows, columns);

        // Use Google Benchmark's built-in memory throughput calculation
        state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(total_bytes_processed));

        // Add memory size info
        state.counters["MemoryMB"] = ::benchmark::Counter(static_cast<double>(total_bytes_processed) / (1024*1024));
        state.counters["BytesPerRow"] = ::benchmark::Counter(rows > 0 ? static_cast<double>(total_bytes_processed) / rows : 0);
    }
};

// Profile-aware benchmark registration
#define BENCHMARK_PROFILE(func, profile_type) \
    static void func##_##profile_type(::benchmark::State& state); \
    static auto benchmark_##func##_##profile_type = \
        ::benchmark::RegisterBenchmark(#func "_" #profile_type, func##_##profile_type) \
        ->Apply([](::benchmark::internal::Benchmark* bench) { \
            base::benchmark_adapter::ProfileManager::configure_benchmark(bench, base::benchmark_adapter::Profile::profile_type); \
        }); \
    static void func##_##profile_type(::benchmark::State& state)

// Multi-profile registration
#define BENCHMARK_ALL_PROFILES(func) \
    BENCHMARK_PROFILE(func, Quick); \
    BENCHMARK_PROFILE(func, Development); \
    BENCHMARK_PROFILE(func, Performance)

} // namespace benchmark_adapter
} // namespace base
