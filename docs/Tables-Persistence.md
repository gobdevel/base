# Table Serialization and Persistence

## Overview

The Table system provides comprehensive serialization and persistence capabilities, supporting both JSON and binary formats for data storage, backup, and transfer. This guide covers serialization formats, file operations, and data recovery strategies.

## Table of Contents

1. [Serialization Formats](#serialization-formats)
2. [JSON Serialization](#json-serialization)
3. [Binary Serialization](#binary-serialization)
4. [File Operations](#file-operations)
5. [Data Recovery](#data-recovery)
6. [Migration and Import/Export](#migration-and-importexport)
7. [Performance Considerations](#performance-considerations)
8. [Best Practices](#best-practices)
9. [Examples](#examples)

## Serialization Formats

The Table system supports multiple serialization formats:

1. **JSON Format**: Human-readable, cross-platform compatible
2. **Binary Format**: Compact, high-performance storage
3. **Schema-Aware**: Preserves type information and constraints

### Format Comparison

| Feature             | JSON   | Binary  |
| ------------------- | ------ | ------- |
| Human Readable      | ✅ Yes | ❌ No   |
| File Size           | Larger | Smaller |
| Performance         | Slower | Faster  |
| Cross-Platform      | ✅ Yes | ✅ Yes  |
| Schema Preservation | ✅ Yes | ✅ Yes  |
| Partial Loading     | ❌ No  | ✅ Yes  |

## JSON Serialization

### Basic JSON Operations

```cpp
#include "tables.h"
using namespace base;

// Serialize table to JSON
auto json_data = table->to_json();

// Save JSON to string
std::string json_string = json_data.dump(2);  // Pretty print with 2-space indent

// Create table from JSON
auto restored_table = Table::from_json(json_data);
```

### JSON Structure

The JSON format preserves complete table information:

```json
{
  "schema": {
    "name": "users",
    "version": 1,
    "columns": [
      {
        "name": "id",
        "type": "Integer",
        "nullable": false
      },
      {
        "name": "name",
        "type": "String",
        "nullable": false
      },
      {
        "name": "email",
        "type": "String",
        "nullable": true
      }
    ],
    "primary_key": ["id"]
  },
  "indexes": [
    {
      "name": "email_idx",
      "columns": ["email"]
    }
  ],
  "rows": [
    {
      "id": 1,
      "name": "John Doe",
      "email": "john@example.com"
    },
    {
      "id": 2,
      "name": "Jane Smith",
      "email": null
    }
  ]
}
```

### Custom JSON Serialization

```cpp
// Serialize with custom options
nlohmann::json custom_json;
custom_json["timestamp"] = current_timestamp();
custom_json["application"] = "MyApp";
custom_json["table_data"] = table->to_json();

// Add metadata
custom_json["metadata"] = {
    {"row_count", table->get_row_count()},
    {"created_by", "user123"},
    {"export_reason", "backup"}
};
```

## Binary Serialization

### Basic Binary Operations

```cpp
// Serialize to binary
std::vector<std::uint8_t> binary_data = table->to_binary();

// Save binary data to file
std::ofstream file("table_backup.dat", std::ios::binary);
file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
file.close();

// Load binary data from file
std::ifstream input_file("table_backup.dat", std::ios::binary);
std::vector<std::uint8_t> loaded_data(
    (std::istreambuf_iterator<char>(input_file)),
    std::istreambuf_iterator<char>()
);

// Create table from binary
auto restored_table = Table::from_binary(loaded_data);
```

### Binary Format Advantages

```cpp
// Compare serialization performance
auto start = std::chrono::high_resolution_clock::now();

// JSON serialization
auto json_data = table->to_json();
auto json_size = json_data.dump().size();

auto json_time = std::chrono::high_resolution_clock::now();

// Binary serialization
auto binary_data = table->to_binary();
auto binary_size = binary_data.size();

auto end = std::chrono::high_resolution_clock::now();

auto json_duration = std::chrono::duration_cast<std::chrono::milliseconds>(json_time - start);
auto binary_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - json_time);

std::cout << "JSON: " << json_size << " bytes, " << json_duration.count() << "ms" << std::endl;
std::cout << "Binary: " << binary_size << " bytes, " << binary_duration.count() << "ms" << std::endl;
```

## File Operations

### Saving to Files

```cpp
// Save to JSON file
table->save_to_file("backup.json");

// Save to binary file
table->save_to_file("backup.dat", SerializationFormat::Binary);

// Save with compression (if supported)
table->save_to_file("backup.gz", SerializationFormat::Binary, true);
```

### Loading from Files

```cpp
// Load from JSON file
auto table_from_json = Table::load_from_file("backup.json");

// Load from binary file
auto table_from_binary = Table::load_from_file("backup.dat");

// Auto-detect format
auto table_auto = Table::load_from_file("backup.unknown");
```

### File Format Detection

```cpp
SerializationFormat detect_format(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file");
    }

    // Read first few bytes
    char buffer[4];
    file.read(buffer, 4);

    // JSON files typically start with '{' or '['
    if (buffer[0] == '{' || buffer[0] == '[') {
        return SerializationFormat::JSON;
    }

    // Binary files have custom header
    return SerializationFormat::Binary;
}
```

### Atomic File Operations

```cpp
void safe_save_to_file(const Table& table, const std::string& filename) {
    std::string temp_filename = filename + ".tmp";

    try {
        // Save to temporary file first
        table.save_to_file(temp_filename);

        // Atomic rename (on most filesystems)
        std::filesystem::rename(temp_filename, filename);

    } catch (const std::exception& e) {
        // Clean up temporary file on error
        std::filesystem::remove(temp_filename);
        throw;
    }
}
```

## Data Recovery

### Backup Strategies

```cpp
class TableBackupManager {
private:
    std::string backup_dir_;

public:
    explicit TableBackupManager(const std::string& backup_dir)
        : backup_dir_(backup_dir) {
        std::filesystem::create_directories(backup_dir_);
    }

    void create_backup(const Table& table, const std::string& name) {
        auto timestamp = current_timestamp();
        auto backup_name = name + "_" + timestamp + ".json";
        auto backup_path = std::filesystem::path(backup_dir_) / backup_name;

        table.save_to_file(backup_path.string());

        // Keep only last 10 backups
        cleanup_old_backups(name, 10);
    }

    std::unique_ptr<Table> restore_backup(const std::string& backup_file) {
        auto backup_path = std::filesystem::path(backup_dir_) / backup_file;
        return Table::load_from_file(backup_path.string());
    }

    std::vector<std::string> list_backups(const std::string& name) {
        std::vector<std::string> backups;

        for (const auto& entry : std::filesystem::directory_iterator(backup_dir_)) {
            if (entry.path().filename().string().starts_with(name + "_")) {
                backups.push_back(entry.path().filename().string());
            }
        }

        std::sort(backups.rbegin(), backups.rend());  // Most recent first
        return backups;
    }

private:
    void cleanup_old_backups(const std::string& name, size_t keep_count) {
        auto backups = list_backups(name);

        for (size_t i = keep_count; i < backups.size(); ++i) {
            auto old_backup = std::filesystem::path(backup_dir_) / backups[i];
            std::filesystem::remove(old_backup);
        }
    }
};
```

### Incremental Backups

```cpp
class IncrementalBackup {
private:
    std::chrono::system_clock::time_point last_backup_;
    std::unordered_set<std::size_t> changed_rows_;

public:
    void track_change(std::size_t row_id) {
        changed_rows_.insert(row_id);
    }

    void create_incremental_backup(const Table& table, const std::string& filename) {
        nlohmann::json backup_data;
        backup_data["type"] = "incremental";
        backup_data["timestamp"] = current_timestamp();
        backup_data["base_timestamp"] = format_timestamp(last_backup_);

        // Only serialize changed rows
        nlohmann::json changed_data = nlohmann::json::array();
        for (auto row_id : changed_rows_) {
            auto row = table.get_row(row_id);
            if (row) {
                changed_data.push_back(row->to_json());
            }
        }

        backup_data["changed_rows"] = changed_data;

        // Save incremental backup
        std::ofstream file(filename);
        file << backup_data.dump(2);

        // Reset tracking
        changed_rows_.clear();
        last_backup_ = std::chrono::system_clock::now();
    }
};
```

### Data Validation

```cpp
bool validate_serialized_data(const nlohmann::json& data) {
    try {
        // Check required fields
        if (!data.contains("schema") || !data.contains("rows")) {
            return false;
        }

        // Validate schema structure
        const auto& schema = data["schema"];
        if (!schema.contains("name") || !schema.contains("version") ||
            !schema.contains("columns")) {
            return false;
        }

        // Validate data types in rows
        const auto& rows = data["rows"];
        if (!rows.is_array()) {
            return false;
        }

        // Additional validation logic...
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}
```

## Migration and Import/Export

### Schema Migration

```cpp
class SchemaMigrator {
public:
    static std::unique_ptr<Table> migrate_v1_to_v2(const nlohmann::json& v1_data) {
        // Create new v2 schema
        auto new_schema = std::make_unique<TableSchema>("users", 2);

        // Copy existing columns
        new_schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        new_schema->add_column(ColumnDefinition("name", ColumnType::String, false));
        new_schema->add_column(ColumnDefinition("email", ColumnType::String, true));

        // Add new columns with defaults
        new_schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, true));
        new_schema->add_column(ColumnDefinition("status", ColumnType::String, true));

        new_schema->set_primary_key({"id"});

        // Create new table
        auto new_table = std::make_unique<Table>(std::move(new_schema));

        // Migrate data with defaults for new columns
        for (const auto& old_row : v1_data["rows"]) {
            std::unordered_map<std::string, CellValue> new_row;

            // Copy existing data
            new_row["id"] = old_row["id"].get<std::int64_t>();
            new_row["name"] = old_row["name"].get<std::string>();
            if (!old_row["email"].is_null()) {
                new_row["email"] = old_row["email"].get<std::string>();
            }

            // Add defaults for new columns
            new_row["created_at"] = std::string("2025-01-01T00:00:00Z");
            new_row["status"] = std::string("active");

            new_table->insert_row(new_row);
        }

        return new_table;
    }
};
```

### CSV Import/Export

```cpp
class CSVExporter {
public:
    static void export_to_csv(const Table& table, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open CSV file for writing");
        }

        const auto& schema = table.get_schema();

        // Write header
        bool first_column = true;
        for (const auto& column : schema.get_columns()) {
            if (!first_column) file << ",";
            file << "\"" << column.get_name() << "\"";
            first_column = false;
        }
        file << "\n";

        // Write data rows
        auto rows = table.get_all_rows();
        for (const auto& row : rows) {
            first_column = true;
            for (const auto& column : schema.get_columns()) {
                if (!first_column) file << ",";

                auto value = row.get_cell_value(column.get_name());
                if (value) {
                    file << "\"" << format_cell_value(*value) << "\"";
                }

                first_column = false;
            }
            file << "\n";
        }
    }

private:
    static std::string format_cell_value(const CellValue& value) {
        return std::visit([](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, std::int64_t>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, double>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "true" : "false";
            } else {
                return "null";
            }
        }, value);
    }
};
```

## Performance Considerations

### Streaming Serialization

```cpp
class StreamingSerializer {
public:
    static void stream_to_json(const Table& table, std::ostream& output) {
        output << "{\n";
        output << "  \"schema\": ";

        // Serialize schema
        auto schema_json = table.get_schema().to_json();
        output << schema_json.dump(2) << ",\n";

        output << "  \"rows\": [\n";

        // Stream rows one by one
        auto rows = table.get_all_rows();
        for (size_t i = 0; i < rows.size(); ++i) {
            if (i > 0) output << ",\n";
            output << "    " << rows[i].to_json().dump();
        }

        output << "\n  ]\n";
        output << "}\n";
    }
};
```

### Memory-Efficient Loading

```cpp
class ChunkedLoader {
public:
    static std::unique_ptr<Table> load_in_chunks(const std::string& filename,
                                                size_t chunk_size = 1000) {
        std::ifstream file(filename);
        nlohmann::json metadata;

        // Read just the schema first
        file >> metadata;

        auto schema_json = metadata["schema"];
        auto schema = TableSchema::from_json(schema_json);
        auto table = std::make_unique<Table>(std::move(schema));

        // Load rows in chunks
        const auto& rows_json = metadata["rows"];
        for (size_t i = 0; i < rows_json.size(); i += chunk_size) {
            auto txn = table->begin_transaction();

            size_t end = std::min(i + chunk_size, rows_json.size());
            for (size_t j = i; j < end; ++j) {
                auto row_data = parse_row(rows_json[j]);
                txn->insert_row(row_data);
            }

            txn->commit();

            // Optional: Report progress
            std::cout << "Loaded " << end << "/" << rows_json.size() << " rows\n";
        }

        return table;
    }
};
```

## Best Practices

### 1. Regular Backups

```cpp
class AutoBackupTable {
private:
    std::unique_ptr<Table> table_;
    std::string backup_dir_;
    std::chrono::minutes backup_interval_;
    std::thread backup_thread_;
    std::atomic<bool> running_;

public:
    AutoBackupTable(std::unique_ptr<Table> table,
                   const std::string& backup_dir,
                   std::chrono::minutes interval = std::chrono::minutes(30))
        : table_(std::move(table)), backup_dir_(backup_dir),
          backup_interval_(interval), running_(true) {

        backup_thread_ = std::thread([this]() {
            while (running_) {
                std::this_thread::sleep_for(backup_interval_);
                if (running_) {
                    create_backup();
                }
            }
        });
    }

    ~AutoBackupTable() {
        running_ = false;
        if (backup_thread_.joinable()) {
            backup_thread_.join();
        }
    }

    Table* get() { return table_.get(); }

private:
    void create_backup() {
        try {
            auto timestamp = current_timestamp();
            auto backup_name = "auto_backup_" + timestamp + ".json";
            auto backup_path = std::filesystem::path(backup_dir_) / backup_name;

            table_->save_to_file(backup_path.string());

        } catch (const std::exception& e) {
            std::cerr << "Backup failed: " << e.what() << std::endl;
        }
    }
};
```

### 2. Data Validation

```cpp
void save_with_validation(const Table& table, const std::string& filename) {
    // Serialize to memory first
    auto json_data = table.to_json();

    // Validate serialized data
    if (!validate_serialized_data(json_data)) {
        throw std::runtime_error("Data validation failed");
    }

    // Test round-trip
    try {
        auto test_table = Table::from_json(json_data);
        if (test_table->get_row_count() != table.get_row_count()) {
            throw std::runtime_error("Round-trip validation failed");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Round-trip test failed: " + std::string(e.what()));
    }

    // Save to file
    std::ofstream file(filename);
    file << json_data.dump(2);
}
```

### 3. Compression for Large Tables

```cpp
#include <fstream>
#include <sstream>

void save_compressed(const Table& table, const std::string& filename) {
    // Serialize to string
    auto json_data = table.to_json();
    std::string json_string = json_data.dump();

    // Compress using zlib or similar (pseudo-code)
    auto compressed_data = compress_string(json_string);

    // Save compressed data
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(compressed_data.data()),
               compressed_data.size());
}

std::unique_ptr<Table> load_compressed(const std::string& filename) {
    // Load compressed data
    std::ifstream file(filename, std::ios::binary);
    std::vector<std::uint8_t> compressed_data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // Decompress
    auto json_string = decompress_data(compressed_data);

    // Parse and create table
    auto json_data = nlohmann::json::parse(json_string);
    return Table::from_json(json_data);
}
```

## Examples

### Complete Backup and Restore System

```cpp
#include "tables.h"
#include <filesystem>
#include <chrono>

class TablePersistenceManager {
private:
    std::string data_dir_;
    std::string backup_dir_;

public:
    explicit TablePersistenceManager(const std::string& base_dir)
        : data_dir_(base_dir + "/data"), backup_dir_(base_dir + "/backups") {
        std::filesystem::create_directories(data_dir_);
        std::filesystem::create_directories(backup_dir_);
    }

    // Save table with automatic backup
    void save_table(const Table& table, const std::string& name) {
        auto timestamp = current_timestamp();

        // Create backup of existing data
        auto main_file = data_dir_ + "/" + name + ".json";
        if (std::filesystem::exists(main_file)) {
            auto backup_file = backup_dir_ + "/" + name + "_" + timestamp + ".json";
            std::filesystem::copy_file(main_file, backup_file);
        }

        // Save new data atomically
        auto temp_file = main_file + ".tmp";
        table.save_to_file(temp_file);
        std::filesystem::rename(temp_file, main_file);

        // Cleanup old backups (keep last 5)
        cleanup_old_backups(name, 5);
    }

    // Load table with fallback to backup
    std::unique_ptr<Table> load_table(const std::string& name) {
        auto main_file = data_dir_ + "/" + name + ".json";

        // Try to load main file
        try {
            return Table::load_from_file(main_file);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load main file: " << e.what() << std::endl;
        }

        // Fallback to most recent backup
        auto backups = list_backups(name);
        for (const auto& backup : backups) {
            try {
                auto backup_path = backup_dir_ + "/" + backup;
                auto table = Table::load_from_file(backup_path);

                std::cout << "Restored from backup: " << backup << std::endl;
                return table;

            } catch (const std::exception& e) {
                std::cerr << "Backup restore failed: " << backup << " - " << e.what() << std::endl;
            }
        }

        throw std::runtime_error("No valid data found for table: " + name);
    }

    // Export table to various formats
    void export_table(const Table& table, const std::string& filename,
                     const std::string& format) {
        if (format == "json") {
            table.save_to_file(filename);
        } else if (format == "csv") {
            CSVExporter::export_to_csv(table, filename);
        } else if (format == "binary") {
            table.save_to_file(filename, SerializationFormat::Binary);
        } else {
            throw std::runtime_error("Unsupported export format: " + format);
        }
    }

private:
    std::vector<std::string> list_backups(const std::string& name) {
        std::vector<std::string> backups;
        std::string prefix = name + "_";

        for (const auto& entry : std::filesystem::directory_iterator(backup_dir_)) {
            auto filename = entry.path().filename().string();
            if (filename.starts_with(prefix) && filename.ends_with(".json")) {
                backups.push_back(filename);
            }
        }

        std::sort(backups.rbegin(), backups.rend());  // Most recent first
        return backups;
    }

    void cleanup_old_backups(const std::string& name, size_t keep_count) {
        auto backups = list_backups(name);

        for (size_t i = keep_count; i < backups.size(); ++i) {
            auto old_backup = backup_dir_ + "/" + backups[i];
            std::filesystem::remove(old_backup);
        }
    }
};

// Usage example
void persistence_example() {
    // Create table
    auto schema = std::make_unique<TableSchema>("products", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Add some data
    table->insert_row({
        {"id", static_cast<std::int64_t>(1)},
        {"name", std::string("Product A")},
        {"price", 29.99}
    });

    // Setup persistence manager
    TablePersistenceManager manager("./app_data");

    // Save with automatic backup
    manager.save_table(*table, "products");

    // Export to different formats
    manager.export_table(*table, "products_export.json", "json");
    manager.export_table(*table, "products_export.csv", "csv");
    manager.export_table(*table, "products_export.dat", "binary");

    // Later... load the table
    auto loaded_table = manager.load_table("products");

    std::cout << "Loaded table with " << loaded_table->get_row_count() << " rows" << std::endl;
}
```

---

_See also: [Table API Reference](Tables-API.md), [Performance Guide](Tables-Performance.md), [Best Practices](Tables-BestPractices.md)_
