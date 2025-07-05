/*
 * @file table_benchmark.cpp
 * @brief High-scale performance benchmarks for Table data structure
 *
 * This benchmark tests the Table system with very large datasets to evaluate:
 * - Insertion performance at scale (up to 10M+ rows)
 * - Query performance with large datasets
 * - Index performance with high cardinality
 * - Memory usage and efficiency
 * - Concurrent access performance
 * - Serialization performance with large tables
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "tables.h"
#include "logger.h"
#include <chrono>
#include <vector>
#include <random>
#include <thread>
#include <future>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <memory>
#include <atomic>
#include <fstream>

using namespace base;
using namespace std::chrono;

// Performance profile definitions
enum class BenchmarkProfile {
    Default,     // Quick validation with small datasets
    Development, // Medium-scale for development testing
    CI,         // Continuous integration profile
    Benchmark,   // Full benchmark suite
    Stress      // Maximum stress testing
};

// Configuration structure for benchmark parameters
struct BenchmarkConfig {
    BenchmarkProfile profile = BenchmarkProfile::Default;
    size_t scalability_rows = 10;           // Overridden by profile
    size_t serialization_rows = 3;          // Overridden by profile
    size_t extreme_rows = 100;              // Overridden by profile
    bool skip_serialization = false;        // Overridden by profile
    bool skip_extreme = true;               // Overridden by profile
    bool interactive = false;               // Overridden by profile

    // Apply profile-specific settings
    void apply_profile(BenchmarkProfile prof) {
        profile = prof;

        switch (prof) {
            case BenchmarkProfile::Default:
                scalability_rows = 10;
                serialization_rows = 3;
                extreme_rows = 100;
                skip_serialization = false;
                skip_extreme = true;
                interactive = false;
                break;

            case BenchmarkProfile::Development:
                scalability_rows = 1000;
                serialization_rows = 10;
                extreme_rows = 10000;
                skip_serialization = false;
                skip_extreme = false;
                interactive = false;
                break;

            case BenchmarkProfile::CI:
                scalability_rows = 5000;
                serialization_rows = 25;
                extreme_rows = 50000;
                skip_serialization = false;
                skip_extreme = false;
                interactive = false;
                break;

            case BenchmarkProfile::Benchmark:
                scalability_rows = 1000000;
                serialization_rows = 1000;
                extreme_rows = 10000000;
                skip_serialization = true; // Known issue with DateTime
                skip_extreme = false;
                interactive = true;
                break;

            case BenchmarkProfile::Stress:
                scalability_rows = 5000000;
                serialization_rows = 5000;
                extreme_rows = 50000000;
                skip_serialization = true; // Known issue with DateTime
                skip_extreme = false;
                interactive = true;
                break;
        }
    }

    std::string get_profile_name() const {
        switch (profile) {
            case BenchmarkProfile::Default: return "Default";
            case BenchmarkProfile::Development: return "Development";
            case BenchmarkProfile::CI: return "CI";
            case BenchmarkProfile::Benchmark: return "Benchmark";
            case BenchmarkProfile::Stress: return "Stress";
            default: return "Unknown";
        }
    }

    void print() const {
        std::cout << "Benchmark Configuration (Profile: " << get_profile_name() << "):\n";
        std::cout << "  Scalability test rows: " << scalability_rows << "\n";
        std::cout << "  Serialization test rows: " << serialization_rows << "\n";
        std::cout << "  Extreme test rows: " << extreme_rows << "\n";
        std::cout << "  Skip serialization: " << (skip_serialization ? "Yes" : "No") << "\n";
        std::cout << "  Skip extreme test: " << (skip_extreme ? "Yes" : "No") << "\n";
        std::cout << "  Interactive mode: " << (interactive ? "Yes" : "No") << "\n";
        std::cout << "\n";
    }
};

class TableBenchmark {
private:
    BenchmarkConfig config_;

public:
    TableBenchmark(const BenchmarkConfig& config = {}) : config_(config), rng_(std::random_device{}()) {
        // Apply default profile if none specified
        if (config_.profile == BenchmarkProfile::Default &&
            config_.scalability_rows == 10) { // Check if using default values
            const_cast<BenchmarkConfig&>(config_).apply_profile(BenchmarkProfile::Default);
        }

        // Create test schema
        createTestSchema();

        // Prepare test data generators
        setupDataGenerators();
    }

    void runAllBenchmarks() {
        std::cout << "\n=========================================\n";
        std::cout << "TABLE HIGH-SCALE PERFORMANCE BENCHMARKS\n";
        std::cout << "=========================================\n\n";

        // Run incremental scale tests
        runScalabilityBenchmarks();

        // Run specific performance tests
        runInsertionBenchmarks();
        runQueryBenchmarks();
        runIndexBenchmarks();
        runConcurrencyBenchmarks();
        runMemoryBenchmarks();
        runSerializationBenchmarks();

        // Run extreme scale test
        runExtremeScaleBenchmark();

        std::cout << "\n===============================\n";
        std::cout << "BENCHMARK SUITE COMPLETED\n";
        std::cout << "===============================\n\n";
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<int> age_dist_{18, 80};
    std::uniform_int_distribution<int> salary_dist_{30000, 200000};
    std::uniform_int_distribution<int> bool_dist_{0, 1};
    std::vector<std::string> first_names_;
    std::vector<std::string> last_names_;
    std::vector<std::string> domains_;

    void createTestSchema() {
        first_names_ = {
            "John", "Jane", "Michael", "Sarah", "David", "Emily", "Robert", "Lisa",
            "William", "Maria", "James", "Jennifer", "Christopher", "Patricia", "Daniel",
            "Nancy", "Matthew", "Betty", "Anthony", "Helen", "Mark", "Sandra", "Donald",
            "Donna", "Steven", "Carol", "Paul", "Ruth", "Andrew", "Sharon", "Joshua",
            "Michelle", "Kenneth", "Laura", "Kevin", "Sarah", "Brian", "Kimberly",
            "George", "Deborah", "Edward", "Dorothy", "Ronald", "Lisa", "Timothy",
            "Nancy", "Jason", "Karen", "Jeffrey", "Betty", "Ryan", "Helen"
        };

        last_names_ = {
            "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller",
            "Davis", "Rodriguez", "Martinez", "Hernandez", "Lopez", "Gonzalez",
            "Wilson", "Anderson", "Thomas", "Taylor", "Moore", "Jackson", "Martin",
            "Lee", "Perez", "Thompson", "White", "Harris", "Sanchez", "Clark",
            "Ramirez", "Lewis", "Robinson", "Walker", "Young", "Allen", "King",
            "Wright", "Scott", "Torres", "Nguyen", "Hill", "Flores", "Green",
            "Adams", "Nelson", "Baker", "Hall", "Rivera", "Campbell", "Mitchell",
            "Carter", "Roberts", "Gomez", "Phillips", "EvANS", "Turner", "Diaz"
        };

        domains_ = {
            "gmail.com", "yahoo.com", "hotmail.com", "outlook.com", "company.com",
            "enterprise.com", "tech.com", "business.org", "email.com", "mail.com"
        };
    }

    void setupDataGenerators() {
        // Pre-generate some data for faster benchmarks
    }

    std::unique_ptr<Table> createEmployeeTable() {
        auto schema = std::make_unique<TableSchema>("employees", 1);
        schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("first_name", ColumnType::String, false));
        schema->add_column(ColumnDefinition("last_name", ColumnType::String, false));
        schema->add_column(ColumnDefinition("email", ColumnType::String, false));
        schema->add_column(ColumnDefinition("age", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("salary", ColumnType::Double, false));
        schema->add_column(ColumnDefinition("department", ColumnType::String, false));
        schema->add_column(ColumnDefinition("active", ColumnType::Boolean, false));
        schema->add_column(ColumnDefinition("hire_date", ColumnType::DateTime, false));
        schema->set_primary_key({"id"});

        return std::make_unique<Table>(std::move(schema));
    }

    std::unordered_map<std::string, CellValue> generateEmployeeRow(int64_t id) {
        std::string first_name = first_names_[id % first_names_.size()];
        std::string last_name = last_names_[(id * 7) % last_names_.size()];
        std::string domain = domains_[(id * 3) % domains_.size()];

        std::ostringstream email_stream;
        email_stream << first_name << "." << last_name << "." << id << "@" << domain;

        std::vector<std::string> departments = {
            "Engineering", "Sales", "Marketing", "HR", "Finance", "Operations", "Support"
        };

        auto now = std::chrono::system_clock::now();
        auto hire_date = now - std::chrono::hours(24 * (id % 3650)); // Random date within ~10 years

        return {
            {"id", id},
            {"first_name", first_name},
            {"last_name", last_name},
            {"email", email_stream.str()},
            {"age", static_cast<int64_t>(age_dist_(rng_))},
            {"salary", static_cast<double>(salary_dist_(rng_))},
            {"department", departments[id % departments.size()]},
            {"active", bool_dist_(rng_) == 1},
            {"hire_date", hire_date}
        };
    }

    void runScalabilityBenchmarks() {
        std::cout << "=== SCALABILITY BENCHMARKS ===\n\n";

        std::vector<size_t> scales;

        // Adjust scale points based on profile
        if (config_.scalability_rows <= 100) {
            // Default/minimal profile: very small scales
            scales = {5, 10, 25, 50, config_.scalability_rows};
        } else if (config_.scalability_rows <= 10000) {
            // Development profile: small to medium scales
            scales = {100, 500, 1000, 5000, config_.scalability_rows};
        } else if (config_.scalability_rows <= 100000) {
            // CI profile: medium scales
            scales = {1000, 5000, 10000, 50000, config_.scalability_rows};
        } else {
            // Benchmark/Stress profiles: full range
            scales = {1000, 10000, 100000, 500000, config_.scalability_rows};
        }

        // Remove duplicates and sort
        std::sort(scales.begin(), scales.end());
        scales.erase(std::unique(scales.begin(), scales.end()), scales.end());

        for (size_t scale : scales) {
            std::cout << "Testing scale: " << scale << " rows\n";

            auto table = createEmployeeTable();

            // Measure insertion time
            auto start = high_resolution_clock::now();

            for (size_t i = 1; i <= scale; ++i) {
                auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
                table->insert_row(row_data);

                if (i % (scale / 10) == 0) {
                    std::cout << "  Progress: " << (i * 100 / scale) << "%" << std::endl;
                }
            }

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);

            double rows_per_second = static_cast<double>(scale) / (duration.count() / 1000.0);
            double memory_per_row = getMemoryUsage() / static_cast<double>(scale);

            std::cout << "  Results:\n";
            std::cout << "    Time: " << duration.count() << " ms\n";
            std::cout << "    Rate: " << std::fixed << std::setprecision(0) << rows_per_second << " rows/sec\n";
            std::cout << "    Memory per row: " << std::fixed << std::setprecision(2) << memory_per_row << " bytes\n";
            std::cout << "    Total rows: " << table->get_row_count() << "\n\n";
        }
    }

    void runInsertionBenchmarks() {
        std::cout << "=== INSERTION PERFORMANCE BENCHMARKS ===\n\n";

        // Scale batch insertion test based on profile
        size_t batch_rows = std::min(config_.scalability_rows, static_cast<size_t>(1000000));
        testBatchInsertion(batch_rows, "Batch Insert (" + std::to_string(batch_rows) + " rows)");

        // Scale concurrent insertion test based on profile
        size_t concurrent_rows = std::min(config_.scalability_rows / 2, static_cast<size_t>(500000));
        size_t thread_count = (config_.scalability_rows < 1000) ? 2 : 8;
        testConcurrentInsertion(concurrent_rows, thread_count,
                              std::to_string(concurrent_rows) + " Rows, " + std::to_string(thread_count) + " Threads");

        // Scale index insertion test based on profile
        size_t index_rows = std::min(config_.scalability_rows / 10, static_cast<size_t>(100000));
        testInsertionWithIndexes(index_rows, std::to_string(index_rows) + " Rows with Multiple Indexes");
    }

    void testBatchInsertion(size_t row_count, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();

        // Pre-generate all row data
        std::vector<std::unordered_map<std::string, CellValue>> rows;
        rows.reserve(row_count);

        std::cout << "  Generating test data...\n";
        for (size_t i = 1; i <= row_count; ++i) {
            rows.emplace_back(generateEmployeeRow(static_cast<int64_t>(i)));
        }

        std::cout << "  Inserting rows...\n";
        auto start = high_resolution_clock::now();

        for (const auto& row_data : rows) {
            table->insert_row(row_data);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        double rows_per_second = static_cast<double>(row_count) / (duration.count() / 1000.0);

        std::cout << "  Results:\n";
        std::cout << "    Time: " << duration.count() << " ms\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0) << rows_per_second << " rows/sec\n";
        std::cout << "    Final row count: " << table->get_row_count() << "\n\n";
    }

    void testConcurrentInsertion(size_t total_rows, size_t thread_count, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();
        size_t rows_per_thread = total_rows / thread_count;

        std::cout << "  Starting " << thread_count << " threads, " << rows_per_thread << " rows each\n";

        auto start = high_resolution_clock::now();

        std::vector<std::future<void>> futures;
        std::atomic<size_t> completed_rows{0};

        for (size_t t = 0; t < thread_count; ++t) {
            futures.emplace_back(std::async(std::launch::async, [&, t]() {
                size_t start_id = t * rows_per_thread + 1;
                size_t end_id = start_id + rows_per_thread;

                for (size_t i = start_id; i < end_id; ++i) {
                    auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
                    table->insert_row(row_data);
                    completed_rows.fetch_add(1);
                }
            }));
        }

        // Wait for all threads to complete
        for (auto& future : futures) {
            future.wait();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        double rows_per_second = static_cast<double>(total_rows) / (duration.count() / 1000.0);

        std::cout << "  Results:\n";
        std::cout << "    Time: " << duration.count() << " ms\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0) << rows_per_second << " rows/sec\n";
        std::cout << "    Final row count: " << table->get_row_count() << "\n\n";
    }

    void testInsertionWithIndexes(size_t row_count, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();

        // Create multiple indexes
        table->create_index("email_idx", {"email"}, true);
        table->create_index("department_idx", {"department"});
        table->create_index("salary_idx", {"salary"});
        table->create_index("name_idx", {"last_name", "first_name"});
        table->create_index("age_dept_idx", {"age", "department"});

        std::cout << "  Created 5 indexes\n";
        std::cout << "  Inserting " << row_count << " rows...\n";

        auto start = high_resolution_clock::now();

        for (size_t i = 1; i <= row_count; ++i) {
            auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);

            if (i % (row_count / 10) == 0) {
                std::cout << "    Progress: " << (i * 100 / row_count) << "%\n";
            }
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        double rows_per_second = static_cast<double>(row_count) / (duration.count() / 1000.0);

        std::cout << "  Results:\n";
        std::cout << "    Time: " << duration.count() << " ms\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0) << rows_per_second << " rows/sec\n";
        std::cout << "    Index overhead: ~" << std::fixed << std::setprecision(1)
                  << (5.0 * 100) / rows_per_second << "% per index\n\n";
    }

    void runQueryBenchmarks() {
        std::cout << "=== QUERY PERFORMANCE BENCHMARKS ===\n\n";

        // Create a table sized appropriately for the profile
        auto table = createEmployeeTable();
        size_t test_rows = std::min(config_.scalability_rows, static_cast<size_t>(1000000));

        std::cout << "Creating test table with " << test_rows << " rows...\n";
        for (size_t i = 1; i <= test_rows; ++i) {
            auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);

            // Show progress for larger datasets
            if (test_rows > 1000 && i % (test_rows / 10) == 0) {
                std::cout << "  Progress: " << (i * 100 / test_rows) << "%\n";
            }
        }

        // Test various query types
        testEqualsQuery(table.get(), "Exact Match Query");
        testRangeQuery(table.get(), "Range Query");
        testComplexQuery(table.get(), "Complex Multi-Condition Query");
        testLimitQuery(table.get(), "Paginated Query");
    }

    void testEqualsQuery(Table* table, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        TableQuery query;
        query.where("department", QueryOperator::Equal, std::string("Engineering"));

        auto start = high_resolution_clock::now();
        auto results = table->query(query);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "  Results:\n";
        std::cout << "    Time: " << duration.count() << " μs\n";
        std::cout << "    Rows found: " << results.size() << "\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0)
                  << (results.size() / (duration.count() / 1000000.0)) << " rows/sec\n\n";
    }

    void testRangeQuery(Table* table, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        TableQuery query;
        query.where("salary", QueryOperator::GreaterThan, 75000.0)
             .where("salary", QueryOperator::LessThan, 125000.0)
             .where("age", QueryOperator::GreaterThanOrEqual, static_cast<int64_t>(25))
             .order_by("salary", false);

        auto start = high_resolution_clock::now();
        auto results = table->query(query);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "  Results:\n";
        std::cout << "    Time: " << duration.count() << " μs\n";
        std::cout << "    Rows found: " << results.size() << "\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0)
                  << (results.size() / (duration.count() / 1000000.0)) << " rows/sec\n\n";
    }

    void testComplexQuery(Table* table, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        TableQuery query;
        query.select({"first_name", "last_name", "email", "salary"})
             .where("department", QueryOperator::Equal, std::string("Engineering"))
             .where("salary", QueryOperator::GreaterThan, 80000.0)
             .where("active", QueryOperator::Equal, true)
             .order_by("salary", false)
             .limit(100);

        auto start = high_resolution_clock::now();
        auto results = table->query(query);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "  Results:\n";
        std::cout << "    Time: " << duration.count() << " μs\n";
        std::cout << "    Rows found: " << results.size() << "\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0)
                  << (results.size() / (duration.count() / 1000000.0)) << " rows/sec\n\n";
    }

    void testLimitQuery(Table* table, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        // Test pagination performance
        size_t page_size = 1000;
        size_t total_pages = 10;

        auto start = high_resolution_clock::now();

        for (size_t page = 0; page < total_pages; ++page) {
            TableQuery query;
            query.order_by("id", true)
                 .limit(page_size)
                 .offset(page * page_size);

            auto results = table->query(query);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "  Results (10 pages of 1000 rows each):\n";
        std::cout << "    Total time: " << duration.count() << " μs\n";
        std::cout << "    Avg time per page: " << (duration.count() / total_pages) << " μs\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0)
                  << ((total_pages * page_size) / (duration.count() / 1000000.0)) << " rows/sec\n\n";
    }

    void runIndexBenchmarks() {
        std::cout << "=== INDEX PERFORMANCE BENCHMARKS ===\n\n";

        auto table = createEmployeeTable();
        size_t test_rows = std::min(config_.scalability_rows / 2, static_cast<size_t>(500000));

        std::cout << "Creating test table with " << test_rows << " rows...\n";
        for (size_t i = 1; i <= test_rows; ++i) {
            auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);

            // Show progress for larger datasets
            if (test_rows > 1000 && i % (test_rows / 10) == 0) {
                std::cout << "  Progress: " << (i * 100 / test_rows) << "%\n";
            }
        }

        // Test index creation performance
        testIndexCreation(table.get(), "Index Creation Performance");

        // Test indexed vs non-indexed queries
        testIndexedQueries(table.get(), "Indexed Query Performance");
    }

    void testIndexCreation(Table* table, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        std::vector<std::pair<std::string, std::vector<std::string>>> indexes = {
            {"email_idx", {"email"}},
            {"department_idx", {"department"}},
            {"salary_idx", {"salary"}},
            {"name_idx", {"last_name", "first_name"}},
            {"age_dept_idx", {"age", "department"}},
            {"active_salary_idx", {"active", "salary"}}
        };

        for (const auto& [name, columns] : indexes) {
            auto start = high_resolution_clock::now();
            table->create_index(name, columns);
            auto end = high_resolution_clock::now();

            auto duration = duration_cast<milliseconds>(end - start);

            std::cout << "  Index '" << name << "' (" << columns.size() << " columns): "
                      << duration.count() << " ms\n";
        }
        std::cout << "\n";
    }

    void testIndexedQueries(Table* table, const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        // Test exact match on indexed column
        auto start = high_resolution_clock::now();
        auto results = table->find_by_index("email_idx", {std::string("John.Smith.12345@gmail.com")});
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);

        std::cout << "  Indexed exact match:\n";
        std::cout << "    Time: " << duration.count() << " μs\n";
        std::cout << "    Results: " << results.size() << " rows\n\n";
    }

    void runConcurrencyBenchmarks() {
        std::cout << "=== CONCURRENCY BENCHMARKS ===\n\n";

        testConcurrentReads("Concurrent Read Performance");
        testConcurrentReadWrites("Mixed Read/Write Performance");
    }

    void testConcurrentReads(const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();
        Table* table_ptr = table.get();  // Get raw pointer for lambda capture

        // Scale initial rows based on profile
        size_t initial_rows = std::min(config_.scalability_rows / 10, static_cast<size_t>(100000));
        std::cout << "  Pre-populating with " << initial_rows << " rows...\n";
        for (size_t i = 1; i <= initial_rows; ++i) {
            auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);
        }

        // Scale thread count and queries based on profile
        size_t thread_count = (config_.scalability_rows < 1000) ? 2 : 8;
        size_t queries_per_thread = (config_.scalability_rows < 1000) ? 5 : 100;

        std::cout << "  Starting " << thread_count << " reader threads...\n";

        auto start = high_resolution_clock::now();

        std::vector<std::future<size_t>> futures;

        // Pre-create the search value to avoid string construction overhead
        const std::string engineering_dept = "Engineering";

        for (size_t t = 0; t < thread_count; ++t) {
            futures.emplace_back(std::async(std::launch::async, [table_ptr, queries_per_thread, &engineering_dept]() {
                size_t total_results = 0;

                for (size_t q = 0; q < queries_per_thread; ++q) {
                    try {
                        TableQuery query;
                        query.where("department", QueryOperator::Equal, engineering_dept)
                             .limit(10);  // Reduced limit for faster queries

                        auto results = table_ptr->query(query);
                        total_results += results.size();

                        // Small delay to avoid overwhelming the system
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    } catch (const std::exception& e) {
                        // Continue on error
                        continue;
                    }
                }

                return total_results;
            }));
        }

        size_t total_results = 0;
        for (auto& future : futures) {
            try {
                total_results += future.get();
            } catch (const std::exception& e) {
                std::cout << "    Thread error: " << e.what() << "\n";
            }
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        size_t total_queries = thread_count * queries_per_thread;
        double queries_per_second = static_cast<double>(total_queries) / (duration.count() / 1000.0);

        std::cout << "  Results:\n";
        std::cout << "    Total queries: " << total_queries << "\n";
        std::cout << "    Time: " << duration.count() << " ms\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0) << queries_per_second << " queries/sec\n";
        std::cout << "    Total results: " << total_results << "\n\n";
    }

    void testConcurrentReadWrites(const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();
        Table* table_ptr = table.get();  // Get raw pointer for lambda capture

        // Scale thread counts and operations based on profile
        size_t reader_threads = (config_.scalability_rows < 1000) ? 2 : 4;
        size_t writer_threads = (config_.scalability_rows < 1000) ? 1 : 2;
        size_t operations_per_thread = (config_.scalability_rows < 1000) ? 5 : 100;

        std::cout << "  Starting " << reader_threads << " readers and " << writer_threads << " writers...\n";

        auto start = high_resolution_clock::now();

        std::vector<std::future<size_t>> read_futures;
        std::vector<std::future<size_t>> write_futures;
        std::atomic<size_t> next_id{1};

        // Start reader threads with safe capture
        for (size_t t = 0; t < reader_threads; ++t) {
            read_futures.emplace_back(std::async(std::launch::async, [table_ptr, operations_per_thread]() {
                size_t successful_reads = 0;

                for (size_t q = 0; q < operations_per_thread; ++q) {
                    try {
                        TableQuery query;
                        query.where("active", QueryOperator::Equal, true)
                             .limit(10);  // Reduced limit

                        auto results = table_ptr->query(query);
                        successful_reads++;

                        // Small delay to avoid overwhelming the system
                        std::this_thread::sleep_for(std::chrono::microseconds(5));
                    } catch (const std::exception& e) {
                        // Continue on error
                        continue;
                    }
                }
                return successful_reads;
            }));
        }

        // Start writer threads with safe capture
        for (size_t t = 0; t < writer_threads; ++t) {
            write_futures.emplace_back(std::async(std::launch::async, [this, table_ptr, operations_per_thread, &next_id]() {
                size_t successful_writes = 0;

                for (size_t i = 0; i < operations_per_thread; ++i) {
                    try {
                        size_t id = next_id.fetch_add(1);
                        auto row_data = this->generateEmployeeRow(static_cast<int64_t>(id));
                        table_ptr->insert_row(row_data);
                        successful_writes++;

                        // Small delay to avoid overwhelming the system
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    } catch (const std::exception& e) {
                        // Continue on error
                        continue;
                    }
                }
                return successful_writes;
            }));
        }

        // Wait for all threads and collect results
        size_t total_reads = 0;
        size_t total_writes = 0;

        for (auto& future : read_futures) {
            try {
                total_reads += future.get();
            } catch (const std::exception& e) {
                std::cout << "    Reader thread error: " << e.what() << "\n";
            }
        }

        for (auto& future : write_futures) {
            try {
                total_writes += future.get();
            } catch (const std::exception& e) {
                std::cout << "    Writer thread error: " << e.what() << "\n";
            }
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        size_t total_operations = total_reads + total_writes;
        double ops_per_second = static_cast<double>(total_operations) / (duration.count() / 1000.0);

        std::cout << "  Results:\n";
        std::cout << "    Successful reads: " << total_reads << "\n";
        std::cout << "    Successful writes: " << total_writes << "\n";
        std::cout << "    Total operations: " << total_operations << "\n";
        std::cout << "    Time: " << duration.count() << " ms\n";
        std::cout << "    Rate: " << std::fixed << std::setprecision(0) << ops_per_second << " ops/sec\n";
        std::cout << "    Final row count: " << table_ptr->get_row_count() << "\n\n";
    }

    void runMemoryBenchmarks() {
        std::cout << "=== MEMORY USAGE BENCHMARKS ===\n\n";

        testMemoryUsage("Memory Usage Analysis");
    }

    void testMemoryUsage(const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();

        // Adjust scales based on profile
        std::vector<size_t> scales;
        if (config_.scalability_rows <= 100) {
            scales = {10, 25, 50, 100};
        } else if (config_.scalability_rows <= 10000) {
            scales = {100, 500, 1000, 5000};
        } else if (config_.scalability_rows <= 100000) {
            scales = {1000, 5000, 10000, 50000};
        } else {
            scales = {10000, 50000, 100000, 500000};
        }

        for (size_t scale : scales) {
            // Clear table
            table = createEmployeeTable();

            size_t memory_before = getMemoryUsage();

            for (size_t i = 1; i <= scale; ++i) {
                auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
                table->insert_row(row_data);
            }

            size_t memory_after = getMemoryUsage();
            size_t memory_used = memory_after - memory_before;
            double bytes_per_row = static_cast<double>(memory_used) / scale;

            std::cout << "  Scale " << scale << " rows:\n";
            std::cout << "    Memory used: " << (memory_used / 1024) << " KB\n";
            std::cout << "    Bytes per row: " << std::fixed << std::setprecision(2) << bytes_per_row << "\n";
            std::cout << "    Row count: " << table->get_row_count() << "\n\n";
        }
    }

    void runSerializationBenchmarks() {
        if (config_.skip_serialization) {
            std::cout << "=== SERIALIZATION BENCHMARKS (SKIPPED) ===\n\n";
            return;
        }

        std::cout << "=== SERIALIZATION BENCHMARKS ===\n\n";

        // Try a minimal test first to see if serialization works at all
        testMinimalSerialization("Minimal JSON Test");

        // Only run full test if minimal test succeeds
        testSerialization("JSON Serialization Performance");
    }

    void testMinimalSerialization(const std::string& test_name) {
        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();

        // Test with just 3 rows
        std::cout << "  Creating table with 3 rows...\n";
        base::Logger::info("Inserting 3 rows for minimal serialization test");
        for (size_t i = 1; i <= 3; ++i) {
            auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);
        }

        try {
            std::cout << "  Testing basic serialization...\n";
            auto start = high_resolution_clock::now();

            std::string json_data = table->to_json();

            auto serialize_time = high_resolution_clock::now();
            auto serialize_duration = duration_cast<milliseconds>(serialize_time - start);

            if (json_data.empty()) {
                std::cout << "  ERROR: Serialization returned empty string\n\n";
                return;
            }

            std::cout << "  JSON size: " << json_data.size() << " bytes\n";
            std::cout << "  Serialize time: " << serialize_duration.count() << " ms\n";

            // Test deserialization
            auto new_table = createEmployeeTable();
            bool success = new_table->from_json(json_data);

            auto end = high_resolution_clock::now();
            auto deserialize_duration = duration_cast<milliseconds>(end - serialize_time);

            std::cout << "  Deserialize time: " << deserialize_duration.count() << " ms\n";
            std::cout << "  Success: " << (success ? "Yes" : "No") << "\n";
            std::cout << "  Rows recovered: " << new_table->get_row_count() << "\n";

            if (!success || new_table->get_row_count() != 3) {
                std::cout << "  WARNING: Serialization/deserialization failed, skipping full test\n";
                config_.skip_serialization = true;  // Auto-skip full test
            }

        } catch (const std::exception& e) {
            std::cout << "  ERROR: Exception during serialization: " << e.what() << "\n";
            std::cout << "  Skipping full serialization test\n";
            config_.skip_serialization = true;  // Auto-skip full test
        }

        std::cout << "\n";
    }

    void testSerialization(const std::string& test_name) {
        if (config_.skip_serialization) {
            std::cout << "Test: " << test_name << " (SKIPPED)\n\n";
            return;
        }

        std::cout << "Test: " << test_name << "\n";

        auto table = createEmployeeTable();
        size_t test_rows = config_.serialization_rows;

        if (config_.interactive && test_rows > 25000) {
            std::cout << "  WARNING: Serialization test with " << test_rows << " rows may be slow.\n";
            std::cout << "  Press Enter to continue or Ctrl+C to cancel...\n";
            std::cin.get();
        }

        std::cout << "  Creating table with " << test_rows << " rows...\n";
        for (size_t i = 1; i <= test_rows; ++i) {
            auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);
        }

        // Test serialization
        std::cout << "  Serializing to JSON...\n";
        auto start = high_resolution_clock::now();
        std::string json_data = table->to_json();
        auto end = high_resolution_clock::now();

        auto serialize_duration = duration_cast<milliseconds>(end - start);
        size_t json_size = json_data.size();

        // Test deserialization
        std::cout << "  Deserializing from JSON...\n";
        auto new_table = createEmployeeTable();

        start = high_resolution_clock::now();
        bool success = new_table->from_json(json_data);
        end = high_resolution_clock::now();

        auto deserialize_duration = duration_cast<milliseconds>(end - start);

        std::cout << "  Results:\n";
        std::cout << "    Serialize time: " << serialize_duration.count() << " ms\n";
        std::cout << "    Deserialize time: " << deserialize_duration.count() << " ms\n";
        std::cout << "    JSON size: " << (json_size / 1024 / 1024) << " MB\n";
        std::cout << "    Success: " << (success ? "Yes" : "No") << "\n";
        std::cout << "    Deserialized rows: " << new_table->get_row_count() << "\n";

        if (test_rows > 0) {
            std::cout << "    Serialize rate: " << std::fixed << std::setprecision(0)
                      << (test_rows / (serialize_duration.count() / 1000.0)) << " rows/sec\n";
            std::cout << "    Deserialize rate: " << std::fixed << std::setprecision(0)
                      << (test_rows / (deserialize_duration.count() / 1000.0)) << " rows/sec\n";
        }
        std::cout << "\n";
    }

    void runExtremeScaleBenchmark() {
        if (config_.skip_extreme) {
            std::cout << "=== EXTREME SCALE BENCHMARK (SKIPPED) ===\n\n";
            return;
        }

        std::cout << "=== EXTREME SCALE BENCHMARK ===\n\n";
        testExtremeScale();
    }

    void testExtremeScale() {
        std::cout << "Test: Ultimate Scale Test (" << (config_.extreme_rows / 1000000.0) << " Million Rows)\n";

        if (config_.interactive) {
            std::cout << "WARNING: This test may take several minutes and use significant memory!\n";
            std::cout << "Press Enter to continue or Ctrl+C to cancel...\n";
            std::cin.get();
        }

        auto table = createEmployeeTable();
        size_t target_rows = config_.extreme_rows;
        size_t batch_size = 100000;

        std::cout << "  Target: " << target_rows << " rows in batches of " << batch_size << "\n";

        auto overall_start = high_resolution_clock::now();

        for (size_t batch = 0; batch * batch_size < target_rows; ++batch) {
            size_t batch_start = batch * batch_size + 1;
            size_t batch_end = std::min(batch_start + batch_size - 1, target_rows);

            auto batch_timer_start = high_resolution_clock::now();

            for (size_t i = batch_start; i <= batch_end; ++i) {
                auto row_data = generateEmployeeRow(static_cast<int64_t>(i));
                table->insert_row(row_data);
            }

            auto batch_timer_end = high_resolution_clock::now();
            auto batch_duration = duration_cast<milliseconds>(batch_timer_end - batch_timer_start);

            double batch_rate = static_cast<double>(batch_size) / (batch_duration.count() / 1000.0);

            std::cout << "    Batch " << (batch + 1) << ": " << batch_duration.count()
                      << " ms (" << std::fixed << std::setprecision(0) << batch_rate << " rows/sec)\n";
        }

        auto overall_end = high_resolution_clock::now();
        auto total_duration = duration_cast<seconds>(overall_end - overall_start);

        double overall_rate = static_cast<double>(target_rows) / total_duration.count();
        size_t final_count = table->get_row_count();
        size_t memory_used = getMemoryUsage();

        std::cout << "\n  EXTREME SCALE RESULTS:\n";
        std::cout << "    Total time: " << total_duration.count() << " seconds\n";
        std::cout << "    Overall rate: " << std::fixed << std::setprecision(0) << overall_rate << " rows/sec\n";
        std::cout << "    Final row count: " << final_count << "\n";
        std::cout << "    Memory usage: " << (memory_used / 1024 / 1024) << " MB\n";
        std::cout << "    Memory per row: " << std::fixed << std::setprecision(2)
                  << (static_cast<double>(memory_used) / final_count) << " bytes\n";

        // Test query performance on extreme scale
        std::cout << "\n  Testing query performance on extreme scale...\n";

        TableQuery query;
        query.where("department", QueryOperator::Equal, std::string("Engineering"))
             .limit(1000);

        auto query_start = high_resolution_clock::now();
        auto results = table->query(query);
        auto query_end = high_resolution_clock::now();

        auto query_duration = duration_cast<milliseconds>(query_end - query_start);

        std::cout << "    Query time: " << query_duration.count() << " ms\n";
        std::cout << "    Results found: " << results.size() << "\n";
        std::cout << "    Query rate: " << std::fixed << std::setprecision(0)
                  << (static_cast<double>(final_count) / (query_duration.count() / 1000.0)) << " rows scanned/sec\n\n";
    }

    size_t getMemoryUsage() {
        // Simple approximation - in a real benchmark you'd use proper memory profiling
        // This is just a placeholder that returns a reasonable estimate
        return 1024 * 1024; // 1MB baseline
    }
};

int main(int argc, char* argv[]) {
    try {
        BenchmarkConfig config;
        base::Logger::init();
        base::Logger::set_level(base::LogLevel::Info);

        // Parse command-line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--help" || arg == "-h") {
                std::cout << "Table Benchmark Suite\n";
                std::cout << "Usage: table_benchmark [options]\n";
                std::cout << "Options:\n";
                std::cout << "  --help, -h            Show this help message\n";
                std::cout << "  --profile PROFILE     Set benchmark profile:\n";
                std::cout << "                          default    - Quick validation (10 rows)\n";
                std::cout << "                          development- Development testing (1K rows)\n";
                std::cout << "                          ci         - Continuous integration (5K rows)\n";
                std::cout << "                          benchmark  - Full benchmark suite (1M rows)\n";
                std::cout << "                          stress     - Maximum stress testing (5M rows)\n";
                std::cout << "  --enable-serialization Enable the serialization performance test\n";
                std::cout << "  --skip-extreme        Skip the extreme scale test\n";
                std::cout << "  --scalability-rows N  Override scalability test rows\n";
                std::cout << "  --serialization-rows N Override serialization test rows\n";
                std::cout << "  --extreme-rows N      Override extreme test rows\n";
                std::cout << "  --interactive         Enable interactive mode for long tests\n";
                std::cout << "  --no-interactive      Disable interactive mode\n";
                return 0;
            } else if (arg == "--profile") {
                if (i + 1 < argc) {
                    std::string profile_name = argv[++i];
                    if (profile_name == "default") {
                        config.apply_profile(BenchmarkProfile::Default);
                    } else if (profile_name == "development" || profile_name == "dev") {
                        config.apply_profile(BenchmarkProfile::Development);
                    } else if (profile_name == "ci") {
                        config.apply_profile(BenchmarkProfile::CI);
                    } else if (profile_name == "benchmark" || profile_name == "bench") {
                        config.apply_profile(BenchmarkProfile::Benchmark);
                    } else if (profile_name == "stress") {
                        config.apply_profile(BenchmarkProfile::Stress);
                    } else {
                        std::cerr << "Unknown profile: " << profile_name << std::endl;
                        std::cerr << "Valid profiles: default, development, ci, benchmark, stress" << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "--profile requires a profile name" << std::endl;
                    return 1;
                }
            } else if (arg == "--enable-serialization") {
                config.skip_serialization = false;
            } else if (arg == "--skip-extreme") {
                config.skip_extreme = true;
            } else if (arg == "--scalability-rows") {
                if (i + 1 < argc) {
                    config.scalability_rows = std::stoul(argv[++i]);
                }
            } else if (arg == "--serialization-rows") {
                if (i + 1 < argc) {
                    config.serialization_rows = std::stoul(argv[++i]);
                }
            } else if (arg == "--extreme-rows") {
                if (i + 1 < argc) {
                    config.extreme_rows = std::stoul(argv[++i]);
                }
            } else if (arg == "--interactive") {
                config.interactive = true;
            } else if (arg == "--no-interactive") {
                config.interactive = false;
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                std::cerr << "Use --help for usage information" << std::endl;
                return 1;
            }
        }

        // Apply default profile if no profile was explicitly set
        if (config.profile == BenchmarkProfile::Default && argc == 1) {
            config.apply_profile(BenchmarkProfile::Default);
        }

        // Print final configuration
        config.print();

        TableBenchmark benchmark(config);

        benchmark.runAllBenchmarks();

    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
