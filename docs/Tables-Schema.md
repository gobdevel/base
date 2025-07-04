# Table Schema Management

## Overview

The Table Schema system provides comprehensive schema definition, validation, and evolution capabilities. It supports both compile-time and runtime schema definitions with full type safety and versioning.

## Table of Contents

1. [Schema Basics](#schema-basics)
2. [Column Definitions](#column-definitions)
3. [Data Types](#data-types)
4. [Constraints](#constraints)
5. [Primary Keys](#primary-keys)
6. [Schema Evolution](#schema-evolution)
7. [Schema Validation](#schema-validation)
8. [Best Practices](#best-practices)

## Schema Basics

### Creating a Schema

```cpp
#include "tables.h"
using namespace base;

// Create a new schema with name and version
auto schema = std::make_unique<TableSchema>("users", 1);
```

### Schema Properties

- **Name**: Unique identifier for the table
- **Version**: Schema version for evolution tracking
- **Columns**: Ordered list of column definitions
- **Primary Key**: Unique identifier columns
- **Constraints**: Validation rules and relationships

## Column Definitions

### Basic Column Creation

```cpp
// Column with name, type, and nullability
ColumnDefinition id_col("id", ColumnType::Integer, false);     // Required
ColumnDefinition name_col("name", ColumnType::String, false);  // Required
ColumnDefinition email_col("email", ColumnType::String, true); // Optional
```

### Adding Columns to Schema

```cpp
schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
schema->add_column(ColumnDefinition("name", ColumnType::String, false));
schema->add_column(ColumnDefinition("email", ColumnType::String, true));
schema->add_column(ColumnDefinition("age", ColumnType::Integer, true));
schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, true));
```

### Column Properties

Each column has the following properties:

- **Name**: Unique column identifier within the table
- **Type**: Data type (see [Data Types](#data-types))
- **Nullable**: Whether the column can contain NULL values
- **Position**: Automatic ordering based on addition sequence

## Data Types

### Supported Types

```cpp
enum class ColumnType {
    Integer,    // std::int64_t
    Double,     // double
    String,     // std::string
    Boolean,    // bool
    DateTime,   // std::string (ISO 8601 format)
    Blob        // std::vector<std::uint8_t>
};
```

### Type Usage Examples

```cpp
// Integer column for IDs, counts, etc.
schema->add_column(ColumnDefinition("user_id", ColumnType::Integer, false));

// String column for text data
schema->add_column(ColumnDefinition("username", ColumnType::String, false));

// Double column for floating-point numbers
schema->add_column(ColumnDefinition("salary", ColumnType::Double, true));

// Boolean column for flags
schema->add_column(ColumnDefinition("is_active", ColumnType::Boolean, true));

// DateTime column for timestamps (ISO 8601 format)
schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, true));

// Blob column for binary data
schema->add_column(ColumnDefinition("avatar", ColumnType::Blob, true));
```

### Type Conversion

The system automatically handles type conversion for `CellValue`:

```cpp
// Inserting different types
std::unordered_map<std::string, CellValue> row = {
    {"user_id", static_cast<std::int64_t>(123)},
    {"username", std::string("alice")},
    {"salary", 75000.50},
    {"is_active", true},
    {"created_at", std::string("2025-01-15T10:30:00Z")},
    {"avatar", std::vector<std::uint8_t>{0x89, 0x50, 0x4E, 0x47}}
};
```

## Constraints

### Nullable Constraints

```cpp
// Required columns (NOT NULL)
schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
schema->add_column(ColumnDefinition("name", ColumnType::String, false));

// Optional columns (NULLABLE)
schema->add_column(ColumnDefinition("email", ColumnType::String, true));
schema->add_column(ColumnDefinition("phone", ColumnType::String, true));
```

### Validation Rules

The schema automatically validates:

- **Type Safety**: Ensures values match column types
- **Null Constraints**: Validates required vs. optional columns
- **Primary Key Uniqueness**: Enforces unique primary key values
- **Column Existence**: Validates column names exist in schema

## Primary Keys

### Setting Primary Keys

```cpp
// Single column primary key
schema->set_primary_key({"id"});

// Composite primary key
schema->set_primary_key({"company_id", "user_id"});
```

### Primary Key Rules

- **Uniqueness**: Primary key values must be unique across all rows
- **Non-Null**: Primary key columns cannot be NULL
- **Immutable**: Primary key values cannot be changed after insertion
- **Auto-Validation**: System automatically validates uniqueness

### Examples

```cpp
// User table with single primary key
auto user_schema = std::make_unique<TableSchema>("users", 1);
user_schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
user_schema->add_column(ColumnDefinition("name", ColumnType::String, false));
user_schema->set_primary_key({"id"});

// User-Company relationship with composite primary key
auto membership_schema = std::make_unique<TableSchema>("memberships", 1);
membership_schema->add_column(ColumnDefinition("user_id", ColumnType::Integer, false));
membership_schema->add_column(ColumnDefinition("company_id", ColumnType::Integer, false));
membership_schema->add_column(ColumnDefinition("role", ColumnType::String, false));
membership_schema->set_primary_key({"user_id", "company_id"});
```

## Schema Evolution

### Version Management

```cpp
// Create initial schema (v1)
auto schema_v1 = std::make_unique<TableSchema>("products", 1);
schema_v1->add_column(ColumnDefinition("id", ColumnType::Integer, false));
schema_v1->add_column(ColumnDefinition("name", ColumnType::String, false));
schema_v1->add_column(ColumnDefinition("price", ColumnType::Double, false));
schema_v1->set_primary_key({"id"});

// Create evolved schema (v2) with additional columns
auto schema_v2 = std::make_unique<TableSchema>("products", 2);
schema_v2->add_column(ColumnDefinition("id", ColumnType::Integer, false));
schema_v2->add_column(ColumnDefinition("name", ColumnType::String, false));
schema_v2->add_column(ColumnDefinition("price", ColumnType::Double, false));
schema_v2->add_column(ColumnDefinition("description", ColumnType::String, true)); // New column
schema_v2->add_column(ColumnDefinition("category_id", ColumnType::Integer, true)); // New column
schema_v2->set_primary_key({"id"});
```

### Applying Schema Evolution

```cpp
// Apply schema evolution to existing table
table->evolve_schema(std::move(schema_v2));
```

### Evolution Rules

- **Backward Compatibility**: New schemas must be compatible with existing data
- **Additive Changes**: Can add new columns (must be nullable or have defaults)
- **Type Safety**: Cannot change existing column types
- **Primary Key Stability**: Cannot change primary key structure

## Schema Validation

### Automatic Validation

The system automatically validates:

```cpp
// This will throw TableException if validation fails
try {
    auto row_id = table->insert_row({
        {"id", static_cast<std::int64_t>(1)},
        {"name", std::string("Product A")},
        // Missing required column "price" - will throw exception
    });
} catch (const TableException& e) {
    // Handle validation error
    std::cout << "Validation error: " << e.what() << std::endl;
}
```

### Manual Validation

```cpp
// Validate row data against schema
std::unordered_map<std::string, CellValue> row_data = {
    {"id", static_cast<std::int64_t>(1)},
    {"name", std::string("Product A")},
    {"price", 29.99}
};

// The table will validate this automatically on insert/update
if (table->insert_row(row_data)) {
    std::cout << "Row validated and inserted successfully" << std::endl;
}
```

## Best Practices

### Schema Design

1. **Use Descriptive Names**: Choose clear, meaningful column names
2. **Plan for Growth**: Design schemas with future evolution in mind
3. **Minimize Nulls**: Use nullable columns sparingly
4. **Choose Appropriate Types**: Select the most suitable data type for each column
5. **Document Schemas**: Maintain clear documentation of schema purpose and structure

### Primary Key Selection

1. **Use Surrogate Keys**: Consider auto-incrementing integer IDs
2. **Keep Keys Simple**: Avoid complex composite keys when possible
3. **Ensure Stability**: Primary keys should not change frequently
4. **Consider Performance**: Simple keys query faster than complex ones

### Evolution Strategy

1. **Version Control**: Always increment schema versions
2. **Test Evolution**: Validate schema changes with existing data
3. **Additive Changes**: Prefer adding new columns over modifying existing ones
4. **Migration Planning**: Plan data migration strategies for complex changes

### Example: Complete Schema Setup

```cpp
#include "tables.h"
using namespace base;

// Create a comprehensive user schema
auto create_user_schema() -> std::unique_ptr<TableSchema> {
    auto schema = std::make_unique<TableSchema>("users", 1);

    // Primary key
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));

    // Required user information
    schema->add_column(ColumnDefinition("username", ColumnType::String, false));
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));
    schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false));

    // Optional user information
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("age", ColumnType::Integer, true));
    schema->add_column(ColumnDefinition("is_active", ColumnType::Boolean, true));
    schema->add_column(ColumnDefinition("profile_picture", ColumnType::Blob, true));

    // Set primary key
    schema->set_primary_key({"id"});

    return schema;
}

// Usage
auto table = std::make_unique<Table>(create_user_schema());
```

---

_See also: [Table API Reference](Tables-API.md), [Quick Start Guide](Tables-QuickStart.md)_
