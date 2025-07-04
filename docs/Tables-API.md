# Table API Reference

## Table of Contents

1. [Core Classes](#core-classes)
2. [Data Types](#data-types)
3. [Table Operations](#table-operations)
4. [Schema Management](#schema-management)
5. [Query System](#query-system)
6. [Index Management](#index-management)
7. [Transactions](#transactions)
8. [Serialization](#serialization)
9. [Utilities](#utilities)

## Core Classes

### Table

Main table class providing data storage and manipulation.

```cpp
class Table {
public:
    // Iterator type aliases
    using iterator = TableIterator;
    using const_iterator = TableConstIterator;

    explicit Table(std::unique_ptr<TableSchema> schema);
    ~Table() = default;

    // Copy operations
    Table(const Table& other);
    Table& operator=(const Table& other);

    // Move operations
    Table(Table&& other) noexcept;
    Table& operator=(Table&& other) noexcept;

    // Row operations
    std::size_t insert_row(const std::unordered_map<std::string, CellValue>& values);
    bool update_row(std::size_t row_id, const std::unordered_map<std::string, CellValue>& values);
    bool delete_row(std::size_t row_id);

    // Row access
    std::optional<TableRow> get_row(std::size_t row_id) const;
    std::vector<TableRow> get_all_rows() const;
    std::size_t get_row_count() const;

    // Iterator support
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    // Range-based for loop support (alternative names)
    iterator rows_begin() { return begin(); }
    iterator rows_end() { return end(); }
    const_iterator rows_begin() const { return begin(); }
    const_iterator rows_end() const { return end(); }

    // Table operations
    void clear();                                           // Remove all rows
    bool empty() const;                                     // Check if table is empty
    std::unique_ptr<Table> clone() const;                   // Deep copy table
    void merge_from(const Table& other);                    // Merge rows from another table
    void swap(Table& other) noexcept;                       // Swap table contents

    // Query operations
    std::vector<TableRow> query(const TableQuery& query) const;
    std::vector<TableRow> find_by_index(const std::string& index_name,
                                       const std::vector<CellValue>& key) const;

    // Schema management
    const TableSchema& get_schema() const;
    void evolve_schema(std::unique_ptr<TableSchema> new_schema);

    // Index management
    void create_index(const std::string& name, const std::vector<std::string>& columns,
                     bool unique = false);
    void drop_index(const std::string& name);
    std::vector<std::string> get_index_names() const;

    // Change callbacks
    void add_change_callback(const std::string& name, ChangeCallback callback);
    void remove_change_callback(const std::string& name);

    // Transaction support
    std::unique_ptr<TableTransaction> begin_transaction();

    // Persistence
    bool save_to_file(const std::string& filename) const;
    bool load_from_file(const std::string& filename);

    // Serialization
    std::string to_json() const;
    bool from_json(const std::string& json);

    // Statistics
    struct Statistics {
        std::size_t row_count;
        std::size_t index_count;
        std::size_t schema_version;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_modified;
        std::size_t total_inserts;
        std::size_t total_updates;
        std::size_t total_deletes;
    };

    Statistics get_statistics() const;

    // Thread safety
    void enable_concurrent_access(bool enable = true);
    bool is_concurrent_access_enabled() const;
};
```

### TableIterator

Forward iterator for traversing table rows.

```cpp
class TableIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = TableRow;
    using difference_type = std::ptrdiff_t;
    using pointer = const TableRow*;
    using reference = const TableRow&;

    TableIterator() = default;
    TableIterator(const std::unordered_map<std::size_t, std::unique_ptr<TableRow>>& rows,
                  std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator it);

    reference operator*() const;
    pointer operator->() const;
    TableIterator& operator++();
    TableIterator operator++(int);
    bool operator==(const TableIterator& other) const;
    bool operator!=(const TableIterator& other) const;
};
```

### TableConstIterator

Const forward iterator for read-only traversal of table rows.

```cpp
class TableConstIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const TableRow;
    using difference_type = std::ptrdiff_t;
    using pointer = const TableRow*;
    using reference = const TableRow&;

    TableConstIterator() = default;
    TableConstIterator(const std::unordered_map<std::size_t, std::unique_ptr<TableRow>>& rows,
                       std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator it);

    reference operator*() const;
    pointer operator->() const;
    TableConstIterator& operator++();
    TableConstIterator operator++(int);
    bool operator==(const TableConstIterator& other) const;
    bool operator!=(const TableConstIterator& other) const;
};
```

### TableSchema

Schema definition and management.

```cpp
class TableSchema {
public:
    TableSchema(std::string name, std::uint32_t version = 1);

    // Column management
    void add_column(const ColumnDefinition& column);
    void remove_column(const std::string& name);
    void modify_column(const std::string& name, const ColumnDefinition& new_def);

    // Schema information
    std::string get_name() const;
    std::uint32_t get_version() const;
    const std::vector<ColumnDefinition>& get_columns() const;
    std::optional<ColumnDefinition> get_column(const std::string& name) const;

    // Validation
    bool validate_row(const TableRow& row) const;
    std::vector<std::string> get_validation_errors(const TableRow& row) const;

    // Primary key management
    void set_primary_key(const std::vector<std::string>& columns);
    const std::vector<std::string>& get_primary_key() const;

    // Serialization
    std::string to_json() const;
    bool from_json(const std::string& json);

    // Schema evolution
    std::unique_ptr<TableSchema> evolve(std::uint32_t new_version) const;
};
```

### TableRow

Individual row with metadata and versioning.

```cpp
class TableRow {
public:
    TableRow(std::size_t id);

    // Cell value access
    void set_value(const std::string& column, const CellValue& value);
    CellValue get_value(const std::string& column) const;
    bool has_column(const std::string& column) const;

    // Metadata
    std::size_t get_id() const;
    std::uint32_t get_version() const;
    std::chrono::system_clock::time_point get_created_at() const;
    std::chrono::system_clock::time_point get_updated_at() const;

    // Internal version management
    void increment_version();

    // Get all values
    const std::unordered_map<std::string, CellValue>& get_all_values() const;

    // Serialization
    std::string to_json() const;
    bool from_json(const std::string& json);
};
```

## Data Types

### ColumnType

Supported column data types.

```cpp
enum class ColumnType {
    Integer,    // std::int64_t
    Double,     // double
    String,     // std::string
    Boolean,    // bool
    DateTime,   // std::chrono::system_clock::time_point
    Binary,     // std::vector<std::uint8_t>
    Json        // JSON data (stored as string)
};
```

### CellValue

Type-safe variant for table cell values.

```cpp
using CellValue = std::variant<
    std::int64_t,
    double,
    std::string,
    bool,
    std::chrono::system_clock::time_point,
    std::vector<std::uint8_t>,
    std::monostate  // null value
>;
```

### ColumnDefinition

Column metadata and constraints.

```cpp
struct ColumnDefinition {
    std::string name;
    ColumnType type;
    bool nullable = true;
    std::vector<ColumnConstraint> constraints;
    std::string description;
    std::any default_value;

    ColumnDefinition(std::string n, ColumnType t, bool null = true);
};
```

### ColumnConstraint

Column constraint definition.

```cpp
enum class ConstraintType {
    NotNull,
    Unique,
    PrimaryKey,
    ForeignKey,
    Check,
    Default
};

struct ColumnConstraint {
    ConstraintType type;
    std::string name;
    std::any value;  // For default values, check expressions, etc.
    std::string reference_table;  // For foreign keys
    std::string reference_column; // For foreign keys

    ColumnConstraint(ConstraintType t, std::string n = "");
    ColumnConstraint(ConstraintType t, std::string n, std::any v);
};
```

## Table Operations

### Row Insertion

```cpp
// Insert single row
std::unordered_map<std::string, CellValue> row_data = {
    {"id", static_cast<std::int64_t>(1)},
    {"name", std::string("John Doe")},
    {"age", static_cast<std::int64_t>(30)}
};
std::size_t row_id = table->insert_row(row_data);
```

### Row Update

```cpp
// Update specific columns
std::unordered_map<std::string, CellValue> updates = {
    {"name", std::string("John Smith")},
    {"age", static_cast<std::int64_t>(31)}
};
bool success = table->update_row(row_id, updates);
```

### Row Deletion

```cpp
// Delete by row ID
bool success = table->delete_row(row_id);
```

### Row Retrieval

```cpp
// Get specific row
std::optional<TableRow> row = table->get_row(row_id);
if (row.has_value()) {
    auto name = std::get<std::string>(row->get_value("name"));
}

// Get all rows
std::vector<TableRow> all_rows = table->get_all_rows();

// Get row count
std::size_t count = table->get_row_count();
```

## Query System

### TableQuery

Query builder for complex data retrieval.

```cpp
class TableQuery {
public:
    TableQuery& select(const std::vector<std::string>& columns);
    TableQuery& where(const QueryCondition& condition);
    TableQuery& where(const std::string& column, QueryOperator op, const CellValue& value);
    TableQuery& order_by(const std::string& column, bool ascending = true);
    TableQuery& limit(std::size_t count);
    TableQuery& offset(std::size_t count);

    // Getters
    const std::vector<std::string>& get_selected_columns() const;
    const std::vector<QueryCondition>& get_conditions() const;
    const std::vector<std::pair<std::string, bool>>& get_order_by() const;
    std::size_t get_limit() const;
    std::size_t get_offset() const;
};
```

### QueryOperator

Supported query operators.

```cpp
enum class QueryOperator {
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Like,
    In,
    Between,
    IsNull,
    IsNotNull
};
```

### QueryCondition

Individual query condition.

```cpp
struct QueryCondition {
    std::string column;
    QueryOperator op;
    CellValue value;
    std::optional<CellValue> value2;  // For BETWEEN operator

    QueryCondition(std::string col, QueryOperator o, CellValue val);
    QueryCondition(std::string col, QueryOperator o, CellValue val1, CellValue val2);
};
```

### Query Examples

```cpp
// Simple equality query
TableQuery query1;
query1.where("name", QueryOperator::Equal, std::string("John Doe"));

// Range query with ordering
TableQuery query2;
query2.where("age", QueryOperator::GreaterThan, static_cast<std::int64_t>(25))
      .where("age", QueryOperator::LessThan, static_cast<std::int64_t>(65))
      .order_by("age", true)
      .limit(10);

// Column selection
TableQuery query3;
query3.select({"name", "email", "age"})
      .where("active", QueryOperator::Equal, true);

// Execute queries
auto results1 = table->query(query1);
auto results2 = table->query(query2);
auto results3 = table->query(query3);
```

## Index Management

### TableIndex

Index for fast data retrieval.

```cpp
class TableIndex {
public:
    TableIndex(std::string name, std::vector<std::string> columns, bool unique = false);

    // Index operations
    void insert(const TableRow& row);
    void remove(const TableRow& row);
    void update(const TableRow& old_row, const TableRow& new_row);

    // Lookup operations
    std::vector<std::size_t> find_exact(const std::vector<CellValue>& key) const;
    std::vector<std::size_t> find_range(const std::vector<CellValue>& start_key,
                                       const std::vector<CellValue>& end_key) const;

    // Index information
    std::string get_name() const;
    const std::vector<std::string>& get_columns() const;
    bool is_unique() const;

    // Statistics
    std::size_t size() const;
    void clear();
};
```

### Index Operations

```cpp
// Create single column index
table->create_index("name_idx", {"name"});

// Create multi-column index
table->create_index("name_email_idx", {"name", "email"});

// Create unique index
table->create_index("email_unique", {"email"}, true);

// Query using index
auto results = table->find_by_index("name_idx", {std::string("John Doe")});

// List all indexes
auto index_names = table->get_index_names();

// Drop index
table->drop_index("name_idx");
```

## Transactions

### TableTransaction

Transaction context for atomic operations.

```cpp
class TableTransaction {
public:
    TableTransaction(Table* table, std::string id);
    ~TableTransaction();

    // Transaction operations
    void begin();
    void commit();
    void rollback();

    // State management
    bool is_active() const;
    bool is_committed() const;
    bool is_rolled_back() const;

    std::string get_id() const;
};
```

### Transaction Usage

```cpp
// Begin transaction
auto transaction = table->begin_transaction();
transaction->begin();

try {
    // Perform multiple operations
    table->insert_row(row1);
    table->update_row(id, updates);
    table->delete_row(old_id);

    // Commit if all succeed
    transaction->commit();

} catch (const std::exception& e) {
    // Rollback on error
    transaction->rollback();
    throw;
}
```

## Serialization

### JSON Format

Tables can be serialized to/from JSON format:

```json
{
  "schema": {
    "name": "users",
    "version": 1,
    "columns": [
      {
        "name": "id",
        "type": "integer",
        "nullable": false,
        "constraints": [{ "type": "primary_key" }]
      },
      {
        "name": "name",
        "type": "string",
        "nullable": false
      }
    ],
    "primary_key": ["id"]
  },
  "rows": [
    {
      "id": 1,
      "values": {
        "id": 1,
        "name": "John Doe"
      },
      "version": 1,
      "created_at": "2025-07-03T19:00:00Z",
      "updated_at": "2025-07-03T19:00:00Z"
    }
  ],
  "indexes": [
    {
      "name": "name_idx",
      "columns": ["name"],
      "unique": false
    }
  ],
  "statistics": {
    "row_count": 1,
    "total_inserts": 1,
    "total_updates": 0,
    "total_deletes": 0
  }
}
```

### Serialization Methods

```cpp
// Serialize to JSON string
std::string json_data = table->to_json();

// Deserialize from JSON string
bool success = table->from_json(json_data);

// Save to file
bool saved = table->save_to_file("table.json");

// Load from file
bool loaded = table->load_from_file("table.json");
```

## Change Events

### ChangeEvent

Event data for table modifications.

```cpp
enum class ChangeType {
    RowInserted,
    RowUpdated,
    RowDeleted,
    SchemaChanged,
    IndexCreated,
    IndexDropped
};

struct ChangeEvent {
    ChangeType type;
    std::string table_name;
    std::optional<std::size_t> row_id;
    std::unordered_map<std::string, CellValue> old_values;
    std::unordered_map<std::string, CellValue> new_values;
    std::chrono::system_clock::time_point timestamp;
    std::string transaction_id;
};
```

### Callback Registration

```cpp
// Register callback
table->add_change_callback("audit_log", [](const ChangeEvent& event) {
    switch (event.type) {
        case ChangeType::RowInserted:
            base::Logger::info("Row inserted: {}", event.row_id.value_or(0));
            break;
        case ChangeType::RowUpdated:
            base::Logger::info("Row updated: {}", event.row_id.value_or(0));
            break;
        case ChangeType::RowDeleted:
            base::Logger::info("Row deleted: {}", event.row_id.value_or(0));
            break;
        default:
            break;
    }
});

// Remove callback
table->remove_change_callback("audit_log");
```

## Utilities

### TableFactory

Factory for creating tables with various configurations.

```cpp
class TableFactory {
public:
    // Create table with compile-time schema
    template<typename... ColumnTypes>
    static std::unique_ptr<Table> create_table(const std::string& name,
                                             const std::vector<std::string>& column_names);

    // Create table with runtime schema
    static std::unique_ptr<Table> create_table(const std::string& name,
                                             const std::vector<ColumnDefinition>& columns);

    // Load table from file
    static std::unique_ptr<Table> load_table(const std::string& filename);

    // Create table from JSON schema
    static std::unique_ptr<Table> create_table_from_json(const std::string& json_schema);
};
```

### Cell Utilities

Utility functions for working with cell values.

```cpp
namespace cell_utils {
    // Convert cell value to string
    std::string to_string(const CellValue& value);

    // Create cell value from string
    CellValue from_string(const std::string& str, ColumnType type);

    // Compare two cell values
    bool compare_values(const CellValue& left, const CellValue& right, QueryOperator op);

    // Get the type of a cell value
    ColumnType get_value_type(const CellValue& value);

    // Check if cell value is null
    bool is_null(const CellValue& value);

    // Create null cell value
    CellValue make_null();
}
```

### Error Handling

The Table system uses standard C++ exceptions:

```cpp
try {
    table->insert_row(data);
} catch (const std::invalid_argument& e) {
    // Invalid column names, types, or constraints
} catch (const std::runtime_error& e) {
    // Runtime errors (file I/O, JSON parsing, etc.)
} catch (const std::logic_error& e) {
    // Logic errors (schema mismatches, etc.)
}
```

## Thread Safety

The Table system is designed for concurrent access:

- **Read Operations**: Multiple threads can read simultaneously
- **Write Operations**: Synchronized using shared_mutex
- **Atomic Counters**: Statistics are thread-safe
- **Index Updates**: Automatically synchronized

```cpp
// Enable/disable concurrent access
table->enable_concurrent_access(true);
bool is_safe = table->is_concurrent_access_enabled();
```

---

## Iterator and Container API Details

### Iterator Support

The Table class provides STL-compatible iterators for traversing rows:

```cpp
// Range-based for loop (preferred)
for (const auto& row : *table) {
    std::cout << "Row ID: " << row.get_id() << std::endl;
}

// Explicit iterator usage
for (auto it = table->begin(); it != table->end(); ++it) {
    const TableRow& row = *it;
    // Process row...
}

// Const iterators
for (auto it = table->cbegin(); it != table->cend(); ++it) {
    // Read-only access
}

// STL algorithm compatibility
auto count = std::count_if(table->begin(), table->end(),
    [](const TableRow& row) {
        // Custom predicate
        return /* condition */;
    });
```

### Copy Operations

#### Copy Constructor

```cpp
Table(const Table& other);
```

Creates a deep copy of the table, including all data, indexes, and configuration.

#### Copy Assignment

```cpp
Table& operator=(const Table& other);
```

Assigns the contents of another table, performing a deep copy.

### Move Operations

#### Move Constructor

```cpp
Table(Table&& other) noexcept;
```

Efficiently transfers ownership without copying data. `noexcept` guarantee.

#### Move Assignment

```cpp
Table& operator=(Table&& other) noexcept;
```

Moves contents from another table. `noexcept` guarantee.

### Table Utility Methods

#### clear()

```cpp
void clear();
```

Removes all rows from the table while preserving schema and indexes.

- Fires deletion events for all rows
- Recreates primary key index if defined
- Thread-safe operation

#### empty()

```cpp
bool empty() const;
```

Returns `true` if the table contains no rows. O(1) operation.

#### clone()

```cpp
std::unique_ptr<Table> clone() const;
```

Creates a complete deep copy of the table, returning a new Table instance.

- Copies all data, indexes, and configuration
- Returns independent table
- Thread-safe operation

#### merge_from()

```cpp
void merge_from(const Table& other);
```

Merges rows from another compatible table.

- Validates schema compatibility
- Handles primary key conflicts by adjusting row IDs
- Fires change events for merged rows
- Throws exception for incompatible schemas

#### swap()

```cpp
void swap(Table& other) noexcept;
```

Efficiently swaps the contents of two tables.

- O(1) operation
- `noexcept` guarantee
- Swaps all data, indexes, and configuration
- Thread-safe operation

### Usage Examples

```cpp
// Create tables
auto table1 = createTable();
auto table2 = createTable();

// Copy operations
Table copied_table(*table1);              // Copy constructor
*table2 = *table1;                        // Copy assignment

// Move operations
Table moved_table(std::move(*table1));    // Move constructor
*table2 = std::move(moved_table);         // Move assignment

// Utility operations
table1->clear();                          // Remove all rows
bool is_empty = table1->empty();          // Check if empty
auto cloned = table2->clone();            // Create deep copy
table1->merge_from(*table2);              // Merge tables
table1->swap(*table2);                    // Swap contents

// Iterator usage
for (const auto& row : *table1) {
    auto id = row.get_id();
    auto values = row.get_all_values();
    // Process row...
}
```

---

_For more examples and advanced usage, see [Examples Documentation](Tables-Examples.md) and [Iterator Guide](Tables-Iterators.md)._
