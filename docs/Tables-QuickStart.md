# Table System - Quick Start Guide

## Introduction

This guide will get you up and running with the Base Table system in minutes. The Table system provides a powerful, thread-safe data management solution with schema flexibility and high performance.

## Prerequisites

- C++20 compatible compiler
- Base framework installed and configured
- nlohmann/json dependency (handled by Conan)

## Basic Setup

### 1. Include Headers

```cpp
#include "tables.h"
#include "logger.h"  // For logging (optional)
using namespace base;
```

### 2. Initialize Logger (Optional)

```cpp
base::Logger::init();  // For debugging and monitoring
```

## Creating Your First Table

### Step 1: Define Schema

```cpp
// Create a schema for a user table
auto schema = std::make_unique<TableSchema>("users", 1);

// Add columns
schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));        // Required
schema->add_column(ColumnDefinition("name", ColumnType::String, false));      // Required
schema->add_column(ColumnDefinition("email", ColumnType::String, true));      // Optional
schema->add_column(ColumnDefinition("age", ColumnType::Integer, true));       // Optional
schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, true)); // Optional

// Set primary key
schema->set_primary_key({"id"});
```

### Step 2: Create Table

```cpp
// Create table with schema
auto table = std::make_unique<Table>(std::move(schema));

base::Logger::info("Created table: {}", table->get_schema().get_name());
```

## Basic Operations

### Inserting Data

```cpp
// Create row data
std::unordered_map<std::string, CellValue> user1 = {
    {"id", static_cast<std::int64_t>(1)},
    {"name", std::string("Alice Johnson")},
    {"email", std::string("alice@example.com")},
    {"age", static_cast<std::int64_t>(30)}
};

std::unordered_map<std::string, CellValue> user2 = {
    {"id", static_cast<std::int64_t>(2)},
    {"name", std::string("Bob Smith")},
    {"email", std::string("bob@example.com")},
    {"age", static_cast<std::int64_t>(25)}
};

// Insert rows
auto row1_id = table->insert_row(user1);
auto row2_id = table->insert_row(user2);

base::Logger::info("Inserted users with IDs: {} and {}", row1_id, row2_id);
```

### Querying Data

```cpp
// Get all rows
auto all_users = table->get_all_rows();
base::Logger::info("Total users: {}", all_users.size());

// Get specific row
auto user = table->get_row(row1_id);
if (user.has_value()) {
    auto name = std::get<std::string>(user->get_value("name"));
    base::Logger::info("Found user: {}", name);
}

// Query with conditions
TableQuery query;
query.where("age", QueryOperator::GreaterThan, static_cast<std::int64_t>(27));
auto older_users = table->query(query);
base::Logger::info("Users over 27: {}", older_users.size());
```

### Updating Data

```cpp
// Update user information
std::unordered_map<std::string, CellValue> updates = {
    {"email", std::string("alice.johnson@newcompany.com")},
    {"age", static_cast<std::int64_t>(31)}
};

if (table->update_row(row1_id, updates)) {
    base::Logger::info("Updated user successfully");
}
```

### Deleting Data

```cpp
// Delete a user
if (table->delete_row(row2_id)) {
    base::Logger::info("Deleted user successfully");
}
```

## Adding Indexes for Performance

```cpp
// Create indexes for faster queries
table->create_index("name_idx", {"name"});           // Single column
table->create_index("name_email_idx", {"name", "email"}); // Multi-column

base::Logger::info("Created indexes for faster queries");

// Query using index
auto indexed_results = table->find_by_index("name_idx", {std::string("Alice Johnson")});
```

## Advanced Querying

```cpp
// Complex query with multiple conditions
TableQuery advanced_query;
advanced_query
    .where("age", QueryOperator::GreaterThanOrEqual, static_cast<std::int64_t>(25))
    .where("name", QueryOperator::Like, std::string("%Alice%"))
    .order_by("age", false)  // Descending order
    .limit(10);

auto results = table->query(advanced_query);
```

## Working with Transactions

```cpp
// Begin a transaction
auto transaction = table->begin_transaction();
transaction->begin();

try {
    // Perform multiple operations
    table->insert_row(user1);
    table->insert_row(user2);

    // Commit if all operations succeed
    transaction->commit();
    base::Logger::info("Transaction committed successfully");

} catch (const std::exception& e) {
    // Rollback on error
    transaction->rollback();
    base::Logger::error("Transaction failed, rolled back: {}", e.what());
}
```

## Serialization and Persistence

```cpp
// Serialize table to JSON
auto json_data = table->to_json();
base::Logger::info("Serialized table ({} bytes)", json_data.length());

// Save to file
if (table->save_to_file("users.json")) {
    base::Logger::info("Table saved to file");
}

// Load from file
auto loaded_table = createEmptyTable(); // Helper function
if (loaded_table->load_from_file("users.json")) {
    base::Logger::info("Table loaded from file");
}
```

## Change Notifications

```cpp
// Register for change events
table->add_change_callback("user_changes", [](const ChangeEvent& event) {
    switch (event.type) {
        case ChangeType::RowInserted:
            base::Logger::info("Row inserted in table: {}", event.table_name);
            break;
        case ChangeType::RowUpdated:
            base::Logger::info("Row updated in table: {}", event.table_name);
            break;
        case ChangeType::RowDeleted:
            base::Logger::info("Row deleted from table: {}", event.table_name);
            break;
        default:
            break;
    }
});
```

## Error Handling

```cpp
try {
    // Table operations
    auto result = table->insert_row(user_data);

} catch (const std::invalid_argument& e) {
    base::Logger::error("Invalid data: {}", e.what());
} catch (const std::runtime_error& e) {
    base::Logger::error("Runtime error: {}", e.what());
} catch (const std::exception& e) {
    base::Logger::error("Unexpected error: {}", e.what());
}
```

## Complete Example

Here's a complete working example:

```cpp
#include "tables.h"
#include "logger.h"

using namespace base;

int main() {
    // Initialize
    base::Logger::init();

    try {
        // Create schema
        auto schema = std::make_unique<TableSchema>("products", 1);
        schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("name", ColumnType::String, false));
        schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
        schema->add_column(ColumnDefinition("category", ColumnType::String, true));
        schema->set_primary_key({"id"});

        // Create table
        auto table = std::make_unique<Table>(std::move(schema));

        // Insert products
        table->insert_row({
            {"id", static_cast<std::int64_t>(1)},
            {"name", std::string("Laptop")},
            {"price", 999.99},
            {"category", std::string("Electronics")}
        });

        table->insert_row({
            {"id", static_cast<std::int64_t>(2)},
            {"name", std::string("Book")},
            {"price", 29.99},
            {"category", std::string("Education")}
        });

        // Create index
        table->create_index("category_idx", {"category"});

        // Query expensive items
        TableQuery query;
        query.where("price", QueryOperator::GreaterThan, 50.0)
             .order_by("price", false);

        auto expensive_items = table->query(query);
        base::Logger::info("Found {} expensive items", expensive_items.size());

        // Get statistics
        auto stats = table->get_statistics();
        base::Logger::info("Table stats - Rows: {}, Inserts: {}",
                          stats.row_count, stats.total_inserts);

        base::Logger::info("Quick start example completed successfully!");

    } catch (const std::exception& e) {
        base::Logger::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
```

## Next Steps

- [API Reference](Tables-API.md) - Complete API documentation
- [Schema Management](Tables-Schema.md) - Advanced schema features
- [Performance Tuning](Tables-Performance.md) - Optimization techniques
- [Best Practices](Tables-BestPractices.md) - Design patterns and tips

## Common Patterns

### Factory Pattern for Table Creation

```cpp
class UserTableFactory {
public:
    static std::unique_ptr<Table> create() {
        auto schema = std::make_unique<TableSchema>("users", 1);
        schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("name", ColumnType::String, false));
        schema->add_column(ColumnDefinition("email", ColumnType::String, true));
        schema->set_primary_key({"id"});
        return std::make_unique<Table>(std::move(schema));
    }
};
```

### Repository Pattern

```cpp
class UserRepository {
private:
    std::unique_ptr<Table> table_;

public:
    UserRepository() : table_(UserTableFactory::create()) {}

    std::size_t addUser(const std::string& name, const std::string& email) {
        return table_->insert_row({
            {"id", generateId()},
            {"name", name},
            {"email", email}
        });
    }

    std::optional<TableRow> findUserByEmail(const std::string& email) {
        TableQuery query;
        query.where("email", QueryOperator::Equal, email);
        auto results = table_->query(query);
        return results.empty() ? std::nullopt : std::make_optional(results[0]);
    }
};
```

---

_Happy coding with the Base Table system!_ 🚀
