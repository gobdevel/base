// Example usage of the benchmark adapter
// This shows how to use profiles with Google Benchmark

#include "benchmark_adapter.h"
#include "../include/tables.h"

using namespace base;

// Example using the BENCHMARK_PROFILE macro for a single profile
BENCHMARK_PROFILE(TableInsert, Quick) {
    auto scale = benchmark_adapter::ProfileManager::get_scale_factor(
        benchmark_adapter::Profile::Quick);

    auto schema = std::make_unique<TableSchema>("demo", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer));

    Table table(std::move(schema));

    for (auto _ : state) {
        for (size_t i = 0; i < scale; ++i) {
            table.insert_row({{"id", static_cast<long long>(i)}});
        }
    }

    benchmark_adapter::TableMetrics::add_table_metrics(state, scale, 1);
}

// Example of a simple query benchmark
BENCHMARK_PROFILE(TableQuery, Development) {
    auto scale = benchmark_adapter::ProfileManager::get_scale_factor(
        benchmark_adapter::Profile::Development);

    auto schema = std::make_unique<TableSchema>("demo", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer));

    Table table(std::move(schema));

    // Pre-populate table
    for (size_t i = 0; i < scale; ++i) {
        table.insert_row({{"id", static_cast<long long>(i)}});
    }

    for (auto _ : state) {
        // Query operation using TableQuery
        TableQuery query;
        query.where("id", QueryOperator::GreaterThan, static_cast<long long>(scale/2));
        auto results = table.query(query);
        benchmark::DoNotOptimize(results);
    }

    benchmark_adapter::TableMetrics::add_table_metrics(state, scale, 1);
}

BENCHMARK_MAIN();
