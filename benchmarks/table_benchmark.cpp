/*
 * @file table_benchmark.cpp
 * @brief Table benchmarks using Google Benchmark
 *
 * This benchmark leverages Google Benchmark's superior statistical engine.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <benchmark/benchmark.h>
#include "../include/tables.h"
#include "../include/logger.h"
#include <vector>
#include <random>
#include <memory>
#include <sstream>
#include <atomic>

using namespace base;

// Shared data generator for consistent test data
class TableTestData {
public:
    static TableTestData& instance() {
        static TableTestData data;
        return data;
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

        return {
            {"id", id},
            {"first_name", first_name},
            {"last_name", last_name},
            {"email", email_stream.str()},
            {"age", static_cast<int64_t>(age_dist_(rng_))},
            {"salary", static_cast<double>(salary_dist_(rng_))},
            {"department", departments[id % departments.size()]},
            {"active", bool_dist_(rng_) == 1}
        };
    }

private:
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_int_distribution<int> age_dist_{18, 80};
    std::uniform_int_distribution<int> salary_dist_{30000, 200000};
    std::uniform_int_distribution<int> bool_dist_{0, 1};

    std::vector<std::string> first_names_ = {
        "John", "Jane", "Michael", "Sarah", "David", "Emily", "Robert", "Lisa"
    };

    std::vector<std::string> last_names_ = {
        "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller"
    };

    std::vector<std::string> domains_ = {
        "gmail.com", "yahoo.com", "hotmail.com", "outlook.com", "company.com"
    };
};

// Standard Google Benchmark functions
static void BM_TableInsertion(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table = data.createEmployeeTable();
        state.ResumeTiming();

        for (size_t i = 1; i <= scale; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);
        }
    }

    state.counters["Rows"] = benchmark::Counter(scale);
    state.counters["RowsPerSec"] = benchmark::Counter(scale, benchmark::Counter::kIsRate);
}

static void BM_TableQuery(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        // Query by primary key
        auto rows = table->find_by_index("__primary_key", {static_cast<std::int64_t>(state.iterations() % scale + 1)});
        benchmark::DoNotOptimize(rows);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
}

// Update operations benchmark
static void BM_TableUpdate(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        size_t row_id = (state.iterations() % scale) + 1;
        auto updated_data = data.generateEmployeeRow(static_cast<int64_t>(row_id));
        updated_data["salary"] = static_cast<double>(150000); // Update salary
        table->update_row(row_id, updated_data);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["UpdatesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Delete operations benchmark
static void BM_TableDelete(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table = data.createEmployeeTable();
        // Insert test data
        for (size_t i = 1; i <= scale; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);
        }
        state.ResumeTiming();

        // Delete half the rows
        for (size_t i = 1; i <= scale / 2; ++i) {
            table->delete_row(i);
        }
    }

    state.counters["DeletesPerIteration"] = benchmark::Counter(scale / 2);
    state.counters["DeletesPerSec"] = benchmark::Counter(scale / 2, benchmark::Counter::kIsRate);
}

// Index creation benchmark
static void BM_TableIndexCreation(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        state.PauseTiming();
        // Drop index if it exists from previous iteration
        try {
            table->drop_index("idx_department");
        } catch (...) {} // Ignore if index doesn't exist
        state.ResumeTiming();

        table->create_index("idx_department", {"department"});
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["IndexCreationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Index query benchmark
static void BM_TableIndexQuery(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data and create index
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }
    table->create_index("idx_department", {"department"});

    std::vector<std::string> departments = {
        "Engineering", "Sales", "Marketing", "HR", "Finance", "Operations", "Support"
    };

    for (auto _ : state) {
        std::string dept = departments[state.iterations() % departments.size()];
        auto rows = table->find_by_index("idx_department", {dept});
        benchmark::DoNotOptimize(rows);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["QueriesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Table copy/clone benchmark
static void BM_TableClone(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        auto cloned_table = table->clone();
        benchmark::DoNotOptimize(cloned_table);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["ClonesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// JSON serialization benchmark
static void BM_TableJsonSerialization(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        std::string json = table->to_json();
        benchmark::DoNotOptimize(json);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["SerializationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);

    // Estimate JSON size
    std::string sample_json = table->to_json();
    state.counters["JsonSizeKB"] = benchmark::Counter(sample_json.size() / 1024.0);
}

// JSON deserialization benchmark
static void BM_TableJsonDeserialization(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data and serialize
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }
    std::string json_data = table->to_json();

    for (auto _ : state) {
        state.PauseTiming();
        auto empty_table = data.createEmployeeTable();
        state.ResumeTiming();

        empty_table->from_json(json_data);
        benchmark::DoNotOptimize(empty_table);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["DeserializationsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
    state.counters["JsonSizeKB"] = benchmark::Counter(json_data.size() / 1024.0);
}

// Iterator performance benchmark
static void BM_TableIteration(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        size_t count = 0;
        for (const auto& row : *table) {
            count++;
            auto row_id = row.get_id();
            benchmark::DoNotOptimize(row_id);
        }
        benchmark::DoNotOptimize(count);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["RowsIteratedPerSec"] = benchmark::Counter(scale, benchmark::Counter::kIsRate);
}

// Table merge benchmark
static void BM_TableMerge(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table1 = data.createEmployeeTable();
        auto table2 = data.createEmployeeTable();

        // Insert data into both tables
        for (size_t i = 1; i <= scale / 2; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
            table1->insert_row(row_data);
        }
        for (size_t i = scale / 2 + 1; i <= scale; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
            row_data["id"] = static_cast<int64_t>(i + 1000); // Avoid ID conflicts
            table2->insert_row(row_data);
        }
        state.ResumeTiming();

        table1->merge_from(*table2);
        benchmark::DoNotOptimize(table1);
    }

    state.counters["MergedRows"] = benchmark::Counter(scale);
    state.counters["MergesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Complex query benchmark
static void BM_TableComplexQuery(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    // Create indexes for better query performance
    table->create_index("idx_age", {"age"});
    table->create_index("idx_department", {"department"});

    for (auto _ : state) {
        // Complex query: Find all active employees in Engineering dept with age > 30
        TableQuery query;
        query.where("department", QueryOperator::Equal, std::string("Engineering"))
             .where("age", QueryOperator::GreaterThan, static_cast<int64_t>(30))
             .where("active", QueryOperator::Equal, true);

        auto results = table->query(query);
        benchmark::DoNotOptimize(results);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["ComplexQueriesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Concurrent access benchmark
static void BM_TableConcurrentAccess(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();
    table->enable_concurrent_access(true);

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    std::atomic<size_t> next_id{static_cast<size_t>(scale + 1)};

    for (auto _ : state) {
        // Simulate concurrent operations
        if (state.iterations() % 3 == 0) {
            // Insert operation with unique ID
            size_t unique_id = next_id.fetch_add(1);
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(unique_id));
            table->insert_row(row_data);
        } else if (state.iterations() % 3 == 1) {
            // Query operation
            auto rows = table->find_by_index("__primary_key", {static_cast<int64_t>((state.iterations() % scale) + 1)});
            benchmark::DoNotOptimize(rows);
        } else {
            // Update operation
            size_t row_id = (state.iterations() % scale) + 1;
            table->update_row(row_id, {{"salary", static_cast<double>(state.iterations() % 100000 + 50000)}});
        }
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["ConcurrentOpsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Memory usage benchmark (row count scaling)
static void BM_TableMemoryScaling(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table = data.createEmployeeTable();
        state.ResumeTiming();

        // Insert data and measure time
        for (size_t i = 1; i <= scale; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
            table->insert_row(row_data);
        }

        benchmark::DoNotOptimize(table);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["RowsPerSec"] = benchmark::Counter(scale, benchmark::Counter::kIsRate);

    // Estimate memory usage (rough calculation)
    size_t estimated_row_size = 200; // bytes per row (rough estimate)
    state.counters["EstimatedMemoryMB"] = benchmark::Counter((scale * estimated_row_size) / (1024.0 * 1024.0));
}

// Transaction operations benchmark
static void BM_TableTransactions(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert initial test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    std::atomic<size_t> next_id{static_cast<size_t>(scale + 1)};

    for (auto _ : state) {
        auto transaction = table->begin_transaction();
        transaction->begin();

        // Perform multiple operations within transaction with unique IDs
        for (size_t i = 0; i < 10; ++i) {
            size_t unique_id = next_id.fetch_add(1);
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(unique_id));
            table->insert_row(row_data);
        }

        transaction->commit();
        benchmark::DoNotOptimize(transaction);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["TransactionsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Schema evolution benchmark
static void BM_TableSchemaEvolution(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    uint32_t schema_version = 1;
    for (auto _ : state) {
        // Create new schema with additional column
        auto new_schema = std::make_unique<TableSchema>("employees", ++schema_version);
        new_schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        new_schema->add_column(ColumnDefinition("first_name", ColumnType::String, false));
        new_schema->add_column(ColumnDefinition("last_name", ColumnType::String, false));
        new_schema->add_column(ColumnDefinition("email", ColumnType::String, false));
        new_schema->add_column(ColumnDefinition("age", ColumnType::Integer, false));
        new_schema->add_column(ColumnDefinition("salary", ColumnType::Double, false));
        new_schema->add_column(ColumnDefinition("department", ColumnType::String, false));
        new_schema->add_column(ColumnDefinition("active", ColumnType::Boolean, false));
        new_schema->add_column(ColumnDefinition("bonus", ColumnType::Double, true)); // New column

        table->evolve_schema(std::move(new_schema));
        benchmark::DoNotOptimize(table);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["SchemaEvolutionsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Change callbacks benchmark
static void BM_TableChangeCallbacks(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Add change callback
    std::atomic<size_t> callback_count{0};
    table->add_change_callback("test_callback", [&callback_count](const ChangeEvent& event) {
        callback_count++;
    });

    std::uint64_t row_id = 0;
    for (auto _ : state) {
        state.PauseTiming();
        callback_count = 0;
        state.ResumeTiming();

        // Perform operations that trigger callbacks
        for (size_t i = 1; i <= scale; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(++row_id));
            table->insert_row(row_data);
        }

        benchmark::DoNotOptimize(callback_count.load());
    }

    state.counters["CallbacksTriggered"] = benchmark::Counter(scale);
    state.counters["CallbacksPerSec"] = benchmark::Counter(scale, benchmark::Counter::kIsRate);
}

// File persistence benchmark - Save
static void BM_TableFileSave(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        std::string filename = "/tmp/benchmark_table_" + std::to_string(state.iterations()) + ".dat";
        table->save_to_file(filename);

        // Clean up
        std::remove(filename.c_str());
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["SavesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// File persistence benchmark - Load
static void BM_TableFileLoad(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data and save to file
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    std::string filename = "/tmp/benchmark_table_load.dat";
    table->save_to_file(filename);

    for (auto _ : state) {
        state.PauseTiming();
        auto empty_table = data.createEmployeeTable();
        state.ResumeTiming();

        empty_table->load_from_file(filename);
        benchmark::DoNotOptimize(empty_table);
    }

    // Clean up
    std::remove(filename.c_str());

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["LoadsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Statistics gathering benchmark
static void BM_TableStatistics(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    for (auto _ : state) {
        auto stats = table->get_statistics();
        benchmark::DoNotOptimize(stats);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["StatisticsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Table dump/formatting benchmark
static void BM_TableDump(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    TableDumpOptions options;
    options.format = TableOutputFormat::ASCII;
    options.page_size = 20;

    for (auto _ : state) {
        std::string dump = table->dump_to_string(options);
        benchmark::DoNotOptimize(dump);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["DumpsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Advanced query operations benchmark
static void BM_TableAdvancedQuery(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    // Create compound index
    table->create_index("idx_compound", {"department", "age"});

    for (auto _ : state) {
        // Advanced query with multiple conditions, ordering, and limits
        TableQuery query;
        query.where("age", QueryOperator::GreaterThan, static_cast<int64_t>(25))
             .where("age", QueryOperator::LessThan, static_cast<int64_t>(65))
             .where("active", QueryOperator::Equal, true)
             .order_by("salary", false) // Descending
             .limit(10);

        auto results = table->query(query);
        benchmark::DoNotOptimize(results);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["AdvancedQueriesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Range query benchmark
static void BM_TableRangeQuery(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();
    auto table = data.createEmployeeTable();

    // Setup: Insert test data
    for (size_t i = 1; i <= scale; ++i) {
        auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
        table->insert_row(row_data);
    }

    // Create index for range queries
    table->create_index("idx_salary", {"salary"});

    for (auto _ : state) {
        // Range query on salary using Between operator
        TableQuery query;
        query.where(QueryCondition("salary", QueryOperator::Between,
                   static_cast<double>(50000), static_cast<double>(100000)));

        auto results = table->query(query);
        benchmark::DoNotOptimize(results);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["RangeQueriesPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Bulk operations benchmark
static void BM_TableBulkOperations(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table = data.createEmployeeTable();
        std::vector<std::unordered_map<std::string, CellValue>> bulk_data;

        // Prepare bulk data
        for (size_t i = 1; i <= scale; ++i) {
            bulk_data.push_back(data.generateEmployeeRow(static_cast<int64_t>(i)));
        }
        state.ResumeTiming();

        // Bulk insert
        for (const auto& row_data : bulk_data) {
            table->insert_row(row_data);
        }

        benchmark::DoNotOptimize(table);
    }

    state.counters["BulkRows"] = benchmark::Counter(scale);
    state.counters["BulkRowsPerSec"] = benchmark::Counter(scale, benchmark::Counter::kIsRate);
}

// Table swap benchmark
static void BM_TableSwap(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table1 = data.createEmployeeTable();
        auto table2 = data.createEmployeeTable();

        // Insert data into both tables
        for (size_t i = 1; i <= scale; ++i) {
            table1->insert_row(data.generateEmployeeRow(static_cast<int64_t>(i)));
            table2->insert_row(data.generateEmployeeRow(static_cast<int64_t>(i + scale)));
        }
        state.ResumeTiming();

        table1->swap(*table2);
        benchmark::DoNotOptimize(table1);
        benchmark::DoNotOptimize(table2);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["SwapsPerSec"] = benchmark::Counter(state.iterations(), benchmark::Counter::kIsRate);
}

// Cell value operations benchmark
static void BM_CellValueOperations(benchmark::State& state) {
    auto scale = state.range(0);

    std::vector<CellValue> test_values;
    for (size_t i = 0; i < scale; ++i) {
        test_values.push_back(static_cast<int64_t>(i));
        test_values.push_back(static_cast<double>(i * 1.5));
        test_values.push_back(std::string("test_string_") + std::to_string(i));
        test_values.push_back(i % 2 == 0);
    }

    for (auto _ : state) {
        size_t count = 0;
        for (const auto& value : test_values) {
            std::string str = cell_utils::to_string(value);
            ColumnType type = cell_utils::get_value_type(value);
            bool is_null = cell_utils::is_null(value);

            count += str.length() + static_cast<size_t>(type) + (is_null ? 1 : 0);
        }
        benchmark::DoNotOptimize(count);
    }

    state.counters["CellValues"] = benchmark::Counter(test_values.size());
    state.counters["CellOpsPerSec"] = benchmark::Counter(test_values.size(), benchmark::Counter::kIsRate);
}

// Memory efficiency benchmark
static void BM_TableMemoryEfficiency(benchmark::State& state) {
    auto scale = state.range(0);
    auto& data = TableTestData::instance();

    for (auto _ : state) {
        state.PauseTiming();
        auto table = data.createEmployeeTable();
        state.ResumeTiming();

        // Insert, access, and immediately remove to test memory management
        std::vector<size_t> row_ids;
        for (size_t i = 1; i <= scale; ++i) {
            auto row_data = data.generateEmployeeRow(static_cast<int64_t>(i));
            size_t row_id = table->insert_row(row_data);
            row_ids.push_back(row_id);
        }

        // Access all rows
        for (size_t row_id : row_ids) {
            auto row = table->get_row(row_id);
            benchmark::DoNotOptimize(row);
        }

        // Delete all rows
        for (size_t row_id : row_ids) {
            table->delete_row(row_id);
        }

        benchmark::DoNotOptimize(table);
    }

    state.counters["TableRows"] = benchmark::Counter(scale);
    state.counters["MemoryOpsPerSec"] = benchmark::Counter(scale * 3, benchmark::Counter::kIsRate); // insert + get + delete
}

// Register benchmarks with comprehensive argument ranges
BENCHMARK(BM_TableInsertion)->Arg(10)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableQuery)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableUpdate)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableDelete)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableIndexCreation)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableIndexQuery)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableClone)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableJsonSerialization)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableJsonDeserialization)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableIteration)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableMerge)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableComplexQuery)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableConcurrentAccess)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableMemoryScaling)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableTransactions)->Arg(100)->Arg(500)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableSchemaEvolution)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableChangeCallbacks)->Arg(50)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableFileSave)->Arg(100)->Arg(1000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_TableFileLoad)->Arg(100)->Arg(1000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_TableStatistics)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableDump)->Arg(50)->Arg(200)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_TableAdvancedQuery)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableRangeQuery)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableBulkOperations)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableSwap)->Arg(100)->Arg(1000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_CellValueOperations)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_TableMemoryEfficiency)->Arg(100)->Arg(500)->Unit(benchmark::kMicrosecond);

// Use Google Benchmark's main
BENCHMARK_MAIN();
