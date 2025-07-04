/*
 * @file tables.h
 * @brief High-performance, thread-safe table data structure with schema evolution
 *
 * Provides a comprehensive table implementation with:
 * - Compile-time and runtime schema definition
 * - Thread-safe concurrent access with optimistic locking
 * - Indexing, searching, and querying capabilities
 * - JSON and binary serialization/deserialization
 * - Schema evolution and versioning
 * - Constraint enforcement and data validation
 * - Transaction support with rollback capabilities
 * - Change callbacks and event notifications
 * - In-memory and persistent storage options
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "logger.h"
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <any>
#include <variant>
#include <optional>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <regex>
#include <iostream>
#include <iomanip>

namespace base {

// Forward declarations
class Table;
class TableSchema;
class TableRow;
class TableIndex;
class TableTransaction;

/**
 * @brief Supported data types for table columns
 */
enum class ColumnType {
    Integer,
    Double,
    String,
    Boolean,
    DateTime,
    Binary,
    Json
};

/**
 * @brief Convert ColumnType to string representation
 */
inline std::string column_type_to_string(ColumnType type) {
    switch (type) {
        case ColumnType::Integer: return "integer";
        case ColumnType::Double: return "double";
        case ColumnType::String: return "string";
        case ColumnType::Boolean: return "boolean";
        case ColumnType::DateTime: return "datetime";
        case ColumnType::Binary: return "binary";
        case ColumnType::Json: return "json";
        default: return "unknown";
    }
}

/**
 * @brief Parse string to ColumnType
 */
inline std::optional<ColumnType> string_to_column_type(const std::string& str) {
    if (str == "integer") return ColumnType::Integer;
    if (str == "double") return ColumnType::Double;
    if (str == "string") return ColumnType::String;
    if (str == "boolean") return ColumnType::Boolean;
    if (str == "datetime") return ColumnType::DateTime;
    if (str == "binary") return ColumnType::Binary;
    if (str == "json") return ColumnType::Json;
    return std::nullopt;
}

/**
 * @brief Type-safe variant for table cell values
 */
using CellValue = std::variant<
    std::int64_t,
    double,
    std::string,
    bool,
    std::chrono::system_clock::time_point,
    std::vector<std::uint8_t>,
    std::monostate  // null value
>;

/**
 * @brief Constraint types for column validation
 */
enum class ConstraintType {
    NotNull,
    Unique,
    PrimaryKey,
    ForeignKey,
    Check,
    Default
};

/**
 * @brief Column constraint definition
 */
struct ColumnConstraint {
    ConstraintType type;
    std::string name;
    std::any value;  // For default values, check expressions, etc.
    std::string reference_table;  // For foreign keys
    std::string reference_column; // For foreign keys

    ColumnConstraint(ConstraintType t, std::string n = "")
        : type(t), name(std::move(n)) {}

    ColumnConstraint(ConstraintType t, std::string n, std::any v)
        : type(t), name(std::move(n)), value(std::move(v)) {}
};

/**
 * @brief Column definition with metadata
 */
struct ColumnDefinition {
    std::string name;
    ColumnType type;
    bool nullable = true;
    std::vector<ColumnConstraint> constraints;
    std::string description;
    std::any default_value;

    ColumnDefinition(std::string n, ColumnType t, bool null = true)
        : name(std::move(n)), type(t), nullable(null) {}
};

/**
 * @brief Table change event types
 */
enum class ChangeType {
    RowInserted,
    RowUpdated,
    RowDeleted,
    SchemaChanged,
    IndexCreated,
    IndexDropped
};

/**
 * @brief Change event data
 */
struct ChangeEvent {
    ChangeType type;
    std::string table_name;
    std::optional<std::size_t> row_id;
    std::unordered_map<std::string, CellValue> old_values;
    std::unordered_map<std::string, CellValue> new_values;
    std::chrono::system_clock::time_point timestamp;
    std::string transaction_id;

    ChangeEvent(ChangeType t, std::string table)
        : type(t), table_name(std::move(table))
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Change callback function type
 */
using ChangeCallback = std::function<void(const ChangeEvent&)>;

/**
 * @brief Query operator types
 */
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

/**
 * @brief Query condition for filtering rows
 */
struct QueryCondition {
    std::string column;
    QueryOperator op;
    CellValue value;
    std::optional<CellValue> value2;  // For BETWEEN operator

    QueryCondition(std::string col, QueryOperator o, CellValue val)
        : column(std::move(col)), op(o), value(std::move(val)) {}

    QueryCondition(std::string col, QueryOperator o, CellValue val1, CellValue val2)
        : column(std::move(col)), op(o), value(std::move(val1)), value2(std::move(val2)) {}
};

/**
 * @brief Query builder for table operations
 */
class TableQuery {
public:
    TableQuery& select(const std::vector<std::string>& columns);
    TableQuery& where(const QueryCondition& condition);
    TableQuery& where(const std::string& column, QueryOperator op, const CellValue& value);
    TableQuery& order_by(const std::string& column, bool ascending = true);
    TableQuery& limit(std::size_t count);
    TableQuery& offset(std::size_t count);

    const std::vector<std::string>& get_selected_columns() const { return selected_columns_; }
    const std::vector<QueryCondition>& get_conditions() const { return conditions_; }
    const std::vector<std::pair<std::string, bool>>& get_order_by() const { return order_by_; }
    std::size_t get_limit() const { return limit_; }
    std::size_t get_offset() const { return offset_; }

private:
    std::vector<std::string> selected_columns_;
    std::vector<QueryCondition> conditions_;
    std::vector<std::pair<std::string, bool>> order_by_;  // column, ascending
    std::size_t limit_ = std::numeric_limits<std::size_t>::max();
    std::size_t offset_ = 0;
};

/**
 * @brief Table row with versioning and metadata
 */
class TableRow {
public:
    TableRow(std::size_t id) : id_(id), version_(1), created_at_(std::chrono::system_clock::now()) {}

    // Cell value access
    void set_value(const std::string& column, const CellValue& value);
    CellValue get_value(const std::string& column) const;
    bool has_column(const std::string& column) const;

    // Metadata
    std::size_t get_id() const { return id_; }
    void set_id(std::size_t id) { id_ = id; }  // Allow ID modification for merge operations
    std::uint32_t get_version() const { return version_; }
    std::chrono::system_clock::time_point get_created_at() const { return created_at_; }
    std::chrono::system_clock::time_point get_updated_at() const { return updated_at_; }

    // Internal version management
    void increment_version();

    // Get all values
    const std::unordered_map<std::string, CellValue>& get_all_values() const { return values_; }

    // Serialization
    std::string to_json() const;
    bool from_json(const std::string& json);

private:
    std::size_t id_;
    std::uint32_t version_;
    std::chrono::system_clock::time_point created_at_;
    std::chrono::system_clock::time_point updated_at_;
    std::unordered_map<std::string, CellValue> values_;

    friend class Table;
};

/**
 * @brief Table schema with versioning support
 */
class TableSchema {
public:
    TableSchema(std::string name, std::uint32_t version = 1);

    // Column management
    void add_column(const ColumnDefinition& column);
    void remove_column(const std::string& name);
    void modify_column(const std::string& name, const ColumnDefinition& new_def);

    // Schema information
    std::string get_name() const { return name_; }
    std::uint32_t get_version() const { return version_; }
    const std::vector<ColumnDefinition>& get_columns() const { return columns_; }
    std::optional<ColumnDefinition> get_column(const std::string& name) const;

    // Validation
    bool validate_row(const TableRow& row) const;
    std::vector<std::string> get_validation_errors(const TableRow& row) const;

    // Primary key management
    void set_primary_key(const std::vector<std::string>& columns);
    const std::vector<std::string>& get_primary_key() const { return primary_key_; }

    // Serialization
    std::string to_json() const;
    bool from_json(const std::string& json);

    // Schema evolution
    std::unique_ptr<TableSchema> evolve(std::uint32_t new_version) const;

private:
    std::string name_;
    std::uint32_t version_;
    std::vector<ColumnDefinition> columns_;
    std::vector<std::string> primary_key_;

    void increment_version() { version_++; }
    friend class Table;
};

/**
 * @brief Table index for fast lookups
 */
class TableIndex {
public:
    TableIndex(std::string name, std::vector<std::string> columns, bool unique = false);

    // Copy constructor and assignment operator
    TableIndex(const TableIndex& other);
    TableIndex& operator=(const TableIndex& other);

    // Index operations
    void insert(const TableRow& row);
    void remove(const TableRow& row);
    void update(const TableRow& old_row, const TableRow& new_row);

    // Lookup operations
    std::vector<std::size_t> find_exact(const std::vector<CellValue>& key) const;
    std::vector<std::size_t> find_range(const std::vector<CellValue>& start_key,
                                       const std::vector<CellValue>& end_key) const;

    // Index information
    std::string get_name() const { return name_; }
    const std::vector<std::string>& get_columns() const { return columns_; }
    bool is_unique() const { return unique_; }

    // Statistics
    std::size_t size() const;
    void clear();

private:
    std::string name_;
    std::vector<std::string> columns_;
    bool unique_;

    // Multi-column index using composite keys
    std::map<std::vector<CellValue>, std::set<std::size_t>> index_;
    mutable std::shared_mutex mutex_;

    std::vector<CellValue> extract_key(const TableRow& row) const;
};

/**
 * @brief Transaction context for atomic operations
 */
class TableTransaction {
public:
    TableTransaction(Table* table, std::string id);
    ~TableTransaction();

    // Transaction operations
    void begin();
    void commit();
    void rollback();

    // State management
    bool is_active() const { return active_; }
    bool is_committed() const { return committed_; }
    bool is_rolled_back() const { return rolled_back_; }

    std::string get_id() const { return id_; }

private:
    Table* table_;
    std::string id_;
    bool active_ = false;
    bool committed_ = false;
    bool rolled_back_ = false;

    // Transaction log for rollback
    std::vector<ChangeEvent> change_log_;

    friend class Table;
};

/**
 * @brief Forward iterator for table rows
 */
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

private:
    std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator current_;
    std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator end_;
};

/**
 * @brief Const iterator for table rows
 */
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

private:
    std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator current_;
    std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator end_;
};

/**
 * @brief Table output format options
 */
enum class TableOutputFormat {
    ASCII,      // ASCII table with borders
    CSV,        // Comma-separated values
    TSV,        // Tab-separated values
    JSON,       // JSON array format
    Markdown    // Markdown table format
};

/**
 * @brief Table dump configuration
 */
struct TableDumpOptions {
    TableOutputFormat format = TableOutputFormat::ASCII;
    std::size_t page_size = 50;                    // Rows per page (0 = no paging)
    std::size_t max_column_width = 40;             // Maximum column width
    bool show_row_numbers = false;                 // Show row IDs
    bool show_headers = true;                      // Show column headers
    bool truncate_long_values = true;              // Truncate values exceeding max_column_width
    std::string null_representation = "NULL";      // How to display null values
    std::vector<std::string> columns_to_show;      // Specific columns to show (empty = all)
    TableQuery filter_query;                      // Optional query to filter rows
    bool color_output = false;                     // Enable ANSI color codes (for terminals)

    TableDumpOptions() = default;
};

/**
 * @brief Interactive paging context for table output
 */
class TablePager {
public:
    explicit TablePager(const Table& table, const TableDumpOptions& options = {});

    // Paging operations
    void show_page(std::size_t page_number = 0) const;
    void show_next_page();
    void show_previous_page();
    void show_first_page();
    void show_last_page();

    // Navigation info
    std::size_t get_current_page() const { return current_page_; }
    std::size_t get_total_pages() const;
    std::size_t get_total_rows() const;

    // Interactive mode
    void start_interactive_mode();  // Interactive CLI paging

    // Direct output
    std::string get_page_as_string(std::size_t page_number = 0) const;

private:
    const Table& table_;
    TableDumpOptions options_;
    std::size_t current_page_ = 0;
    mutable std::vector<TableRow> filtered_rows_;  // Cached filtered rows
    mutable bool rows_cached_ = false;

    void cache_filtered_rows() const;
    void print_navigation_info() const;
    std::string format_table_page(const std::vector<TableRow>& page_rows) const;
    std::string format_ascii_table(const std::vector<TableRow>& rows) const;
    std::string format_csv_table(const std::vector<TableRow>& rows) const;
    std::string format_json_table(const std::vector<TableRow>& rows) const;
    std::string format_markdown_table(const std::vector<TableRow>& rows) const;
    std::vector<std::string> get_display_columns() const;
    std::string truncate_value(const std::string& value, std::size_t max_width) const;
};

/**
 * @brief High-performance table with comprehensive features
 */
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
    const TableSchema& get_schema() const { return *schema_; }
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
    void enable_concurrent_access(bool enable = true) { concurrent_access_enabled_ = enable; }
    bool is_concurrent_access_enabled() const { return concurrent_access_enabled_; }

    // Table dump and printing
    void dump(const TableDumpOptions& options = {}) const;         // Print to stdout
    void dump_to_stream(std::ostream& stream, const TableDumpOptions& options = {}) const;
    std::string dump_to_string(const TableDumpOptions& options = {}) const;
    std::unique_ptr<TablePager> create_pager(const TableDumpOptions& options = {}) const;

    // CLI-friendly methods
    void print_summary() const;                                    // Print table info summary
    void print_schema() const;                                     // Print schema information
    void print_statistics() const;                                // Print detailed statistics

private:
    std::unique_ptr<TableSchema> schema_;
    std::unordered_map<std::size_t, std::unique_ptr<TableRow>> rows_;
    std::unordered_map<std::string, std::unique_ptr<TableIndex>> indexes_;
    std::unordered_map<std::string, ChangeCallback> change_callbacks_;

    // Thread safety
    mutable std::shared_mutex table_mutex_;
    bool concurrent_access_enabled_ = true;

    // Statistics
    std::atomic<std::size_t> next_row_id_{1};
    std::atomic<std::size_t> total_inserts_{0};
    std::atomic<std::size_t> total_updates_{0};
    std::atomic<std::size_t> total_deletes_{0};
    std::chrono::system_clock::time_point created_at_;
    mutable std::atomic<std::chrono::system_clock::time_point> last_modified_;

    // Helper methods
    void fire_change_event(const ChangeEvent& event);
    bool validate_row_constraints(const TableRow& row) const;
    void update_indexes_on_insert(const TableRow& row);
    void update_indexes_on_update(const TableRow& old_row, const TableRow& new_row);
    void update_indexes_on_delete(const TableRow& row);

    // Query evaluation
    bool evaluate_condition(const TableRow& row, const QueryCondition& condition) const;
    std::vector<TableRow> apply_query_filters(const std::vector<TableRow>& rows,
                                             const TableQuery& query) const;

    friend class TableTransaction;
};

/**
 * @brief Table factory for creating tables with various configurations
 */
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

/**
 * @brief Utility functions for working with cell values
 */
namespace cell_utils {
    std::string to_string(const CellValue& value);
    CellValue from_string(const std::string& str, ColumnType type);
    bool compare_values(const CellValue& left, const CellValue& right, QueryOperator op);
    ColumnType get_value_type(const CellValue& value);
    bool is_null(const CellValue& value);
    CellValue make_null();
}

} // namespace base