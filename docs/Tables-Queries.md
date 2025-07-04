# Table Querying and Indexing

## Overview

The Table system provides powerful querying capabilities with SQL-like syntax and high-performance indexing for fast data retrieval. This guide covers query construction, index management, and optimization strategies.

## Table of Contents

1. [Query Basics](#query-basics)
2. [Query Operators](#query-operators)
3. [Complex Queries](#complex-queries)
4. [Indexing](#indexing)
5. [Query Optimization](#query-optimization)
6. [Performance Tips](#performance-tips)
7. [Examples](#examples)

## Query Basics

### Simple Queries

```cpp
#include "tables.h"
using namespace base;

// Create a basic query
TableQuery query;

// Single condition query
query.where("name", QueryOperator::Equal, std::string("John Doe"));
auto results = table->query(query);

// Iterate through results
for (const auto& row : results) {
    auto name = row.get_cell_value("name");
    if (name) {
        std::cout << "Found user: " << std::get<std::string>(*name) << std::endl;
    }
}
```

### Query Builder Pattern

```cpp
TableQuery query;
query.where("age", QueryOperator::GreaterThan, static_cast<std::int64_t>(21))
     .where("is_active", QueryOperator::Equal, true)
     .order_by("name", SortOrder::Ascending)
     .limit(10);

auto results = table->query(query);
```

## Query Operators

### Comparison Operators

```cpp
enum class QueryOperator {
    Equal,              // =
    NotEqual,           // !=
    LessThan,           // <
    LessThanOrEqual,    // <=
    GreaterThan,        // >
    GreaterThanOrEqual, // >=
    Like,               // LIKE (string matching)
    In                  // IN (value list)
};
```

### Operator Examples

```cpp
TableQuery query;

// Equality
query.where("status", QueryOperator::Equal, std::string("active"));

// Numeric comparisons
query.where("age", QueryOperator::GreaterThan, static_cast<std::int64_t>(18));
query.where("salary", QueryOperator::LessThanOrEqual, 100000.0);

// String matching (LIKE with wildcards)
query.where("email", QueryOperator::Like, std::string("%@company.com"));

// Inequality
query.where("department", QueryOperator::NotEqual, std::string("temp"));
```

### Sort Orders

```cpp
enum class SortOrder {
    Ascending,   // ASC
    Descending   // DESC
};

TableQuery query;
query.order_by("created_at", SortOrder::Descending)  // Most recent first
     .order_by("name", SortOrder::Ascending);         // Then by name A-Z
```

## Complex Queries

### Multiple Conditions

```cpp
TableQuery query;

// All conditions are AND-ed together
query.where("department", QueryOperator::Equal, std::string("engineering"))
     .where("level", QueryOperator::GreaterThanOrEqual, static_cast<std::int64_t>(3))
     .where("is_active", QueryOperator::Equal, true)
     .where("salary", QueryOperator::LessThan, 200000.0);

auto senior_engineers = table->query(query);
```

### Ordering and Limiting

```cpp
TableQuery query;

// Get top 5 highest paid active employees
query.where("is_active", QueryOperator::Equal, true)
     .order_by("salary", SortOrder::Descending)
     .limit(5);

auto top_earners = table->query(query);
```

### Range Queries

```cpp
TableQuery query;

// Find employees with salary between 50k and 100k
query.where("salary", QueryOperator::GreaterThanOrEqual, 50000.0)
     .where("salary", QueryOperator::LessThanOrEqual, 100000.0);

auto mid_range_salaries = table->query(query);
```

### Date Range Queries

```cpp
TableQuery query;

// Find records created in the last month
query.where("created_at", QueryOperator::GreaterThan,
           std::string("2025-06-01T00:00:00Z"))
     .where("created_at", QueryOperator::LessThan,
           std::string("2025-07-01T00:00:00Z"));

auto recent_records = table->query(query);
```

## Indexing

### Creating Indexes

```cpp
// Single column index
table->create_index("name_idx", {"name"});
table->create_index("email_idx", {"email"});
table->create_index("department_idx", {"department"});

// Composite index (multiple columns)
table->create_index("dept_level_idx", {"department", "level"});
table->create_index("name_created_idx", {"name", "created_at"});
```

### Index Types

The system supports the following index types:

1. **Single Column Indexes**: Fast lookups on individual columns
2. **Composite Indexes**: Multi-column indexes for complex queries
3. **Automatic Primary Key Index**: Automatically created for primary keys

### Using Indexes for Fast Lookups

```cpp
// Direct index lookup (fastest)
auto results = table->find_by_index("email_idx", {std::string("john@example.com")});

// Composite index lookup
auto dept_results = table->find_by_index("dept_level_idx", {
    std::string("engineering"),
    static_cast<std::int64_t>(4)
});
```

### Index Management

```cpp
// Check if index exists
if (table->has_index("name_idx")) {
    std::cout << "Name index exists" << std::endl;
}

// Drop an index
table->drop_index("old_index");

// List all indexes
auto indexes = table->get_indexes();
for (const auto& [name, index] : indexes) {
    std::cout << "Index: " << name << std::endl;
}
```

## Query Optimization

### Index Selection Strategy

1. **Single Column Queries**: Create single column indexes

   ```cpp
   // Query: WHERE department = 'engineering'
   table->create_index("dept_idx", {"department"});
   ```

2. **Multi-Column Queries**: Create composite indexes

   ```cpp
   // Query: WHERE department = 'engineering' AND level >= 3
   table->create_index("dept_level_idx", {"department", "level"});
   ```

3. **Sort Optimization**: Include ORDER BY columns in index
   ```cpp
   // Query: WHERE department = 'engineering' ORDER BY salary DESC
   table->create_index("dept_salary_idx", {"department", "salary"});
   ```

### Query Performance Analysis

```cpp
// Time query execution
auto start = std::chrono::high_resolution_clock::now();

TableQuery query;
query.where("department", QueryOperator::Equal, std::string("engineering"));
auto results = table->query(query);

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

std::cout << "Query executed in " << duration.count() << " microseconds" << std::endl;
std::cout << "Found " << results.size() << " results" << std::endl;
```

### Index vs. Full Table Scan

```cpp
// Without index - full table scan (slow for large tables)
TableQuery query_slow;
query_slow.where("email", QueryOperator::Equal, std::string("user@example.com"));
auto results_slow = table->query(query_slow);

// With index - direct lookup (fast)
table->create_index("email_idx", {"email"});
auto results_fast = table->find_by_index("email_idx", {std::string("user@example.com")});
```

## Performance Tips

### Index Best Practices

1. **Index Frequently Queried Columns**

   ```cpp
   // If you often query by status, create an index
   table->create_index("status_idx", {"status"});
   ```

2. **Use Composite Indexes for Multi-Column Queries**

   ```cpp
   // For queries like: WHERE dept = 'X' AND level = Y
   table->create_index("dept_level_idx", {"department", "level"});
   ```

3. **Consider Index Order for Composite Indexes**

   ```cpp
   // Most selective column first
   table->create_index("level_dept_idx", {"level", "department"});
   ```

4. **Don't Over-Index**
   - Indexes speed up queries but slow down insertions/updates
   - Only create indexes you actually use

### Query Optimization Strategies

1. **Use Specific Queries**

   ```cpp
   // Good: Specific condition
   query.where("department", QueryOperator::Equal, std::string("engineering"));

   // Bad: Overly broad condition
   query.where("department", QueryOperator::NotEqual, std::string(""));
   ```

2. **Limit Results When Possible**

   ```cpp
   query.where("is_active", QueryOperator::Equal, true)
        .order_by("created_at", SortOrder::Descending)
        .limit(100);  // Don't load more data than needed
   ```

3. **Use Index Lookups for Exact Matches**

   ```cpp
   // Fast: Direct index lookup
   auto user = table->find_by_index("email_idx", {email});

   // Slower: Query with condition
   TableQuery query;
   query.where("email", QueryOperator::Equal, email);
   auto user_results = table->query(query);
   ```

## Examples

### User Management System

```cpp
// Setup
auto schema = std::make_unique<TableSchema>("users", 1);
schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
schema->add_column(ColumnDefinition("username", ColumnType::String, false));
schema->add_column(ColumnDefinition("email", ColumnType::String, false));
schema->add_column(ColumnDefinition("department", ColumnType::String, true));
schema->add_column(ColumnDefinition("level", ColumnType::Integer, true));
schema->add_column(ColumnDefinition("salary", ColumnType::Double, true));
schema->add_column(ColumnDefinition("is_active", ColumnType::Boolean, true));
schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false));
schema->set_primary_key({"id"});

auto table = std::make_unique<Table>(std::move(schema));

// Create indexes for common queries
table->create_index("username_idx", {"username"});
table->create_index("email_idx", {"email"});
table->create_index("dept_level_idx", {"department", "level"});
table->create_index("active_created_idx", {"is_active", "created_at"});

// Query Examples

// 1. Find user by username (fast index lookup)
auto user = table->find_by_index("username_idx", {std::string("alice")});

// 2. Find all active senior engineers
TableQuery senior_engineers;
senior_engineers.where("department", QueryOperator::Equal, std::string("engineering"))
                .where("level", QueryOperator::GreaterThanOrEqual, static_cast<std::int64_t>(4))
                .where("is_active", QueryOperator::Equal, true)
                .order_by("level", SortOrder::Descending);

auto results = table->query(senior_engineers);

// 3. Find recently created active users
TableQuery recent_users;
recent_users.where("is_active", QueryOperator::Equal, true)
            .where("created_at", QueryOperator::GreaterThan, std::string("2025-06-01T00:00:00Z"))
            .order_by("created_at", SortOrder::Descending)
            .limit(50);

auto recent = table->query(recent_users);

// 4. Find high earners
TableQuery high_earners;
high_earners.where("salary", QueryOperator::GreaterThan, 150000.0)
            .where("is_active", QueryOperator::Equal, true)
            .order_by("salary", SortOrder::Descending);

auto top_paid = table->query(high_earners);

// 5. Department statistics
TableQuery dept_query;
dept_query.where("department", QueryOperator::Equal, std::string("engineering"));
auto eng_employees = table->query(dept_query);

std::cout << "Engineering department has " << eng_employees.size() << " employees" << std::endl;
```

### E-commerce Product Catalog

```cpp
// Product table setup
auto product_schema = std::make_unique<TableSchema>("products", 1);
product_schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
product_schema->add_column(ColumnDefinition("name", ColumnType::String, false));
product_schema->add_column(ColumnDefinition("category", ColumnType::String, false));
product_schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
product_schema->add_column(ColumnDefinition("stock_quantity", ColumnType::Integer, false));
product_schema->add_column(ColumnDefinition("is_available", ColumnType::Boolean, false));
product_schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false));
product_schema->set_primary_key({"id"});

auto products = std::make_unique<Table>(std::move(product_schema));

// Create indexes for e-commerce queries
products->create_index("category_idx", {"category"});
products->create_index("price_idx", {"price"});
products->create_index("available_category_idx", {"is_available", "category"});
products->create_index("category_price_idx", {"category", "price"});

// E-commerce query examples

// 1. Browse products by category
TableQuery category_browse;
category_browse.where("category", QueryOperator::Equal, std::string("electronics"))
               .where("is_available", QueryOperator::Equal, true)
               .order_by("name", SortOrder::Ascending);

auto electronics = products->query(category_browse);

// 2. Search by price range
TableQuery price_range;
price_range.where("price", QueryOperator::GreaterThanOrEqual, 100.0)
           .where("price", QueryOperator::LessThanOrEqual, 500.0)
           .where("is_available", QueryOperator::Equal, true)
           .order_by("price", SortOrder::Ascending);

auto mid_range_products = products->query(price_range);

// 3. Find best sellers (assuming stock indicates popularity)
TableQuery popular;
popular.where("is_available", QueryOperator::Equal, true)
       .order_by("stock_quantity", SortOrder::Descending)
       .limit(20);

auto best_sellers = products->query(popular);

// 4. Low stock alert
TableQuery low_stock;
low_stock.where("stock_quantity", QueryOperator::LessThan, static_cast<std::int64_t>(10))
         .where("is_available", QueryOperator::Equal, true)
         .order_by("stock_quantity", SortOrder::Ascending);

auto reorder_needed = products->query(low_stock);
```

---

_See also: [Table API Reference](Tables-API.md), [Schema Management](Tables-Schema.md), [Performance Guide](Tables-Performance.md)_
