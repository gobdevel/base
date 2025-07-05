# Base Framework Benchmarks

This directory contains performance benchmarks for the Base framework using Google Benchmark.

## Overview

We use Google Benchmark as our benchmarking engine, which provides:

- ✅ **Statistical Analysis**: Robust statistical measurements with automatic outlier detection
- ✅ **CPU Time & Wall Time**: Accurate time measurements with CPU scaling awareness
- ✅ **Memory Benchmarks**: Built-in memory allocation tracking
- ✅ **JSON/CSV Output**: Machine-readable results for CI/CD integration
- ✅ **Regression Detection**: Compare results across runs
- ✅ **Industry Standard**: Used by Google, LLVM, and many C++ projects

## Quick Start

### Building and Running Benchmarks

```bash
# Build the project (includes benchmarks)
cd build/Release
cmake ../..
make

# Run table benchmarks
./benchmarks/table_benchmark

# Run with JSON output
./benchmarks/table_benchmark --benchmark_format=json --benchmark_out=results.json

# Run specific benchmarks
./benchmarks/table_benchmark --benchmark_filter="InsertRows"

# Show only summary
./benchmarks/table_benchmark --benchmark_format=console --benchmark_counters_tabular=true
```

### Using Benchmark Targets

```bash
# From the build directory
make run_table_benchmark               # Run with console output
make run_table_benchmark_json          # Run with JSON output
```

## Available Benchmarks

### Table Benchmarks

Located in `table_benchmark.cpp`, these test the performance of our table system:

#### Insert Operations

- **InsertRows**: Bulk row insertion performance
- **InsertRowsWithStrings**: String-heavy insertion performance
- **InsertRandomOrder**: Non-sequential insertion patterns

#### Query Operations

- **FindByValue**: Value-based row lookup
- **FilterRows**: Conditional row filtering
- **SortTable**: Table sorting performance

#### Table Operations

- **CreateTable**: Table creation overhead
- **CopyTable**: Deep copy performance
- **SerializeTable**: JSON serialization performance

### Performance Expectations

| Operation          | Expected Throughput | Notes             |
| ------------------ | ------------------- | ----------------- |
| Row Insertion      | >100K rows/sec      | Bulk operations   |
| Value Lookup       | >1M lookups/sec     | Hash-based        |
| Table Copy         | >50K rows/sec       | Deep copy         |
| JSON Serialization | >25K rows/sec       | String formatting |

## Benchmark Adapter

The `benchmark_adapter.h` provides domain-specific extensions:

### Profile System

```cpp
// Use predefined performance profiles
BENCHMARK_PROFILE(MyBenchmark, Quick);       // 10-100 operations
BENCHMARK_PROFILE(MyBenchmark, Development); // 1K-10K operations
BENCHMARK_PROFILE(MyBenchmark, Performance); // 100K+ operations

// Or register all profiles at once
BENCHMARK_ALL_PROFILES(MyBenchmark);
```

### Custom Metrics

```cpp
// Add table-specific metrics
base::benchmark_adapter::TableMetrics::add_table_metrics(state, rows, columns);
base::benchmark_adapter::TableMetrics::add_throughput_metrics(state, operations);
```

## Running Performance Analysis

### Comparing Runs

```bash
# Save baseline
./table_benchmark --benchmark_out=baseline.json --benchmark_out_format=json

# After changes, compare
./table_benchmark --benchmark_out=current.json --benchmark_out_format=json
compare.py benchmarks baseline.json current.json
```

### CI Integration

```bash
# For CI systems - fast, reliable results
./table_benchmark --benchmark_min_time=0.1 --benchmark_repetitions=3
```

### Deep Analysis

```bash
# Detailed analysis with counters
./table_benchmark --benchmark_counters_tabular=true --benchmark_format=json
```

## Writing New Benchmarks

### Basic Benchmark

```cpp
#include <benchmark/benchmark.h>
#include "../include/your_component.h"

static void BM_YourOperation(benchmark::State& state) {
    // Setup
    YourComponent component;

    for (auto _ : state) {
        // Operation to benchmark
        benchmark::DoNotOptimize(component.your_operation());
    }

    // Optional: Add custom metrics
    state.counters["Operations"] = state.iterations();
}
BENCHMARK(BM_YourOperation);

BENCHMARK_MAIN();
```

### Using the Adapter

```cpp
#include "benchmark_adapter.h"

BENCHMARK_PROFILE(YourBenchmark, Development) {
    auto scale = base::benchmark_adapter::ProfileManager::get_scale_factor(
        base::benchmark_adapter::Profile::Development);

    for (auto _ : state) {
        // Your scaled benchmark
        perform_operations(scale);
    }

    base::benchmark_adapter::TableMetrics::add_throughput_metrics(state, scale);
}
```

## Output Format

Google Benchmark provides rich output:

```
---------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations
---------------------------------------------------------------------
InsertRows/1000                  245 μs          244 μs         2856
InsertRows/10000                2398 μs         2394 μs          292
InsertRowsWithStrings/1000       423 μs          422 μs         1658
FindByValue/10000                 89 μs           89 μs         7865
```

With JSON output, you get detailed statistics, memory info, and custom counters for automated analysis.

## Dependencies

- **Google Benchmark**: Core benchmarking engine
- **Conan**: Package management (Google Benchmark provided as test dependency)
- **CMake**: Build system integration

## Best Practices

1. **Warm-up**: Google Benchmark handles warm-up automatically
2. **CPU Scaling**: Results are CPU scaling-aware
3. **Memory**: Use `DoNotOptimize()` to prevent unwanted optimizations
4. **Reproducibility**: Use `--benchmark_repetitions` for stable results
5. **Filtering**: Use `--benchmark_filter` to run specific tests during development
