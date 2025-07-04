# Table Performance Tuning

## Overview

This guide provides comprehensive performance optimization strategies for the Table system, covering indexing strategies, memory management, concurrency optimization, and benchmarking techniques to achieve maximum performance.

## Table of Contents

1. [Performance Metrics](#performance-metrics)
2. [Indexing Strategies](#indexing-strategies)
3. [Memory Optimization](#memory-optimization)
4. [Query Optimization](#query-optimization)
5. [Concurrency Performance](#concurrency-performance)
6. [Bulk Operations](#bulk-operations)
7. [Benchmarking](#benchmarking)
8. [Performance Monitoring](#performance-monitoring)
9. [Troubleshooting](#troubleshooting)

## Performance Metrics

### Key Performance Indicators

| Operation         | Target Performance     | Measurement             |
| ----------------- | ---------------------- | ----------------------- |
| Insert            | >1000 ops/sec/thread   | Operations per second   |
| Query (indexed)   | <1ms response          | Query execution time    |
| Query (full scan) | 100MB/sec              | Data throughput         |
| Concurrent reads  | Linear scaling         | Ops/sec vs thread count |
| Memory usage      | <64 bytes/row overhead | Memory per row          |
| Index lookup      | <0.1ms                 | Direct access time      |

### Benchmark Table Setup

```cpp
// Create a representative benchmark table
auto create_benchmark_table(size_t row_count) -> std::unique_ptr<Table> {
    auto schema = std::make_unique<TableSchema>("benchmark", 1);

    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("user_id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("category", ColumnType::String, false));
    schema->add_column(ColumnDefinition("value", ColumnType::Double, false));
    schema->add_column(ColumnDefinition("timestamp", ColumnType::DateTime, false));
    schema->add_column(ColumnDefinition("description", ColumnType::String, true));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Create indexes for common access patterns
    table->create_index("user_idx", {"user_id"});
    table->create_index("category_idx", {"category"});
    table->create_index("user_category_idx", {"user_id", "category"});
    table->create_index("timestamp_idx", {"timestamp"});

    // Populate with test data
    populate_test_data(*table, row_count);

    return table;
}
```

## Indexing Strategies

### Index Selection Rules

1. **High Selectivity Columns**: Index columns with many unique values
2. **Frequent Query Columns**: Index columns used in WHERE clauses
3. **Composite Indexes**: Use for multi-column queries
4. **Order Matters**: Most selective column first in composite indexes

### Index Performance Analysis

```cpp
class IndexAnalyzer {
public:
    struct IndexStats {
        std::string name;
        size_t key_count;
        double selectivity;
        double avg_lookup_time;
        size_t memory_usage;
    };

    static std::vector<IndexStats> analyze_indexes(const Table& table) {
        std::vector<IndexStats> stats;

        auto indexes = table.get_indexes();
        for (const auto& [name, index] : indexes) {
            IndexStats stat;
            stat.name = name;
            stat.key_count = index.get_key_count();
            stat.selectivity = calculate_selectivity(table, index);
            stat.avg_lookup_time = benchmark_index_lookup(table, name);
            stat.memory_usage = estimate_index_memory(index);

            stats.push_back(stat);
        }

        return stats;
    }

private:
    static double calculate_selectivity(const Table& table, const TableIndex& index) {
        auto total_rows = table.get_row_count();
        auto unique_keys = index.get_key_count();
        return static_cast<double>(unique_keys) / total_rows;
    }

    static double benchmark_index_lookup(const Table& table, const std::string& index_name) {
        const size_t iterations = 1000;
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            // Perform sample lookups
            auto results = table.find_by_index(index_name, {static_cast<std::int64_t>(i % 100)});
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        return duration.count() / (iterations * 1e6);  // Convert to milliseconds
    }
};
```

### Optimal Index Patterns

```cpp
// Example: E-commerce product catalog optimization
void optimize_product_catalog(Table& products) {
    // High-selectivity single column indexes
    products.create_index("sku_idx", {"sku"});           // Unique product identifier
    products.create_index("category_idx", {"category"}); // Product categories

    // Composite indexes for common query patterns
    // Query: WHERE category = 'X' AND price BETWEEN Y AND Z
    products.create_index("cat_price_idx", {"category", "price"});

    // Query: WHERE available = true AND category = 'X' ORDER BY price
    products.create_index("avail_cat_price_idx", {"available", "category", "price"});

    // Query: WHERE supplier_id = X AND category = 'Y'
    products.create_index("supplier_cat_idx", {"supplier_id", "category"});

    // Don't over-index - avoid indexes that won't be used
    // products.create_index("bad_idx", {"description"});  // Low selectivity, rarely queried
}
```

## Memory Optimization

### Memory Usage Analysis

```cpp
class MemoryProfiler {
public:
    struct MemoryStats {
        size_t total_memory;
        size_t schema_memory;
        size_t row_data_memory;
        size_t index_memory;
        size_t overhead_memory;
        double bytes_per_row;
    };

    static MemoryStats analyze_memory_usage(const Table& table) {
        MemoryStats stats;

        stats.schema_memory = estimate_schema_memory(table.get_schema());
        stats.row_data_memory = estimate_row_data_memory(table);
        stats.index_memory = estimate_index_memory(table);
        stats.overhead_memory = estimate_overhead_memory(table);

        stats.total_memory = stats.schema_memory + stats.row_data_memory +
                           stats.index_memory + stats.overhead_memory;

        auto row_count = table.get_row_count();
        stats.bytes_per_row = row_count > 0 ?
            static_cast<double>(stats.total_memory) / row_count : 0;

        return stats;
    }

    static void print_memory_report(const MemoryStats& stats) {
        std::cout << "Memory Usage Report:\n";
        std::cout << "  Total Memory: " << format_bytes(stats.total_memory) << "\n";
        std::cout << "  Schema: " << format_bytes(stats.schema_memory) << "\n";
        std::cout << "  Row Data: " << format_bytes(stats.row_data_memory) << "\n";
        std::cout << "  Indexes: " << format_bytes(stats.index_memory) << "\n";
        std::cout << "  Overhead: " << format_bytes(stats.overhead_memory) << "\n";
        std::cout << "  Bytes per Row: " << std::fixed << std::setprecision(2)
                  << stats.bytes_per_row << "\n";
    }

private:
    static std::string format_bytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024 && unit < 3) {
            size /= 1024;
            unit++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return oss.str();
    }
};
```

### Memory Optimization Strategies

```cpp
// 1. Efficient data types
void optimize_data_types() {
    auto schema = std::make_unique<TableSchema>("optimized", 1);

    // Use smallest suitable integer type
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));  // 64-bit when needed

    // Consider string length for text fields
    schema->add_column(ColumnDefinition("code", ColumnType::String, false));  // Short codes

    // Use appropriate precision for floating point
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false)); // Full precision

    // Boolean for flags (most memory efficient)
    schema->add_column(ColumnDefinition("active", ColumnType::Boolean, false));
}

// 2. Minimize nullable columns
void minimize_nullables() {
    auto schema = std::make_unique<TableSchema>("efficient", 1);

    // Required columns (no null overhead)
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("status", ColumnType::String, false));

    // Only make columns nullable when truly optional
    schema->add_column(ColumnDefinition("description", ColumnType::String, true));  // Truly optional
}

// 3. Strategic index selection
void optimize_indexes(Table& table) {
    // Only create indexes that will be used
    table.create_index("primary_lookup", {"user_id"});      // High usage
    table.create_index("date_range", {"created_at"});       // Range queries

    // Avoid indexes on low-selectivity columns
    // table.create_index("status_idx", {"status"});        // Only 3 possible values

    // Use composite indexes instead of multiple single-column indexes
    table.create_index("user_date_idx", {"user_id", "created_at"});  // Covers both columns
}
```

## Query Optimization

### Query Performance Patterns

```cpp
class QueryOptimizer {
public:
    // Pattern 1: Use indexes for exact lookups
    static std::vector<TableRow> optimized_user_lookup(const Table& table, std::int64_t user_id) {
        // Fast: Direct index lookup
        return table.find_by_index("user_idx", {user_id});

        // Slow: Full table scan
        // TableQuery query;
        // query.where("user_id", QueryOperator::Equal, user_id);
        // return table.query(query);
    }

    // Pattern 2: Optimize range queries with proper indexing
    static std::vector<TableRow> optimized_date_range(const Table& table,
                                                     const std::string& start_date,
                                                     const std::string& end_date) {
        TableQuery query;
        query.where("timestamp", QueryOperator::GreaterThanOrEqual, start_date)
             .where("timestamp", QueryOperator::LessThanOrEqual, end_date)
             .order_by("timestamp", SortOrder::Ascending);  // Index can help with ordering

        return table.query(query);
    }

    // Pattern 3: Use composite indexes for multi-condition queries
    static std::vector<TableRow> optimized_category_search(const Table& table,
                                                          std::int64_t user_id,
                                                          const std::string& category) {
        // Uses composite index "user_category_idx"
        return table.find_by_index("user_category_idx", {user_id, category});
    }

    // Pattern 4: Limit results to avoid memory pressure
    static std::vector<TableRow> optimized_recent_items(const Table& table, size_t limit = 100) {
        TableQuery query;
        query.order_by("timestamp", SortOrder::Descending)
             .limit(limit);  // Only load what you need

        return table.query(query);
    }
};
```

### Query Performance Benchmarking

```cpp
template<typename Func>
double benchmark_query(Func&& query_func, size_t iterations = 1000) {
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        auto results = query_func();
        // Ensure compiler doesn't optimize away the call
        volatile size_t size = results.size();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    return static_cast<double>(duration.count()) / iterations;  // Average microseconds per query
}

void compare_query_strategies(const Table& table) {
    std::int64_t test_user_id = 1;

    // Benchmark index lookup
    auto index_time = benchmark_query([&]() {
        return table.find_by_index("user_idx", {test_user_id});
    });

    // Benchmark full table scan
    auto scan_time = benchmark_query([&]() {
        TableQuery query;
        query.where("user_id", QueryOperator::Equal, test_user_id);
        return table.query(query);
    });

    std::cout << "Index lookup: " << index_time << " μs\n";
    std::cout << "Full scan: " << scan_time << " μs\n";
    std::cout << "Speedup: " << (scan_time / index_time) << "x\n";
}
```

## Concurrency Performance

### Threading Performance Analysis

```cpp
class ConcurrencyProfiler {
public:
    struct ConcurrencyStats {
        size_t thread_count;
        double ops_per_second;
        double avg_latency_ms;
        double throughput_scaling;
    };

    static std::vector<ConcurrencyStats> benchmark_concurrency(Table& table) {
        std::vector<ConcurrencyStats> results;

        // Test with different thread counts
        for (size_t threads : {1, 2, 4, 8, 16}) {
            auto stats = benchmark_threads(table, threads);
            results.push_back(stats);
        }

        return results;
    }

private:
    static ConcurrencyStats benchmark_threads(Table& table, size_t thread_count) {
        const size_t ops_per_thread = 1000;
        const size_t total_ops = thread_count * ops_per_thread;

        std::vector<std::thread> workers;
        std::vector<double> thread_times(thread_count);

        auto start = std::chrono::high_resolution_clock::now();

        // Launch worker threads
        for (size_t i = 0; i < thread_count; ++i) {
            workers.emplace_back([&, i]() {
                auto thread_start = std::chrono::high_resolution_clock::now();

                // Mix of read and write operations
                for (size_t j = 0; j < ops_per_thread; ++j) {
                    if (j % 4 == 0) {
                        // Write operation (25%)
                        auto txn = table.begin_transaction();
                        txn->insert_row({
                            {"id", static_cast<std::int64_t>(i * ops_per_thread + j)},
                            {"value", static_cast<double>(j)}
                        });
                        txn->commit();
                    } else {
                        // Read operation (75%)
                        auto rows = table.get_all_rows();
                    }
                }

                auto thread_end = std::chrono::high_resolution_clock::now();
                thread_times[i] = std::chrono::duration_cast<std::chrono::microseconds>(
                    thread_end - thread_start).count() / 1000.0;  // Convert to ms
            });
        }

        // Wait for all threads
        for (auto& worker : workers) {
            worker.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();

        ConcurrencyStats stats;
        stats.thread_count = thread_count;
        stats.ops_per_second = (total_ops * 1000.0) / total_time_ms;
        stats.avg_latency_ms = *std::max_element(thread_times.begin(), thread_times.end());
        stats.throughput_scaling = stats.ops_per_second / thread_count;  // Per-thread throughput

        return stats;
    }
};
```

### Lock Contention Optimization

```cpp
class LockOptimizer {
public:
    // Strategy 1: Batch operations to reduce lock frequency
    static void batch_insert_optimization(Table& table,
                                         const std::vector<std::unordered_map<std::string, CellValue>>& rows) {
        // Inefficient: One transaction per row
        // for (const auto& row : rows) {
        //     auto txn = table.begin_transaction();
        //     txn->insert_row(row);
        //     txn->commit();
        // }

        // Efficient: Batch rows in single transaction
        const size_t batch_size = 1000;
        for (size_t i = 0; i < rows.size(); i += batch_size) {
            auto txn = table.begin_transaction();

            size_t end = std::min(i + batch_size, rows.size());
            for (size_t j = i; j < end; ++j) {
                txn->insert_row(rows[j]);
            }

            txn->commit();
        }
    }

    // Strategy 2: Separate read-heavy workloads
    static void read_optimized_pattern(const Table& table) {
        // Prefer direct reads over transactions for read-only operations
        auto rows = table.get_all_rows();
        auto row_count = table.get_row_count();

        TableQuery query;
        query.where("active", QueryOperator::Equal, true);
        auto active_rows = table.query(query);

        // No locks held longer than necessary
    }

    // Strategy 3: Minimize transaction scope
    static void minimal_transaction_scope(Table& table) {
        // Prepare data outside transaction
        auto data = prepare_row_data();

        // Minimal transaction scope
        {
            auto txn = table.begin_transaction();
            txn->insert_row(data);
            txn->commit();
        }

        // Do post-processing outside transaction
        process_after_insert(data);
    }
};
```

## Bulk Operations

### High-Performance Bulk Insert

```cpp
class BulkOperations {
public:
    static void optimized_bulk_insert(Table& table,
                                    const std::vector<std::unordered_map<std::string, CellValue>>& data) {
        const size_t optimal_batch_size = calculate_optimal_batch_size(data.size());

        for (size_t i = 0; i < data.size(); i += optimal_batch_size) {
            auto txn = table.begin_transaction();

            size_t end = std::min(i + optimal_batch_size, data.size());
            for (size_t j = i; j < end; ++j) {
                txn->insert_row(data[j]);
            }

            txn->commit();

            // Optional progress reporting
            if ((i / optimal_batch_size) % 10 == 0) {
                std::cout << "Inserted " << (i + optimal_batch_size) << "/" << data.size() << " rows\n";
            }
        }
    }

    static void parallel_bulk_insert(Table& table,
                                   const std::vector<std::unordered_map<std::string, CellValue>>& data,
                                   size_t thread_count = std::thread::hardware_concurrency()) {
        const size_t chunk_size = data.size() / thread_count;
        std::vector<std::thread> workers;
        std::atomic<size_t> completed_chunks{0};

        for (size_t t = 0; t < thread_count; ++t) {
            size_t start = t * chunk_size;
            size_t end = (t == thread_count - 1) ? data.size() : (t + 1) * chunk_size;

            workers.emplace_back([&, start, end]() {
                std::vector<std::unordered_map<std::string, CellValue>> chunk(
                    data.begin() + start, data.begin() + end
                );

                optimized_bulk_insert(table, chunk);
                completed_chunks++;

                std::cout << "Thread " << std::this_thread::get_id()
                         << " completed chunk " << completed_chunks << "/" << thread_count << "\n";
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }
    }

private:
    static size_t calculate_optimal_batch_size(size_t total_rows) {
        // Balance between transaction overhead and memory usage
        if (total_rows < 1000) return total_rows;
        if (total_rows < 10000) return 500;
        if (total_rows < 100000) return 1000;
        return 2000;  // For very large datasets
    }
};
```

## Benchmarking

### Comprehensive Performance Test Suite

```cpp
class PerformanceTestSuite {
public:
    struct BenchmarkResults {
        double insert_ops_per_sec;
        double query_ops_per_sec;
        double index_lookup_avg_ms;
        double full_scan_avg_ms;
        double memory_mb_per_1k_rows;
        double concurrent_read_scaling;
    };

    static BenchmarkResults run_full_benchmark() {
        BenchmarkResults results;

        // Create test table
        auto table = create_benchmark_table(10000);

        // Test 1: Insert performance
        results.insert_ops_per_sec = benchmark_insert_performance(*table);

        // Test 2: Query performance
        results.query_ops_per_sec = benchmark_query_performance(*table);

        // Test 3: Index lookup performance
        results.index_lookup_avg_ms = benchmark_index_lookup_performance(*table);

        // Test 4: Full scan performance
        results.full_scan_avg_ms = benchmark_full_scan_performance(*table);

        // Test 5: Memory efficiency
        results.memory_mb_per_1k_rows = benchmark_memory_efficiency(*table);

        // Test 6: Concurrent read scaling
        results.concurrent_read_scaling = benchmark_concurrent_reads(*table);

        return results;
    }

    static void print_benchmark_report(const BenchmarkResults& results) {
        std::cout << "\n=== Performance Benchmark Report ===\n";
        std::cout << "Insert Performance: " << std::fixed << std::setprecision(0)
                  << results.insert_ops_per_sec << " ops/sec\n";
        std::cout << "Query Performance: " << results.query_ops_per_sec << " ops/sec\n";
        std::cout << "Index Lookup: " << std::setprecision(3)
                  << results.index_lookup_avg_ms << " ms avg\n";
        std::cout << "Full Scan: " << results.full_scan_avg_ms << " ms avg\n";
        std::cout << "Memory Usage: " << std::setprecision(2)
                  << results.memory_mb_per_1k_rows << " MB per 1K rows\n";
        std::cout << "Concurrent Read Scaling: " << std::setprecision(1)
                  << results.concurrent_read_scaling << "x\n";
        std::cout << "================================\n\n";

        // Performance recommendations
        print_recommendations(results);
    }

private:
    static double benchmark_insert_performance(Table& table) {
        const size_t num_inserts = 1000;
        auto start = std::chrono::high_resolution_clock::now();

        auto txn = table.begin_transaction();
        for (size_t i = 0; i < num_inserts; ++i) {
            txn->insert_row({
                {"id", static_cast<std::int64_t>(i + 10000)},
                {"value", static_cast<double>(i)}
            });
        }
        txn->commit();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        return (num_inserts * 1000.0) / duration_ms;  // ops/sec
    }

    static void print_recommendations(const BenchmarkResults& results) {
        std::cout << "Performance Recommendations:\n";

        if (results.insert_ops_per_sec < 500) {
            std::cout << "- Consider batching insert operations\n";
            std::cout << "- Reduce number of indexes during bulk loads\n";
        }

        if (results.index_lookup_avg_ms > 1.0) {
            std::cout << "- Review index selectivity\n";
            std::cout << "- Consider composite indexes for multi-column queries\n";
        }

        if (results.memory_mb_per_1k_rows > 1.0) {
            std::cout << "- Optimize data types for smaller memory footprint\n";
            std::cout << "- Reduce nullable columns\n";
        }

        if (results.concurrent_read_scaling < 2.0) {
            std::cout << "- Check for lock contention in read operations\n";
            std::cout << "- Consider read-optimized access patterns\n";
        }

        std::cout << "\n";
    }
};
```

### Continuous Performance Monitoring

```cpp
class PerformanceMonitor {
private:
    mutable std::mutex stats_mutex_;
    std::unordered_map<std::string, std::vector<double>> operation_times_;
    std::chrono::steady_clock::time_point start_time_;

public:
    PerformanceMonitor() : start_time_(std::chrono::steady_clock::now()) {}

    void record_operation(const std::string& operation_name, double duration_ms) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        operation_times_[operation_name].push_back(duration_ms);
    }

    void print_statistics() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);

        std::cout << "\n=== Performance Statistics ===\n";
        for (const auto& [operation, times] : operation_times_) {
            if (times.empty()) continue;

            double sum = std::accumulate(times.begin(), times.end(), 0.0);
            double avg = sum / times.size();

            auto minmax = std::minmax_element(times.begin(), times.end());
            double min_time = *minmax.first;
            double max_time = *minmax.second;

            std::cout << operation << ":\n";
            std::cout << "  Count: " << times.size() << "\n";
            std::cout << "  Average: " << std::fixed << std::setprecision(3) << avg << " ms\n";
            std::cout << "  Min: " << min_time << " ms\n";
            std::cout << "  Max: " << max_time << " ms\n";
            std::cout << "  Total: " << sum << " ms\n\n";
        }

        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        std::cout << "Monitoring Duration: " << uptime.count() << " seconds\n";
        std::cout << "==============================\n\n";
    }
};

// Usage with RAII timing
class ScopedTimer {
private:
    PerformanceMonitor* monitor_;
    std::string operation_name_;
    std::chrono::steady_clock::time_point start_;

public:
    ScopedTimer(PerformanceMonitor* monitor, const std::string& operation)
        : monitor_(monitor), operation_name_(operation),
          start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        monitor_->record_operation(operation_name_, duration.count() / 1000.0);
    }
};

#define PERF_TIMER(monitor, operation) ScopedTimer timer(monitor, operation)
```

## Troubleshooting

### Common Performance Issues

```cpp
class PerformanceTroubleshooter {
public:
    static void diagnose_slow_queries(const Table& table) {
        std::cout << "=== Query Performance Diagnosis ===\n";

        // Check for missing indexes
        auto schema = table.get_schema();
        auto indexes = table.get_indexes();

        std::cout << "Available indexes:\n";
        for (const auto& [name, index] : indexes) {
            std::cout << "  " << name << ": " << format_columns(index.get_columns()) << "\n";
        }

        // Suggest common indexes
        std::cout << "\nRecommended indexes for common patterns:\n";
        for (const auto& column : schema.get_columns()) {
            if (column.get_type() == ColumnType::Integer ||
                column.get_type() == ColumnType::String) {
                std::cout << "  Consider: CREATE INDEX " << column.get_name()
                         << "_idx ON (" << column.get_name() << ")\n";
            }
        }
    }

    static void diagnose_memory_usage(const Table& table) {
        std::cout << "=== Memory Usage Diagnosis ===\n";

        auto stats = MemoryProfiler::analyze_memory_usage(table);
        MemoryProfiler::print_memory_report(stats);

        // Memory optimization suggestions
        if (stats.bytes_per_row > 100) {
            std::cout << "\nMemory optimization suggestions:\n";
            std::cout << "- Review data types for efficiency\n";
            std::cout << "- Minimize nullable columns\n";
            std::cout << "- Consider data archiving for old records\n";
        }

        if (stats.index_memory > stats.row_data_memory) {
            std::cout << "- Index memory exceeds data memory\n";
            std::cout << "- Review index necessity and selectivity\n";
        }
    }

    static void diagnose_concurrency_issues(Table& table) {
        std::cout << "=== Concurrency Diagnosis ===\n";

        auto concurrency_stats = ConcurrencyProfiler::benchmark_concurrency(table);

        std::cout << "Thread scaling performance:\n";
        double baseline_throughput = 0;

        for (const auto& stats : concurrency_stats) {
            if (stats.thread_count == 1) {
                baseline_throughput = stats.ops_per_second;
            }

            double scaling_efficiency = baseline_throughput > 0 ?
                (stats.ops_per_second / baseline_throughput) / stats.thread_count : 0;

            std::cout << "  " << stats.thread_count << " threads: "
                     << std::fixed << std::setprecision(0) << stats.ops_per_second
                     << " ops/sec (efficiency: " << std::setprecision(1)
                     << (scaling_efficiency * 100) << "%)\n";
        }

        // Concurrency recommendations
        if (concurrency_stats.size() > 1) {
            auto last_stats = concurrency_stats.back();
            double efficiency = (last_stats.ops_per_second / baseline_throughput) / last_stats.thread_count;

            if (efficiency < 0.5) {
                std::cout << "\nConcurrency issues detected:\n";
                std::cout << "- High lock contention\n";
                std::cout << "- Consider shorter transactions\n";
                std::cout << "- Batch operations where possible\n";
            }
        }
    }

private:
    static std::string format_columns(const std::vector<std::string>& columns) {
        std::ostringstream oss;
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << columns[i];
        }
        return oss.str();
    }
};
```

### Performance Regression Testing

```cpp
class RegressionTester {
public:
    struct BaselineMetrics {
        double insert_baseline;
        double query_baseline;
        double memory_baseline;
    };

    static BaselineMetrics establish_baseline(const std::string& version) {
        std::cout << "Establishing performance baseline for version " << version << "\n";

        auto results = PerformanceTestSuite::run_full_benchmark();

        BaselineMetrics baseline;
        baseline.insert_baseline = results.insert_ops_per_sec;
        baseline.query_baseline = results.query_ops_per_sec;
        baseline.memory_baseline = results.memory_mb_per_1k_rows;

        // Save baseline to file
        save_baseline(version, baseline);

        return baseline;
    }

    static bool check_for_regressions(const BaselineMetrics& baseline,
                                    double tolerance = 0.1) {  // 10% tolerance
        auto current_results = PerformanceTestSuite::run_full_benchmark();

        bool has_regression = false;

        // Check insert performance
        double insert_change = (baseline.insert_baseline - current_results.insert_ops_per_sec) /
                              baseline.insert_baseline;
        if (insert_change > tolerance) {
            std::cout << "REGRESSION: Insert performance decreased by "
                     << (insert_change * 100) << "%\n";
            has_regression = true;
        }

        // Check query performance
        double query_change = (baseline.query_baseline - current_results.query_ops_per_sec) /
                             baseline.query_baseline;
        if (query_change > tolerance) {
            std::cout << "REGRESSION: Query performance decreased by "
                     << (query_change * 100) << "%\n";
            has_regression = true;
        }

        // Check memory usage (increase is bad)
        double memory_change = (current_results.memory_mb_per_1k_rows - baseline.memory_baseline) /
                              baseline.memory_baseline;
        if (memory_change > tolerance) {
            std::cout << "REGRESSION: Memory usage increased by "
                     << (memory_change * 100) << "%\n";
            has_regression = true;
        }

        if (!has_regression) {
            std::cout << "No performance regressions detected\n";
        }

        return !has_regression;
    }

private:
    static void save_baseline(const std::string& version, const BaselineMetrics& baseline) {
        nlohmann::json baseline_data;
        baseline_data["version"] = version;
        baseline_data["timestamp"] = current_timestamp();
        baseline_data["insert_baseline"] = baseline.insert_baseline;
        baseline_data["query_baseline"] = baseline.query_baseline;
        baseline_data["memory_baseline"] = baseline.memory_baseline;

        std::ofstream file("performance_baseline_" + version + ".json");
        file << baseline_data.dump(2);
    }
};
```

---

_See also: [Table API Reference](Tables-API.md), [Indexing Guide](Tables-Queries.md), [Best Practices](Tables-BestPractices.md)_
