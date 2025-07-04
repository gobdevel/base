# Table Iterators and Container APIs

This document covers the iterator support and container-style APIs provided by the Base framework's Table data structure.

## Overview

The Table class provides comprehensive iterator support and container manipulation APIs, making it compatible with STL algorithms and enabling modern C++ idioms like range-based for loops.

## Table of Contents

- [Iterator Types](#iterator-types)
- [Iterator Operations](#iterator-operations)
- [Range-Based For Loops](#range-based-for-loops)
- [Copy Operations](#copy-operations)
- [Move Operations](#move-operations)
- [Table Utility Methods](#table-utility-methods)
- [Performance Considerations](#performance-considerations)
- [Examples](#examples)
- [Best Practices](#best-practices)

## Iterator Types

### TableIterator

Forward iterator providing read-only access to table rows.

```cpp
class TableIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = TableRow;
    using difference_type = std::ptrdiff_t;
    using pointer = const TableRow*;
    using reference = const TableRow&;

    // Standard iterator operations
    reference operator*() const;
    pointer operator->() const;
    TableIterator& operator++();
    TableIterator operator++(int);
    bool operator==(const TableIterator& other) const;
    bool operator!=(const TableIterator& other) const;
};
```

### TableConstIterator

Const forward iterator providing read-only access to table rows.

```cpp
class TableConstIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const TableRow;
    using difference_type = std::ptrdiff_t;
    using pointer = const TableRow*;
    using reference = const TableRow&;

    // Standard iterator operations (same as TableIterator)
};
```

## Iterator Operations

### Basic Iterator Access

```cpp
// Iterator type aliases
using iterator = TableIterator;
using const_iterator = TableConstIterator;

// Standard STL-style iterator methods
iterator begin();
iterator end();
const_iterator begin() const;
const_iterator end() const;
const_iterator cbegin() const;
const_iterator cend() const;

// Alternative naming for clarity
iterator rows_begin() { return begin(); }
iterator rows_end() { return end(); }
const_iterator rows_begin() const { return begin(); }
const_iterator rows_end() const { return end(); }
```

### Iterator Usage Examples

```cpp
auto table = createTable();

// Using iterators explicitly
for (auto it = table->begin(); it != table->end(); ++it) {
    const TableRow& row = *it;
    std::cout << "Row ID: " << row.get_id() << std::endl;
}

// Using const iterators
for (auto it = table->cbegin(); it != table->cend(); ++it) {
    // Read-only access to rows
    auto value = it->get_value("name");
}
```

## Range-Based For Loops

The Table class fully supports range-based for loops, providing a clean and modern syntax for iterating over rows.

```cpp
auto table = createTable();

// Range-based for loop (preferred)
for (const auto& row : *table) {
    std::cout << "Row ID: " << row.get_id() << std::endl;
    auto name = row.get_value("name");
    // Process row...
}

// Const table iteration
const Table& const_table = *table;
for (const auto& row : const_table) {
    // Read-only access
    auto id = row.get_id();
}
```

## Copy Operations

### Copy Constructor

Creates a deep copy of a table, including all rows, indexes, schema, and configuration.

```cpp
Table(const Table& other);
```

**Features:**

- Deep copies all table data
- Recreates all indexes
- Preserves schema and configuration
- Copies change callbacks
- Thread-safe operation

```cpp
auto original_table = createTable();
// ... populate table ...

// Create a copy
Table copied_table(*original_table);

// Verify independence
ASSERT_EQ(copied_table.get_row_count(), original_table->get_row_count());
ASSERT_NE(&copied_table, original_table.get()); // Different objects
```

### Copy Assignment Operator

Assigns the contents of one table to another.

```cpp
Table& operator=(const Table& other);
```

```cpp
auto table1 = createTable();
auto table2 = createTable();

// ... populate tables ...

// Copy assignment
*table1 = *table2;  // table1 now contains table2's data
```

## Move Operations

### Move Constructor

Efficiently transfers ownership of table resources without copying.

```cpp
Table(Table&& other) noexcept;
```

**Features:**

- O(1) operation - no data copying
- `noexcept` guarantee for optimal performance
- Leaves source table in valid but empty state

```cpp
auto create_populated_table() -> std::unique_ptr<Table> {
    auto table = createTable();
    // ... populate table ...
    return table;
}

// Move constructor used automatically
auto table = create_populated_table();
```

### Move Assignment Operator

Moves table contents from one table to another.

```cpp
Table& operator=(Table&& other) noexcept;
```

```cpp
auto table1 = createTable();
auto table2 = createTable();

// Move assignment
*table1 = std::move(*table2);  // table1 gets table2's data, table2 becomes empty
```

## Table Utility Methods

### clear()

Removes all rows from the table while preserving schema and configuration.

```cpp
void clear();
```

**Features:**

- Removes all rows
- Clears all indexes
- Recreates primary key index if defined
- Fires deletion events for all removed rows
- Thread-safe operation

```cpp
table->clear();
ASSERT_TRUE(table->empty());
ASSERT_EQ(table->get_row_count(), 0);
```

### empty()

Checks if the table contains no rows.

```cpp
bool empty() const;
```

```cpp
if (table->empty()) {
    std::cout << "Table has no data" << std::endl;
}
```

### clone()

Creates a complete deep copy of the table, returning a new Table instance.

```cpp
std::unique_ptr<Table> clone() const;
```

**Features:**

- Deep copies all data and configuration
- Returns a new, independent table
- Preserves all indexes and callbacks
- Thread-safe operation

```cpp
auto original = createTable();
// ... populate table ...

auto cloned = original->clone();
ASSERT_EQ(cloned->get_row_count(), original->get_row_count());

// Independent modifications
cloned->clear();
ASSERT_NE(cloned->get_row_count(), original->get_row_count());
```

### merge_from()

Merges rows from another compatible table.

```cpp
void merge_from(const Table& other);
```

**Features:**

- Schema compatibility validation
- Handles primary key conflicts by adjusting row IDs
- Preserves data integrity
- Fires change events for merged rows

```cpp
auto table1 = createTable();
auto table2 = createTable();

// Insert different data with non-conflicting primary keys
table1->insert_row({{"id", 1L}, {"name", std::string("Alice")}});
table2->insert_row({{"id", 2L}, {"name", std::string("Bob")}});

// Merge table2 into table1
table1->merge_from(*table2);

ASSERT_EQ(table1->get_row_count(), 2);  // Now has both rows
ASSERT_EQ(table2->get_row_count(), 1);  // Original unchanged
```

### swap()

Efficiently swaps the contents of two tables.

```cpp
void swap(Table& other) noexcept;
```

**Features:**

- O(1) operation
- `noexcept` guarantee
- Swaps all data, indexes, and configuration
- Thread-safe operation

```cpp
auto table1 = createTable();
auto table2 = createTable();

// Populate with different data
table1->insert_row({{"id", 1L}, {"name", std::string("Alice")}});
table2->insert_row({{"id", 2L}, {"name", std::string("Bob")}});

auto count1 = table1->get_row_count();
auto count2 = table2->get_row_count();

// Swap contents
table1->swap(*table2);

ASSERT_EQ(table1->get_row_count(), count2);
ASSERT_EQ(table2->get_row_count(), count1);
```

## Performance Considerations

### Iterator Performance

- **Forward Iteration**: O(n) for complete traversal
- **Memory Overhead**: Minimal - iterators store only position information
- **Thread Safety**: Iterators are safe for concurrent read operations

### Copy vs Move Performance

| Operation        | Time Complexity | Memory Usage | Use Case                         |
| ---------------- | --------------- | ------------ | -------------------------------- |
| Copy Constructor | O(n)            | 2x original  | When you need independent copies |
| Move Constructor | O(1)            | 1x original  | When transferring ownership      |
| Copy Assignment  | O(n)            | 2x original  | When copying to existing table   |
| Move Assignment  | O(1)            | 1x original  | When moving to existing table    |

### Utility Method Performance

| Method         | Time Complexity | Thread Safety | Notes                    |
| -------------- | --------------- | ------------- | ------------------------ |
| `clear()`      | O(n)            | Yes           | Fires change events      |
| `empty()`      | O(1)            | Yes           | Simple size check        |
| `clone()`      | O(n)            | Yes           | Complete deep copy       |
| `merge_from()` | O(m)            | Yes           | m = rows in source table |
| `swap()`       | O(1)            | Yes           | Constant time exchange   |

## Examples

### Complete Iterator Example

```cpp
#include "tables.h"
#include <iostream>
#include <algorithm>

void demonstrate_iterators() {
    // Create and populate table
    auto schema = std::make_unique<TableSchema>("users", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer));
    schema->add_column(ColumnDefinition("name", ColumnType::String));
    schema->add_column(ColumnDefinition("age", ColumnType::Integer));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Insert sample data
    table->insert_row({{"id", 1L}, {"name", std::string("Alice")}, {"age", 25L}});
    table->insert_row({{"id", 2L}, {"name", std::string("Bob")}, {"age", 30L}});
    table->insert_row({{"id", 3L}, {"name", std::string("Charlie")}, {"age", 35L}});

    // Range-based for loop
    std::cout << "All users:" << std::endl;
    for (const auto& row : *table) {
        auto name = std::get<std::string>(row.get_value("name"));
        auto age = std::get<std::int64_t>(row.get_value("age"));
        std::cout << "  " << name << " (age " << age << ")" << std::endl;
    }

    // STL algorithm compatibility
    auto adult_count = std::count_if(table->begin(), table->end(),
        [](const TableRow& row) {
            auto age = std::get<std::int64_t>(row.get_value("age"));
            return age >= 30;
        });

    std::cout << "Adults (30+): " << adult_count << std::endl;

    // Iterator arithmetic
    if (table->begin() != table->end()) {
        auto first_row = *table->begin();
        std::cout << "First user ID: " << first_row.get_id() << std::endl;
    }
}
```

### Table Manipulation Example

```cpp
void demonstrate_table_operations() {
    auto table1 = createTestTable();
    auto table2 = createTestTable();

    // Populate tables
    table1->insert_row({{"id", 1L}, {"name", std::string("Alice")}});
    table1->insert_row({{"id", 2L}, {"name", std::string("Bob")}});

    table2->insert_row({{"id", 3L}, {"name", std::string("Charlie")}});
    table2->insert_row({{"id", 4L}, {"name", std::string("David")}});

    std::cout << "Table1 rows: " << table1->get_row_count() << std::endl;
    std::cout << "Table2 rows: " << table2->get_row_count() << std::endl;

    // Clone operation
    auto cloned = table1->clone();
    std::cout << "Cloned rows: " << cloned->get_row_count() << std::endl;

    // Merge operation
    table1->merge_from(*table2);
    std::cout << "After merge, Table1 rows: " << table1->get_row_count() << std::endl;

    // Copy operation
    Table copied_table(*table1);
    std::cout << "Copied table rows: " << copied_table.get_row_count() << std::endl;

    // Clear operation
    copied_table.clear();
    std::cout << "After clear: " << copied_table.get_row_count() << std::endl;
    std::cout << "Is empty: " << (copied_table.empty() ? "yes" : "no") << std::endl;

    // Swap operation
    auto original_count1 = table1->get_row_count();
    auto original_count2 = table2->get_row_count();

    table1->swap(*table2);

    std::cout << "After swap:" << std::endl;
    std::cout << "  Table1 rows: " << table1->get_row_count() << std::endl;
    std::cout << "  Table2 rows: " << table2->get_row_count() << std::endl;
}
```

## Best Practices

### Iterator Usage

1. **Prefer Range-Based For Loops**: More readable and less error-prone
2. **Use const iterators**: When you don't need to modify data
3. **Avoid Iterator Invalidation**: Don't modify table structure during iteration
4. **Thread Safety**: Use appropriate locking for concurrent access

```cpp
// Good: Range-based for loop
for (const auto& row : *table) {
    process_row(row);
}

// Less ideal: Manual iterator handling
for (auto it = table->begin(); it != table->end(); ++it) {
    process_row(*it);
}
```

### Copy vs Move

1. **Use Move When Possible**: For better performance
2. **Copy When Independence Required**: When you need separate table instances
3. **Consider clone()**: For explicit deep copying

```cpp
// Good: Move semantics
auto table = std::move(create_table());

// Good: Explicit cloning when needed
auto backup = table->clone();

// Less efficient: Unnecessary copy
auto table_copy = *table;  // Only if you really need a copy
```

### Memory Management

1. **Use Smart Pointers**: Leverage RAII for automatic cleanup
2. **Monitor Memory Usage**: Large tables can consume significant memory
3. **Clear When Appropriate**: Use `clear()` instead of creating new tables

```cpp
// Good: Smart pointer management
auto table = std::make_unique<Table>(std::move(schema));

// Good: Reuse existing table
table->clear();
// ... repopulate ...

// Less efficient: Creating new table
table = std::make_unique<Table>(std::move(new_schema));
```

### Thread Safety

1. **Enable Concurrent Access**: For multi-threaded applications
2. **Use Appropriate Locking**: When needed for data consistency
3. **Avoid Long-Running Iterations**: In concurrent environments

```cpp
// Enable concurrent access
table->enable_concurrent_access(true);

// Safe concurrent iteration
{
    std::shared_lock lock(table_mutex);
    for (const auto& row : *table) {
        // Process row safely
    }
}
```

## Conclusion

The Table iterator and container APIs provide a modern, efficient, and thread-safe interface for working with table data. By leveraging STL-compatible iterators and providing comprehensive copy/move semantics, the Table class integrates seamlessly with modern C++ code while maintaining high performance and data integrity.

For more information, see:

- [Tables API Reference](Tables-API.md)
- [Tables Performance Guide](Tables-Performance.md)
- [Tables Best Practices](Tables-BestPractices.md)
