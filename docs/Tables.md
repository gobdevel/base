# Tables Documentation

## Overview

The Base Table system provides a comprehensive, high-performance, thread-safe data structure for managing structured data with schema evolution, indexing, querying, and persistence capabilities.

## Table of Contents

1. [Quick Start Guide](Tables-QuickStart.md)
2. [Table API Reference](Tables-API.md)
3. [Schema Management](Tables-Schema.md)
4. [Querying and Indexing](Tables-Queries.md)
5. [Transactions and Concurrency](Tables-Transactions.md)
6. [Serialization and Persistence](Tables-Persistence.md)
7. [Iterators and Container APIs](Tables-Iterators.md)
8. [Performance Tuning](Tables-Performance.md)
9. [Best Practices](Tables-BestPractices.md)
10. [Examples and Tutorials](Tables-Examples.md)

## Key Features

### ✅ **Core Functionality**

- **Schema Definition**: Both compile-time and runtime schema support
- **Type Safety**: Type-safe cell values using `std::variant`
- **Thread Safety**: Concurrent access with `shared_mutex` and atomic operations
- **CRUD Operations**: Create, Read, Update, Delete with full validation

### ✅ **Advanced Features**

- **Indexing**: Single and multi-column indexes for fast lookups
- **Querying**: SQL-like query builder with conditions, ordering, and limits
- **Schema Evolution**: Version-controlled schema changes with backward compatibility
- **Transactions**: ACID-like transactions with rollback capabilities
- **Persistence**: JSON and binary serialization with file storage
- **Change Events**: Callback system for data change notifications
- **Iterators**: STL-compatible iterators with range-based for loop support
- **Container APIs**: Copy, move, clone, merge, and swap operations

### ✅ **Performance & Reliability**

- **High Performance**: Optimized for large datasets with efficient memory usage
- **Thread Safety**: Safe concurrent operations across multiple threads
- **Memory Efficient**: Smart memory management with minimal overhead
- **Exception Safety**: Robust error handling and recovery
- **Move Semantics**: Efficient resource management with move constructors

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Table System                             │
├─────────────────────────────────────────────────────────────────┤
│  TableFactory  │  Table  │  TableSchema  │  TableTransaction   │
├─────────────────────────────────────────────────────────────────┤
│  TableQuery    │  TableRow   │  TableIndex  │  ChangeEvents    │
├─────────────────────────────────────────────────────────────────┤
│  CellValue     │  ColumnDefinition        │  Constraints      │
├─────────────────────────────────────────────────────────────────┤
│              Base Framework (Logger, Messaging)                │
└─────────────────────────────────────────────────────────────────┘
```

## Quick Example

```cpp
#include "tables.h"
using namespace base;

// Create schema
auto schema = std::make_unique<TableSchema>("users", 1);
schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
schema->add_column(ColumnDefinition("name", ColumnType::String, false));
schema->add_column(ColumnDefinition("email", ColumnType::String, true));
schema->set_primary_key({"id"});

// Create table
auto table = std::make_unique<Table>(std::move(schema));

// Insert data
std::unordered_map<std::string, CellValue> row = {
    {"id", static_cast<std::int64_t>(1)},
    {"name", std::string("John Doe")},
    {"email", std::string("john@example.com")}
};
auto row_id = table->insert_row(row);

// Query data
TableQuery query;
query.where("name", QueryOperator::Equal, std::string("John Doe"));
auto results = table->query(query);

// Create index for faster queries
table->create_index("name_idx", {"name"});

// Serialize to JSON
auto json_data = table->to_json();
```

## Integration

The Table system seamlessly integrates with the Base framework:

- **Logging**: Uses Base logger for debugging and monitoring
- **Threading**: Compatible with Base application threading model
- **Configuration**: Can be configured through Base config system
- **Error Handling**: Follows Base framework error handling patterns

## Dependencies

- **C++20**: Modern C++ features for performance and safety
- **nlohmann/json**: JSON serialization and deserialization
- **Base Framework**: Logger, threading, and utility components

## Getting Started

See [Quick Start Guide](Tables-QuickStart.md) for step-by-step instructions on setting up and using the Table system in your application.

## Performance

The Table system is designed for high performance:

- **Insert Performance**: ~1000 operations/second per thread
- **Query Performance**: Sub-millisecond queries with proper indexing
- **Memory Efficiency**: Minimal overhead per row (~64 bytes + data)
- **Concurrent Access**: Scales well with multiple reader/writer threads

For detailed performance tuning, see [Performance Guide](Tables-Performance.md).

## Support

- **Examples**: Complete working examples in `examples/table_example.cpp`
- **Unit Tests**: Comprehensive test suite with 24+ test cases
- **Documentation**: Detailed API reference and tutorials
- **Integration**: Seamless integration with Base framework

---

_Last updated: July 3, 2025_
