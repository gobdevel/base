/*
 * @file memory_example.cpp
 * @brief Example demonstrating memory tracking in benchmarks using Google Benchmark's built-in features
 *
 * This example shows how to:
 * 1. Use SetBytesProcessed() for automatic throughput calculation
 * 2. Add custom memory counters
 * 3. Track memory deltas with simple OS-level monitoring
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <benchmark/benchmark.h>
#include "benchmark_adapter.h"
#include <vector>
#include <string>
#include <memory>

using namespace base::benchmark_adapter;

// Example 1: Basic memory tracking with SetBytesProcessed
static void BM_VectorAllocation(benchmark::State& state) {
    const size_t vector_size = state.range(0);
    const size_t bytes_per_element = sizeof(int);
    const size_t total_bytes = vector_size * bytes_per_element;

    for (auto _ : state) {
        std::vector<int> vec(vector_size, 42);
        benchmark::DoNotOptimize(vec.data());
        benchmark::ClobberMemory();
    }

    // Google Benchmark will automatically calculate MB/s
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(total_bytes));

    // Add custom memory counters
    state.counters["ElementCount"] = benchmark::Counter(vector_size);
    state.counters["BytesPerElement"] = benchmark::Counter(bytes_per_element);
    state.counters["TotalMemoryMB"] = benchmark::Counter(static_cast<double>(total_bytes) / (1024*1024));
}
BENCHMARK(BM_VectorAllocation)->Range(1<<10, 1<<20)->Unit(benchmark::kMicrosecond);

// Example 2: String operations with memory tracking using Google Benchmark features
static void BM_StringOperations_WithMemory(benchmark::State& state) {
    const size_t string_size = state.range(0);

    for (auto _ : state) {
        std::string str(string_size, 'A');
        std::string copy = str + str;  // Concatenation
        benchmark::DoNotOptimize(copy.data());
    }

    // Use Google Benchmark's built-in byte tracking
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(string_size * 3)); // original + copy + concatenated

    // Add custom memory counters
    state.counters["StringSize"] = benchmark::Counter(string_size);
    state.counters["TotalBytes"] = benchmark::Counter(string_size * 3);
}
BENCHMARK(BM_StringOperations_WithMemory)->Range(1<<8, 1<<16)->Unit(benchmark::kMicrosecond);

// Example 3: Table operations with memory tracking
static void BM_TableMemoryUsage(benchmark::State& state) {
    const size_t rows = state.range(0);
    const size_t columns = 10;
    const size_t total_bytes = rows * columns * sizeof(double);

    for (auto _ : state) {
        std::vector<std::vector<double>> table(rows, std::vector<double>(columns, 3.14159));

        // Do some work with the table
        for (auto& row : table) {
            for (auto& cell : row) {
                cell *= 2.0;
            }
        }
        benchmark::DoNotOptimize(table.data());
    }

    // Use the enhanced table metrics
    TableMetrics::add_table_memory_metrics(state, rows, columns, total_bytes);
}
BENCHMARK(BM_TableMemoryUsage)->Range(100, 10000)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
