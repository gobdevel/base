# Base Framework Performance Testing & Benchmarks

This directory contains comprehensive performance testing and benchmarking tools for the Base framework.

## Quick Start

### Building and Running Benchmarks

```bash
# Build the benchmark runner
mkdir -p build/benchmarks
cd build/benchmarks
cmake ../../benchmarks
make

# Run all benchmarks
./benchmark_runner

# Run specific benchmark categories
./benchmark_runner --logger      # Logger performance tests
./benchmark_runner --messaging   # Messaging system benchmarks
./benchmark_runner --config      # Configuration access performance
./benchmark_runner --threads     # Thread management benchmarks
./benchmark_runner --memory      # Memory usage analysis
```

### Using the Existing Test Suite for Performance

```bash
# Run existing performance tests
cd /path/to/base
cmake --build build/Release
./build/Release/tests/test_base --gtest_filter="*Performance*"
```

## Benchmark Categories

### 🚀 Logger Performance

Tests various logging scenarios:

- **Simple String Logging**: Basic log message throughput
- **Complex Formatting**: Performance with multiple format arguments
- **Log Level Filtering**: Overhead of level checks
- **Async vs Sync**: Comparison of logging modes

**Expected Performance**:

- Simple logs: >500K messages/second
- Complex formatting: >200K messages/second

### 📡 Messaging System

Comprehensive messaging performance tests:

- **Message Queue**: Send/receive throughput
- **Cross-Thread**: Inter-thread communication latency
- **Message Priorities**: Priority queue performance
- **Memory Usage**: Message storage efficiency

**Expected Performance**:

- Local queue: >1M messages/second
- Cross-thread: >600K messages/second
- Latency: <10μs average

### ⚙️ Configuration System

Configuration access performance:

- **Value Access**: Speed of configuration lookups
- **Type Conversion**: Performance of type-safe access
- **Cache Efficiency**: Repeated access patterns

**Expected Performance**:

- Config access: >1M operations/second
- First access: <50μs
- Cached access: <1μs

### 🧵 Thread Management

Thread lifecycle and management overhead:

- **Thread Creation**: Startup and teardown time
- **Task Posting**: Overhead of task queuing
- **Context Switching**: Inter-thread communication cost

**Expected Performance**:

- Thread creation: <1ms
- Task posting: <5μs
- Message passing: <20μs

## Advanced Performance Testing

### Custom Benchmarks

Create custom benchmarks by extending the `BenchmarkRunner` class:

```cpp
#include "benchmark_runner.h"

class CustomBenchmark : public BenchmarkRunner {
public:
    void run_custom_test() {
        auto result = run_benchmark("Custom Test", 10000, [](size_t i) {
            // Your test code here
            my_function(i);
        });
        add_result(result);
    }
};
```

### Profiling Integration

For detailed profiling, use external tools:

```bash
# CPU profiling with perf
perf record -g ./benchmark_runner
perf report

# Memory profiling with valgrind
valgrind --tool=massif ./benchmark_runner
ms_print massif.out.* | less

# Address sanitizer for memory errors
export ASAN_OPTIONS=detect_leaks=1
./benchmark_runner_asan
```

### Load Testing

For sustained load testing:

```bash
# Run benchmarks continuously for stability testing
while true; do
    ./benchmark_runner --messaging
    sleep 60
done
```

## Performance Targets

### Framework Baseline

| Component | Metric               | Target | Actual   |
| --------- | -------------------- | ------ | -------- |
| Logger    | Simple logs/sec      | >500K  | Measured |
| Logger    | Complex logs/sec     | >200K  | Measured |
| Messaging | Cross-thread msg/sec | >600K  | Measured |
| Messaging | Avg latency          | <10μs  | Measured |
| Config    | Access/sec           | >1M    | Measured |
| Threads   | Creation time        | <1ms   | Measured |

### System Requirements

**Minimum Requirements:**

- CPU: 2+ cores
- RAM: 4GB available
- Disk: SSD recommended for logging tests

**Recommended for benchmarking:**

- CPU: 8+ cores
- RAM: 16GB available
- Disk: NVMe SSD
- Network: Gigabit (for network tests)

## Interpreting Results

### Key Metrics

- **Throughput**: Operations per second
- **Latency**: Time per operation (average, P95, P99)
- **Memory Usage**: RSS/heap growth during operations
- **CPU Usage**: System load during tests

### Performance Analysis

```bash
# Generate performance report
./benchmark_runner > performance_report.txt

# Compare with baseline (if you have previous results)
diff performance_baseline.csv benchmark_results.csv
```

### Red Flags

Watch for these performance issues:

- **Declining Throughput**: May indicate memory leaks
- **High P99 Latency**: Suggests GC pauses or lock contention
- **Memory Growth**: Potential memory leaks
- **CPU Spikes**: Inefficient algorithms or busy loops

## Continuous Performance Testing

### CI Integration

Add to your CI pipeline:

```yaml
# .github/workflows/performance.yml
- name: Run Performance Tests
  run: |
    cd benchmarks
    mkdir build && cd build
    cmake .. && make
    ./benchmark_runner --messaging > results.txt

- name: Check Performance Regression
  run: |
    python scripts/check_performance_regression.py results.txt baseline.txt
```

### Automated Monitoring

Set up automated performance monitoring:

```bash
# Run benchmarks daily and store results
crontab -e
0 2 * * * cd /path/to/base/benchmarks && ./run_daily_benchmarks.sh
```

## Troubleshooting Performance Issues

### Common Issues

1. **Low Throughput**:

   - Check CPU affinity
   - Verify compiler optimizations (-O3)
   - Monitor system load

2. **High Latency**:

   - Check for lock contention
   - Monitor GC if using managed languages
   - Verify thread pool configuration

3. **Memory Issues**:
   - Run with AddressSanitizer
   - Check for memory leaks with Valgrind
   - Monitor RSS growth

### Debug Builds vs Release

Always benchmark in release mode:

```bash
# Debug builds (slower, for development)
cmake -DCMAKE_BUILD_TYPE=Debug

# Release builds (fast, for benchmarking)
cmake -DCMAKE_BUILD_TYPE=Release
```

## Platform-Specific Notes

### Linux

- Use `perf` for detailed CPU profiling
- `htop` for real-time system monitoring
- `/proc/meminfo` for memory analysis

### macOS

- Use Instruments for profiling
- Activity Monitor for system resources
- `vm_stat` for memory statistics

### Windows

- Use Visual Studio Diagnostic Tools
- PerfView for ETW tracing
- Task Manager for basic monitoring

## Contributing Performance Tests

When adding new benchmarks:

1. Follow the existing naming convention
2. Include both throughput and latency measurements
3. Add appropriate documentation
4. Test on multiple platforms
5. Include baseline performance expectations

## Resources

- [spdlog performance](https://github.com/gabime/spdlog#benchmarks)
- [ASIO performance](https://think-async.com/Asio/asio-1.18.1/doc/asio/overview/core/basics.html)
- [C++ benchmarking best practices](https://github.com/google/benchmark)
- [Linux perf tutorial](https://perf.wiki.kernel.org/index.php/Tutorial)

---

_Last updated: Performance testing guide reflects current framework capabilities and benchmarking best practices._
