// Example usage of the benchmark adapter
// This shows how to use profiles with Google Benchmark

#include "benchmark_adapter.h"
#include "../include/tables.h"

using namespace base;

// Simple example demonstrating profile usage
static void BM_TableInsert_Quick(benchmark::State& state) {
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

static void BM_TableInsert_Development(benchmark::State& state) {
    auto scale = benchmark_adapter::ProfileManager::get_scale_factor(
        benchmark_adapter::Profile::Development);

    auto schema = std::make_unique<TableSchema>("demo", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer));

    Table table(std::move(schema));

    for (auto _ : state) {
        for (size_t i = 0; i < scale / 10; ++i) { // Scale down for reasonable test time
            table.insert_row({{"id", static_cast<long long>(i)}});
        }
    }

    benchmark_adapter::TableMetrics::add_table_metrics(state, scale / 10, 1);
}

// Register benchmarks
BENCHMARK(BM_TableInsert_Quick);
BENCHMARK(BM_TableInsert_Development);

BENCHMARK_MAIN();
