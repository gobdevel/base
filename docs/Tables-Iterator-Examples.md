# Table Iterator and Container API Examples

This document provides practical examples of using the Table iterator and container APIs.

## Basic Iterator Usage

### Range-Based For Loops

```cpp
#include "tables.h"
#include <iostream>

void basic_iteration_example() {
    // Create and populate table
    auto schema = std::make_unique<TableSchema>("products", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Insert sample data
    table->insert_row({{"id", 1L}, {"name", std::string("Laptop")}, {"price", 999.99}});
    table->insert_row({{"id", 2L}, {"name", std::string("Mouse")}, {"price", 29.99}});
    table->insert_row({{"id", 3L}, {"name", std::string("Keyboard")}, {"price", 79.99}});

    // Range-based for loop - preferred method
    std::cout << "All products:" << std::endl;
    for (const auto& row : *table) {
        auto id = std::get<std::int64_t>(row.get_value("id"));
        auto name = std::get<std::string>(row.get_value("name"));
        auto price = std::get<double>(row.get_value("price"));

        std::cout << "  ID: " << id << ", Name: " << name
                  << ", Price: $" << price << std::endl;
    }
}
```

### Explicit Iterator Usage

```cpp
void explicit_iterator_example() {
    auto table = create_sample_table();

    // Using explicit iterators
    std::cout << "Using explicit iterators:" << std::endl;
    for (auto it = table->begin(); it != table->end(); ++it) {
        const TableRow& row = *it;
        std::cout << "Row ID: " << row.get_id() << std::endl;
    }

    // Using const iterators
    std::cout << "Using const iterators:" << std::endl;
    for (auto it = table->cbegin(); it != table->cend(); ++it) {
        std::cout << "Row version: " << it->get_version() << std::endl;
    }

    // Alternative naming
    for (auto it = table->rows_begin(); it != table->rows_end(); ++it) {
        // Process rows
    }
}
```

### STL Algorithm Integration

```cpp
#include <algorithm>
#include <numeric>

void stl_algorithm_examples() {
    auto table = create_sample_table();

    // Count rows matching a condition
    auto expensive_count = std::count_if(table->begin(), table->end(),
        [](const TableRow& row) {
            auto price = std::get<double>(row.get_value("price"));
            return price > 100.0;
        });

    std::cout << "Expensive items: " << expensive_count << std::endl;

    // Find first row matching condition
    auto it = std::find_if(table->begin(), table->end(),
        [](const TableRow& row) {
            auto name = std::get<std::string>(row.get_value("name"));
            return name.find("Mouse") != std::string::npos;
        });

    if (it != table->end()) {
        std::cout << "Found mouse with ID: " << it->get_id() << std::endl;
    }

    // Check if all items are reasonably priced
    bool all_affordable = std::all_of(table->begin(), table->end(),
        [](const TableRow& row) {
            auto price = std::get<double>(row.get_value("price"));
            return price < 2000.0;
        });

    std::cout << "All items affordable: " << (all_affordable ? "Yes" : "No") << std::endl;
}
```

## Copy and Move Operations

### Copy Constructor and Assignment

```cpp
void copy_operations_example() {
    auto original = create_sample_table();

    // Copy constructor
    std::cout << "Creating copy..." << std::endl;
    Table copied_table(*original);

    std::cout << "Original rows: " << original->get_row_count() << std::endl;
    std::cout << "Copied rows: " << copied_table.get_row_count() << std::endl;

    // Verify independence
    copied_table.clear();
    std::cout << "After clearing copy:" << std::endl;
    std::cout << "Original rows: " << original->get_row_count() << std::endl;
    std::cout << "Copied rows: " << copied_table.get_row_count() << std::endl;

    // Copy assignment
    auto another_table = create_empty_table();
    *another_table = *original;  // Copy assignment

    std::cout << "Copy assignment result: " << another_table->get_row_count() << " rows" << std::endl;
}
```

### Move Constructor and Assignment

```cpp
std::unique_ptr<Table> create_large_table() {
    auto table = create_sample_table();

    // Add more data
    for (int i = 4; i <= 1000; ++i) {
        table->insert_row({
            {"id", static_cast<std::int64_t>(i)},
            {"name", std::string("Product_") + std::to_string(i)},
            {"price", static_cast<double>(i * 10.5)}
        });
    }

    return table;  // Move constructor used here
}

void move_operations_example() {
    std::cout << "Creating large table..." << std::endl;
    auto large_table = create_large_table();  // Move constructor

    std::cout << "Large table rows: " << large_table->get_row_count() << std::endl;

    // Move assignment
    auto destination_table = create_empty_table();
    *destination_table = std::move(*large_table);  // Move assignment

    std::cout << "After move:" << std::endl;
    std::cout << "Destination rows: " << destination_table->get_row_count() << std::endl;
    std::cout << "Source rows: " << large_table->get_row_count() << std::endl;  // Should be 0
}
```

## Table Utility Methods

### Clear and Empty Operations

```cpp
void clear_and_empty_example() {
    auto table = create_sample_table();

    std::cout << "Initial state:" << std::endl;
    std::cout << "Row count: " << table->get_row_count() << std::endl;
    std::cout << "Is empty: " << (table->empty() ? "Yes" : "No") << std::endl;

    // Clear all data
    table->clear();

    std::cout << "After clear:" << std::endl;
    std::cout << "Row count: " << table->get_row_count() << std::endl;
    std::cout << "Is empty: " << (table->empty() ? "Yes" : "No") << std::endl;

    // Schema and indexes are preserved
    std::cout << "Schema name: " << table->get_schema().get_name() << std::endl;
    std::cout << "Index count: " << table->get_index_names().size() << std::endl;
}
```

### Clone Operation

```cpp
void clone_example() {
    auto original = create_sample_table();

    // Add some indexes and callbacks
    original->create_index("price_idx", {"price"});
    original->add_change_callback("logger", [](const ChangeEvent& event) {
        std::cout << "Change detected: " << static_cast<int>(event.type) << std::endl;
    });

    std::cout << "Original table state:" << std::endl;
    std::cout << "Rows: " << original->get_row_count() << std::endl;
    std::cout << "Indexes: " << original->get_index_names().size() << std::endl;

    // Clone the table
    auto cloned = original->clone();

    std::cout << "Cloned table state:" << std::endl;
    std::cout << "Rows: " << cloned->get_row_count() << std::endl;
    std::cout << "Indexes: " << cloned->get_index_names().size() << std::endl;

    // Verify independence
    cloned->insert_row({{"id", 999L}, {"name", std::string("New Item")}, {"price", 50.0}});

    std::cout << "After adding to clone:" << std::endl;
    std::cout << "Original rows: " << original->get_row_count() << std::endl;
    std::cout << "Cloned rows: " << cloned->get_row_count() << std::endl;
}
```

### Merge Operation

```cpp
void merge_example() {
    auto table1 = create_sample_table();
    auto table2 = create_empty_table();

    // Add different data to second table (avoiding primary key conflicts)
    table2->insert_row({{"id", 10L}, {"name", std::string("Monitor")}, {"price", 299.99}});
    table2->insert_row({{"id", 11L}, {"name", std::string("Speakers")}, {"price", 89.99}});

    std::cout << "Before merge:" << std::endl;
    std::cout << "Table1 rows: " << table1->get_row_count() << std::endl;
    std::cout << "Table2 rows: " << table2->get_row_count() << std::endl;

    // Merge table2 into table1
    table1->merge_from(*table2);

    std::cout << "After merge:" << std::endl;
    std::cout << "Table1 rows: " << table1->get_row_count() << std::endl;
    std::cout << "Table2 rows: " << table2->get_row_count() << std::endl;  // Unchanged

    // Show merged data
    std::cout << "Merged table contents:" << std::endl;
    for (const auto& row : *table1) {
        auto name = std::get<std::string>(row.get_value("name"));
        std::cout << "  " << name << std::endl;
    }
}
```

### Swap Operation

```cpp
void swap_example() {
    auto table1 = create_sample_table();
    auto table2 = create_empty_table();

    // Add different data to table2
    table2->insert_row({{"id", 100L}, {"name", std::string("Software")}, {"price", 199.99}});

    auto count1_before = table1->get_row_count();
    auto count2_before = table2->get_row_count();

    std::cout << "Before swap:" << std::endl;
    std::cout << "Table1 rows: " << count1_before << std::endl;
    std::cout << "Table2 rows: " << count2_before << std::endl;

    // Swap contents
    table1->swap(*table2);

    std::cout << "After swap:" << std::endl;
    std::cout << "Table1 rows: " << table1->get_row_count() << std::endl;
    std::cout << "Table2 rows: " << table2->get_row_count() << std::endl;

    // Verify swap occurred
    assert(table1->get_row_count() == count2_before);
    assert(table2->get_row_count() == count1_before);

    std::cout << "Swap successful!" << std::endl;
}
```

## Advanced Examples

### Iterator with Filtering

```cpp
class FilteredIterator {
private:
    Table::const_iterator current_;
    Table::const_iterator end_;
    std::function<bool(const TableRow&)> predicate_;

    void advance_to_next_valid() {
        while (current_ != end_ && !predicate_(*current_)) {
            ++current_;
        }
    }

public:
    FilteredIterator(Table::const_iterator begin, Table::const_iterator end,
                     std::function<bool(const TableRow&)> pred)
        : current_(begin), end_(end), predicate_(pred) {
        advance_to_next_valid();
    }

    const TableRow& operator*() const { return *current_; }
    const TableRow* operator->() const { return &*current_; }

    FilteredIterator& operator++() {
        ++current_;
        advance_to_next_valid();
        return *this;
    }

    bool operator!=(const FilteredIterator& other) const {
        return current_ != other.current_;
    }
};

void filtered_iteration_example() {
    auto table = create_sample_table();

    // Create filtered range for expensive items
    auto expensive_predicate = [](const TableRow& row) {
        auto price = std::get<double>(row.get_value("price"));
        return price > 50.0;
    };

    FilteredIterator begin(table->begin(), table->end(), expensive_predicate);
    FilteredIterator end(table->end(), table->end(), expensive_predicate);

    std::cout << "Expensive items:" << std::endl;
    for (auto it = begin; it != end; ++it) {
        auto name = std::get<std::string>(it->get_value("name"));
        auto price = std::get<double>(it->get_value("price"));
        std::cout << "  " << name << ": $" << price << std::endl;
    }
}
```

### Batch Operations with Iterators

```cpp
void batch_operations_example() {
    auto table = create_large_table();

    // Process in batches using iterators
    const size_t batch_size = 100;
    size_t processed = 0;

    auto it = table->begin();
    while (it != table->end()) {
        // Process batch
        size_t batch_count = 0;
        double batch_total = 0.0;

        auto batch_start = it;
        while (it != table->end() && batch_count < batch_size) {
            auto price = std::get<double>(it->get_value("price"));
            batch_total += price;
            ++it;
            ++batch_count;
        }

        double batch_average = batch_total / batch_count;
        processed += batch_count;

        std::cout << "Batch " << (processed / batch_size)
                  << ": " << batch_count << " items, "
                  << "Average price: $" << batch_average << std::endl;
    }

    std::cout << "Total processed: " << processed << " items" << std::endl;
}
```

## Performance Comparison

```cpp
#include <chrono>

void performance_comparison() {
    auto table = create_large_table();

    auto start = std::chrono::high_resolution_clock::now();

    // Method 1: Range-based for loop (recommended)
    size_t count1 = 0;
    for (const auto& row : *table) {
        count1++;
    }

    auto mid1 = std::chrono::high_resolution_clock::now();

    // Method 2: Explicit iterators
    size_t count2 = 0;
    for (auto it = table->begin(); it != table->end(); ++it) {
        count2++;
    }

    auto mid2 = std::chrono::high_resolution_clock::now();

    // Method 3: get_all_rows() (for comparison)
    auto all_rows = table->get_all_rows();
    size_t count3 = all_rows.size();

    auto end = std::chrono::high_resolution_clock::now();

    auto time1 = std::chrono::duration_cast<std::chrono::microseconds>(mid1 - start);
    auto time2 = std::chrono::duration_cast<std::chrono::microseconds>(mid2 - mid1);
    auto time3 = std::chrono::duration_cast<std::chrono::microseconds>(end - mid2);

    std::cout << "Performance comparison (" << count1 << " rows):" << std::endl;
    std::cout << "Range-based for: " << time1.count() << " μs" << std::endl;
    std::cout << "Explicit iterator: " << time2.count() << " μs" << std::endl;
    std::cout << "get_all_rows(): " << time3.count() << " μs" << std::endl;
}
```

## Helper Functions

```cpp
// Helper function to create a sample table
std::unique_ptr<Table> create_sample_table() {
    auto schema = std::make_unique<TableSchema>("products", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    table->insert_row({{"id", 1L}, {"name", std::string("Laptop")}, {"price", 999.99}});
    table->insert_row({{"id", 2L}, {"name", std::string("Mouse")}, {"price", 29.99}});
    table->insert_row({{"id", 3L}, {"name", std::string("Keyboard")}, {"price", 79.99}});

    return table;
}

// Helper function to create an empty table with same schema
std::unique_ptr<Table> create_empty_table() {
    auto schema = std::make_unique<TableSchema>("products", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
    schema->set_primary_key({"id"});

    return std::make_unique<Table>(std::move(schema));
}

int main() {
    try {
        std::cout << "=== Table Iterator and Container API Examples ===" << std::endl;

        basic_iteration_example();
        std::cout << std::endl;

        explicit_iterator_example();
        std::cout << std::endl;

        stl_algorithm_examples();
        std::cout << std::endl;

        copy_operations_example();
        std::cout << std::endl;

        move_operations_example();
        std::cout << std::endl;

        clear_and_empty_example();
        std::cout << std::endl;

        clone_example();
        std::cout << std::endl;

        merge_example();
        std::cout << std::endl;

        swap_example();
        std::cout << std::endl;

        filtered_iteration_example();
        std::cout << std::endl;

        batch_operations_example();
        std::cout << std::endl;

        performance_comparison();

        std::cout << "=== All examples completed successfully ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

## Compilation

To compile these examples:

```bash
cd /Users/gobindp/MyWork/crux/build/Release
g++ -std=c++17 -I../../include -L. -lbase iterator_examples.cpp -o iterator_examples
./iterator_examples
```

These examples demonstrate the full range of iterator and container APIs available in the Base Table system, showing both basic usage and advanced techniques for efficient data processing.
