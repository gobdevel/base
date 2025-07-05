# Benchmark Runner Infrastructure

This document describes the comprehensive benchmark runner infrastructure designed for performance testing in the Base framework.

## Overview

The benchmark runner provides a standardized framework for creating, executing, and analyzing performance tests with support for:

- **Multiple Performance Profiles**: Quick validation, development testing, CI, full benchmarks, and stress testing
- **Statistical Analysis**: Automatic calculation of mean, median, standard deviation, and percentiles
- **Memory Tracking**: Monitor peak memory usage and memory deltas during tests
- **Concurrent Testing**: Support for multi-threaded benchmark scenarios
- **Comprehensive Reporting**: Detailed results with JSON/CSV export capabilities
- **Command-line Interface**: Easy profile selection and configuration
- **Easy Test Creation**: Macros and utilities for quick benchmark development

## Basic Usage

### 1. Simple Benchmark Test

```cpp
#include "benchmark_runner.h"

using namespace base::benchmark;

BENCHMARK_SIMPLE(MyBenchmarkTest) {
    auto scale = get_scale_factor();  // Automatically scales with profile

    // Setup your test data
    std::vector<int> data;
    data.reserve(scale);

    // Set metrics for result calculation
    result.set_operations_count(scale);

    // Time the operation
    BenchmarkTimer timer;
    timer.start();

    for (size_t i = 0; i < scale; ++i) {
        data.push_back(static_cast<int>(i));
    }

    timer.stop();
    result.add_timing(timer.get_elapsed());
}
```

### 2. Categorized Benchmark

```cpp
BENCHMARK_TEST(TableInsertBenchmark, "Database Operations") {
    auto scale = get_scale_factor();

    TableSchema schema;
    schema.add_column("id", DataType::Integer);
    schema.add_column("name", DataType::String);

    Table table(schema);
    result.set_operations_count(scale);

    BenchmarkTimer timer;
    timer.start();

    for (size_t i = 0; i < scale; ++i) {
        TableRow row;
        row.set_value("id", static_cast<int>(i));
        row.set_value("name", "User" + std::to_string(i));
        table.insert(row);
    }

    timer.stop();
    result.add_timing(timer.get_elapsed());

    // Add custom metrics
    result.add_custom_metric("table_size", static_cast<double>(table.size()), "rows");
}
```

### 3. Custom Benchmark Class

```cpp
class CustomBenchmark : public BenchmarkTest {
public:
    CustomBenchmark() : BenchmarkTest("CustomBenchmark", "Custom") {}

    void setup(const BenchmarkConfig& config) override {
        // Initialize test resources
        scale_factor_ = config.get_scale_factor();
        thread_count_ = config.get_thread_count();
    }

    void teardown() override {
        // Clean up resources
    }

    void run_benchmark(BenchmarkResult& result) override {
        // Implement your benchmark logic
        result.set_operations_count(scale_factor_);

        BenchmarkTimer timer;
        timer.start();

        // Your benchmark code here

        timer.stop();
        result.add_timing(timer.get_elapsed());
    }
};
```

### 4. Main Function and Registration

```cpp
void register_benchmarks(BenchmarkRunner& runner) {
    // Register macro-defined benchmarks
    BENCHMARK_REGISTER(runner, MyBenchmarkTest);
    BENCHMARK_REGISTER(runner, TableInsertBenchmark);

    // Register custom benchmark classes
    runner.add_test(std::make_unique<CustomBenchmark>());
}

// This macro creates a main function with command-line parsing
BENCHMARK_MAIN();
```

## Performance Profiles

The infrastructure supports different performance profiles for various testing scenarios:

### Quick Profile

- **Scale Factor**: 10
- **Thread Count**: 2
- **Iterations**: 3
- **Use Case**: Fast validation, development debugging

### Development Profile

- **Scale Factor**: 1,000
- **Thread Count**: 4
- **Iterations**: 5
- **Use Case**: Development testing, feature validation

### CI Profile

- **Scale Factor**: 5,000
- **Thread Count**: Half of available cores
- **Iterations**: 10
- **Use Case**: Continuous integration, automated testing

### Performance Profile

- **Scale Factor**: 100,000
- **Thread Count**: All available cores
- **Iterations**: 20
- **Use Case**: Full performance benchmarking

### Stress Profile

- **Scale Factor**: 1,000,000
- **Thread Count**: 2x available cores
- **Iterations**: 50
- **Use Case**: Maximum stress testing, capacity planning

## Command Line Usage

```bash
# Run with default (Quick) profile
./my_benchmark

# Run with specific profile
./my_benchmark --profile ci
./my_benchmark --profile performance

# Run with custom parameters
./my_benchmark --profile development --iterations 10 --detailed

# Run specific category or test
./my_benchmark --category "Database Operations"
./my_benchmark --test "TableInsertBenchmark"

# Output results to file
./my_benchmark --profile ci --json --output results.json
./my_benchmark --profile performance --csv --output results.csv

# Interactive mode (for long-running tests)
./my_benchmark --profile stress --interactive
```

## Advanced Features

### Memory Tracking

Memory tracking is automatically enabled and provides:

- Initial memory usage
- Peak memory usage during test
- Memory delta (difference)
- Memory efficiency metrics

```cpp
BENCHMARK_TEST(MemoryIntensiveBenchmark, "Memory") {
    auto scale = get_scale_factor();

    // The framework automatically tracks memory usage
    std::vector<std::vector<double>> large_data;

    result.set_data_size(scale * sizeof(double) * 1000);

    BenchmarkTimer timer;
    timer.start();

    for (size_t i = 0; i < scale; ++i) {
        large_data.emplace_back(1000, static_cast<double>(i));
    }

    timer.stop();
    result.add_timing(timer.get_elapsed());
}
```

### Concurrent Testing

```cpp
class ConcurrentBenchmark : public BenchmarkTest {
public:
    ConcurrentBenchmark() : BenchmarkTest("ConcurrentBenchmark", "Concurrency") {}

    void run_benchmark(BenchmarkResult& result) override {
        const size_t thread_count = get_thread_count();
        const size_t operations_per_thread = get_scale_factor() / thread_count;

        std::vector<std::future<void>> futures;
        std::atomic<bool> start_flag{false};

        // Launch worker threads
        for (size_t t = 0; t < thread_count; ++t) {
            futures.emplace_back(std::async(std::launch::async, [&, t]() {
                while (!start_flag.load()) std::this_thread::yield();

                // Perform work
                for (size_t i = 0; i < operations_per_thread; ++i) {
                    // Your concurrent operation here
                }
            }));
        }

        BenchmarkTimer timer;
        timer.start();
        start_flag.store(true);

        for (auto& future : futures) {
            future.wait();
        }

        timer.stop();
        result.add_timing(timer.get_elapsed());
        result.add_custom_metric("thread_count", static_cast<double>(thread_count), "threads");
    }
};
```

### Custom Metrics

```cpp
BENCHMARK_SIMPLE(CustomMetricsBenchmark) {
    // ... perform benchmark operations ...

    // Add custom metrics with units
    result.add_custom_metric("cache_hits", cache_hits, "hits");
    result.add_custom_metric("cache_miss_rate", miss_rate, "%");
    result.add_custom_metric("throughput", throughput, "MB/s");
    result.add_custom_metric("efficiency", efficiency_score, "score");
}
```

### Benchmark Suites

```cpp
void create_table_benchmark_suite() {
    BenchmarkSuite suite("Table Operations", "Comprehensive table performance tests");

    suite.create_test<TableInsertBenchmark>();
    suite.create_test<TableQueryBenchmark>();
    suite.create_test<TableIndexBenchmark>();
    suite.create_test<TableConcurrentBenchmark>();

    BenchmarkConfig config;
    config.apply_profile(BenchmarkProfile::Performance);

    suite.run(config);
}
```

## Results and Reporting

### Automatic Metrics

The framework automatically calculates:

- **Operations per second**: Based on operations count and timing
- **Throughput (MB/s)**: Based on data size and timing
- **Latency (ms)**: Average time per operation
- **Statistical measures**: Mean, median, std dev, percentiles
- **Memory efficiency**: Memory usage per operation

### Result Output Formats

**Console Output (Default)**:

```
=== Benchmark Results ===
MyBenchmarkTest (General):
  Operations: 1,000,000
  Duration: 1.234 ms
  Ops/sec: 810,372
  Latency: 1.23 μs
  Memory: 45.2 MB peak (delta: +12.1 MB)
```

**Detailed Output**:

```
MyBenchmarkTest (General):
  Operations: 1,000,000
  Data Size: 8.0 MB

  Timing Statistics:
    Mean: 1.234 ms
    Median: 1.230 ms
    Std Dev: 0.045 ms
    Min: 1.180 ms
    Max: 1.290 ms
    95th %ile: 1.275 ms
    99th %ile: 1.285 ms

  Performance:
    Operations/sec: 810,372
    Throughput: 6.48 MB/s
    Latency: 1.23 μs

  Memory:
    Initial: 33.1 MB
    Peak: 45.2 MB
    Final: 44.8 MB
    Delta: +12.1 MB

  Custom Metrics:
    cache_hits: 95,432 hits
    efficiency: 98.5 score
```

**JSON Output**:

```json
{
  "name": "MyBenchmarkTest",
  "category": "General",
  "operations": 1000000,
  "data_size_bytes": 8388608,
  "timing": {
    "mean_ns": 1234000,
    "median_ns": 1230000,
    "std_dev_ns": 45000,
    "min_ns": 1180000,
    "max_ns": 1290000,
    "p95_ns": 1275000,
    "p99_ns": 1285000
  },
  "performance": {
    "ops_per_sec": 810372,
    "throughput_mbps": 6.48,
    "latency_ms": 0.001234
  },
  "memory": {
    "initial_mb": 33.1,
    "peak_mb": 45.2,
    "final_mb": 44.8,
    "delta_mb": 12.1
  },
  "custom_metrics": {
    "cache_hits": { "value": 95432, "unit": "hits" },
    "efficiency": { "value": 98.5, "unit": "score" }
  }
}
```

## Integration with Build System

Add to your CMakeLists.txt:

```cmake
# Find benchmark runner source files
file(GLOB BENCHMARK_RUNNER_SOURCES src/benchmark_runner.cpp)
file(GLOB BENCHMARK_RUNNER_HEADERS include/benchmark_runner.h)

# Create your benchmark executable
add_executable(my_benchmarks
    examples/benchmark_example.cpp
    ${BENCHMARK_RUNNER_SOURCES}
)

target_include_directories(my_benchmarks PRIVATE include)
target_link_libraries(my_benchmarks crux)

# Add as a test target (optional)
enable_testing()
add_test(NAME benchmark_quick COMMAND my_benchmarks --profile quick)
add_test(NAME benchmark_ci COMMAND my_benchmarks --profile ci)
```

## Best Practices

1. **Use Appropriate Profiles**: Start with Quick for development, use CI for automated testing, Performance for full analysis

2. **Scale Factor Usage**: Always use `get_scale_factor()` for data sizes to ensure proper scaling across profiles

3. **Memory Considerations**: For memory-intensive tests, consider setting data size with `result.set_data_size()` for accurate throughput calculations

4. **Warmup**: Use `BenchmarkTimer::warmup_cpu()` for CPU-intensive tests to ensure consistent results

5. **Custom Metrics**: Add domain-specific metrics that help understand performance characteristics

6. **Categories**: Group related benchmarks using meaningful category names

7. **Concurrent Tests**: Use atomic flags and proper synchronization for concurrent benchmarks

8. **Resource Cleanup**: Use setup/teardown methods for proper resource management

This infrastructure provides everything needed to create robust, scalable benchmark tests that can adapt to different testing scenarios and provide comprehensive performance analysis.
