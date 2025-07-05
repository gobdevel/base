/*
 * @file tables.cpp
 * @brief Implementation of high-performance table data structure
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "tables.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>

namespace base {

// TableQuery implementation
TableQuery& TableQuery::select(const std::vector<std::string>& columns) {
    selected_columns_ = columns;
    return *this;
}

TableQuery& TableQuery::where(const QueryCondition& condition) {
    conditions_.push_back(condition);
    return *this;
}

TableQuery& TableQuery::where(const std::string& column, QueryOperator op, const CellValue& value) {
    conditions_.emplace_back(column, op, value);
    return *this;
}

TableQuery& TableQuery::order_by(const std::string& column, bool ascending) {
    order_by_.emplace_back(column, ascending);
    return *this;
}

TableQuery& TableQuery::limit(std::size_t count) {
    limit_ = count;
    return *this;
}

TableQuery& TableQuery::offset(std::size_t count) {
    offset_ = count;
    return *this;
}

// TableRow implementation
void TableRow::set_value(const std::string& column, const CellValue& value) {
    values_[column] = value;
    updated_at_ = std::chrono::system_clock::now();
}

CellValue TableRow::get_value(const std::string& column) const {
    auto it = values_.find(column);
    return it != values_.end() ? it->second : CellValue{std::monostate{}};
}

bool TableRow::has_column(const std::string& column) const {
    return values_.find(column) != values_.end();
}

void TableRow::increment_version() {
    version_++;
    updated_at_ = std::chrono::system_clock::now();
}

std::string TableRow::to_json() const {
    nlohmann::json j;
    j["id"] = id_;
    j["version"] = version_;
    j["created_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        created_at_.time_since_epoch()).count();
    j["updated_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        updated_at_.time_since_epoch()).count();

    nlohmann::json values_json;
    for (const auto& [key, value] : values_) {
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                values_json[key] = nullptr;
            } else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
                values_json[key] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    v.time_since_epoch()).count();
            } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
                // Convert binary data to base64 or hex string
                std::string hex_str;                        std::ostringstream hex_stream;
                        for (auto byte : v) {
                            hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
                        }
                        hex_str = hex_stream.str();
                values_json[key] = hex_str;
            } else {
                values_json[key] = v;
            }
        }, value);
    }
    j["values"] = values_json;

    return j.dump();
}

bool TableRow::from_json(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);

        id_ = j["id"];
        version_ = j["version"];
        created_at_ = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{j["created_at"]}};
        updated_at_ = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{j["updated_at"]}};

        values_.clear();
        for (const auto& [key, value] : j["values"].items()) {
            if (value.is_null()) {
                values_[key] = std::monostate{};
            } else if (value.is_number_integer()) {
                values_[key] = static_cast<std::int64_t>(value);
            } else if (value.is_number_float()) {
                values_[key] = static_cast<double>(value);
            } else if (value.is_string()) {
                values_[key] = static_cast<std::string>(value);
            } else if (value.is_boolean()) {
                values_[key] = static_cast<bool>(value);
            }
        }

        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to parse TableRow JSON: {}", e.what());
        return false;
    }
}

// TableSchema implementation
TableSchema::TableSchema(std::string name, std::uint32_t version)
    : name_(std::move(name)), version_(version) {}

void TableSchema::add_column(const ColumnDefinition& column) {
    // Check if column already exists
    for (const auto& existing : columns_) {
        if (existing.name == column.name) {
            throw std::invalid_argument("Column '" + column.name + "' already exists");
        }
    }
    columns_.push_back(column);
    increment_version();
}

void TableSchema::remove_column(const std::string& name) {
    auto it = std::find_if(columns_.begin(), columns_.end(),
        [&name](const ColumnDefinition& col) { return col.name == name; });

    if (it != columns_.end()) {
        columns_.erase(it);
        increment_version();
    } else {
        throw std::invalid_argument("Column '" + name + "' not found");
    }
}

void TableSchema::modify_column(const std::string& name, const ColumnDefinition& new_def) {
    auto it = std::find_if(columns_.begin(), columns_.end(),
        [&name](const ColumnDefinition& col) { return col.name == name; });

    if (it != columns_.end()) {
        *it = new_def;
        increment_version();
    } else {
        throw std::invalid_argument("Column '" + name + "' not found");
    }
}

std::optional<ColumnDefinition> TableSchema::get_column(const std::string& name) const {
    auto it = std::find_if(columns_.begin(), columns_.end(),
        [&name](const ColumnDefinition& col) { return col.name == name; });

    return it != columns_.end() ? std::optional<ColumnDefinition>(*it) : std::nullopt;
}

bool TableSchema::validate_row(const TableRow& row) const {
    return get_validation_errors(row).empty();
}

std::vector<std::string> TableSchema::get_validation_errors(const TableRow& row) const {
    std::vector<std::string> errors;

    for (const auto& column : columns_) {
        bool has_value = row.has_column(column.name);
        CellValue value = row.get_value(column.name);
        bool is_null = std::holds_alternative<std::monostate>(value);

        // Check not null constraint
        if (!column.nullable && (!has_value || is_null)) {
            errors.push_back("Column '" + column.name + "' cannot be null");
            continue;
        }

        if (has_value && !is_null) {
            // Type validation would go here
            // For now, we assume the application ensures type safety
        }

        // Check constraints
        for (const auto& constraint : column.constraints) {
            switch (constraint.type) {
                case ConstraintType::NotNull:
                    if (!has_value || is_null) {
                        errors.push_back("Column '" + column.name + "' violates NOT NULL constraint");
                    }
                    break;
                // Other constraint validations would be implemented here
                default:
                    break;
            }
        }
    }

    return errors;
}

void TableSchema::set_primary_key(const std::vector<std::string>& columns) {
    // Validate that all columns exist
    for (const auto& col_name : columns) {
        if (!get_column(col_name)) {
            throw std::invalid_argument("Primary key column '" + col_name + "' not found");
        }
    }
    primary_key_ = columns;
    increment_version();
}

std::string TableSchema::to_json() const {
    nlohmann::json j;
    j["name"] = name_;
    j["version"] = version_;
    j["primary_key"] = primary_key_;

    nlohmann::json columns_json = nlohmann::json::array();
    for (const auto& column : columns_) {
        nlohmann::json col_json;
        col_json["name"] = column.name;
        col_json["type"] = column_type_to_string(column.type);
        col_json["nullable"] = column.nullable;
        col_json["description"] = column.description;

        nlohmann::json constraints_json = nlohmann::json::array();
        for (const auto& constraint : column.constraints) {
            nlohmann::json constraint_json;
            constraint_json["type"] = static_cast<int>(constraint.type);
            constraint_json["name"] = constraint.name;
            constraints_json.push_back(constraint_json);
        }
        col_json["constraints"] = constraints_json;

        columns_json.push_back(col_json);
    }
    j["columns"] = columns_json;

    return j.dump(2);
}

bool TableSchema::from_json(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);

        name_ = j["name"];
        version_ = j["version"];
        primary_key_ = j["primary_key"];

        columns_.clear();
        for (const auto& col_json : j["columns"]) {
            ColumnDefinition column(
                col_json["name"],
                string_to_column_type(col_json["type"]).value_or(ColumnType::String),
                col_json.value("nullable", true)
            );
            column.description = col_json.value("description", "");

            if (col_json.contains("constraints")) {
                for (const auto& constraint_json : col_json["constraints"]) {
                    ColumnConstraint constraint(
                        static_cast<ConstraintType>(constraint_json["type"]),
                        constraint_json.value("name", "")
                    );
                    column.constraints.push_back(constraint);
                }
            }

            columns_.push_back(column);
        }

        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to parse TableSchema JSON: {}", e.what());
        return false;
    }
}

std::unique_ptr<TableSchema> TableSchema::evolve(std::uint32_t new_version) const {
    auto evolved = std::make_unique<TableSchema>(name_, new_version);
    evolved->columns_ = columns_;
    evolved->primary_key_ = primary_key_;
    return evolved;
}

// TableIndex implementation
TableIndex::TableIndex(std::string name, std::vector<std::string> columns, bool unique)
    : name_(std::move(name)), columns_(std::move(columns)), unique_(unique) {}

// Copy constructor
TableIndex::TableIndex(const TableIndex& other)
    : name_(other.name_), columns_(other.columns_), unique_(other.unique_) {
    std::shared_lock lock(other.mutex_);
    index_ = other.index_;
}

// Assignment operator
TableIndex& TableIndex::operator=(const TableIndex& other) {
    if (this != &other) {
        std::unique_lock this_lock(mutex_, std::defer_lock);
        std::shared_lock other_lock(other.mutex_, std::defer_lock);
        std::lock(this_lock, other_lock);

        name_ = other.name_;
        columns_ = other.columns_;
        unique_ = other.unique_;
        index_ = other.index_;
    }
    return *this;
}

void TableIndex::insert(const TableRow& row) {
    std::unique_lock lock(mutex_);
    auto key = extract_key(row);

    if (unique_ && index_.find(key) != index_.end()) {
        throw std::runtime_error("Unique constraint violation in index '" + name_ + "'");
    }

    index_[key].insert(row.get_id());
}

void TableIndex::remove(const TableRow& row) {
    std::unique_lock lock(mutex_);
    auto key = extract_key(row);
    auto it = index_.find(key);

    if (it != index_.end()) {
        it->second.erase(row.get_id());
        if (it->second.empty()) {
            index_.erase(it);
        }
    }
}

void TableIndex::update(const TableRow& old_row, const TableRow& new_row) {
    std::unique_lock lock(mutex_);

    auto old_key = extract_key(old_row);
    auto new_key = extract_key(new_row);

    // Remove from old position
    auto old_it = index_.find(old_key);
    if (old_it != index_.end()) {
        old_it->second.erase(old_row.get_id());
        if (old_it->second.empty()) {
            index_.erase(old_it);
        }
    }

    // Add to new position
    if (unique_ && new_key != old_key && index_.find(new_key) != index_.end()) {
        throw std::runtime_error("Unique constraint violation in index '" + name_ + "'");
    }

    index_[new_key].insert(new_row.get_id());
}

std::vector<std::size_t> TableIndex::find_exact(const std::vector<CellValue>& key) const {
    std::shared_lock lock(mutex_);
    auto it = index_.find(key);

    if (it != index_.end()) {
        return std::vector<std::size_t>(it->second.begin(), it->second.end());
    }

    return {};
}

std::vector<std::size_t> TableIndex::find_range(const std::vector<CellValue>& start_key,
                                               const std::vector<CellValue>& end_key) const {
    std::shared_lock lock(mutex_);
    std::vector<std::size_t> result;

    auto start_it = index_.lower_bound(start_key);
    auto end_it = index_.upper_bound(end_key);

    for (auto it = start_it; it != end_it; ++it) {
        result.insert(result.end(), it->second.begin(), it->second.end());
    }

    return result;
}

std::size_t TableIndex::size() const {
    std::shared_lock lock(mutex_);
    return index_.size();
}

void TableIndex::clear() {
    std::unique_lock lock(mutex_);
    index_.clear();
}

std::vector<CellValue> TableIndex::extract_key(const TableRow& row) const {
    std::vector<CellValue> key;
    key.reserve(columns_.size());

    for (const auto& column : columns_) {
        key.push_back(row.get_value(column));
    }

    return key;
}

// TableTransaction implementation
TableTransaction::TableTransaction(Table* table, std::string id)
    : table_(table), id_(std::move(id)) {}

TableTransaction::~TableTransaction() {
    if (active_ && !committed_ && !rolled_back_) {
        rollback();
    }
}

void TableTransaction::begin() {
    if (active_) {
        throw std::runtime_error("Transaction already active");
    }
    active_ = true;
    change_log_.clear();
}

void TableTransaction::commit() {
    if (!active_) {
        throw std::runtime_error("No active transaction");
    }
    if (committed_ || rolled_back_) {
        throw std::runtime_error("Transaction already finalized");
    }

    committed_ = true;
    active_ = false;
    change_log_.clear();
}

void TableTransaction::rollback() {
    if (!active_) {
        throw std::runtime_error("No active transaction");
    }
    if (committed_ || rolled_back_) {
        throw std::runtime_error("Transaction already finalized");
    }

    // Implement rollback logic here
    // This would reverse all operations in the change log

    rolled_back_ = true;
    active_ = false;
    change_log_.clear();
}

// Table implementation
Table::Table(std::unique_ptr<TableSchema> schema)
    : schema_(std::move(schema))
    , created_at_(std::chrono::system_clock::now())
    , last_modified_(std::chrono::system_clock::now()) {

    // Create primary key index if defined
    if (!schema_->get_primary_key().empty()) {
        create_index("__primary_key", schema_->get_primary_key(), true);
    }
}

std::size_t Table::insert_row(const std::unordered_map<std::string, CellValue>& values) {
    std::unique_lock lock(table_mutex_);

    auto row_id = next_row_id_++;
    auto row = std::make_unique<TableRow>(row_id);

    // Set all provided values
    for (const auto& [column, value] : values) {
        row->set_value(column, value);
    }

    // Validate row
    if (!schema_->validate_row(*row)) {
        auto errors = schema_->get_validation_errors(*row);
        std::string error_msg = "Row validation failed: ";
        for (const auto& error : errors) {
            error_msg += error + "; ";
        }
        throw std::runtime_error(error_msg);
    }

    // Update indexes
    update_indexes_on_insert(*row);

    // Store the row
    rows_[row_id] = std::move(row);

    // Update statistics
    total_inserts_++;
    last_modified_ = std::chrono::system_clock::now();

    // Fire change event
    ChangeEvent event(ChangeType::RowInserted, schema_->get_name());
    event.row_id = row_id;
    event.new_values = values;
    fire_change_event(event);

    return row_id;
}

bool Table::update_row(std::size_t row_id, const std::unordered_map<std::string, CellValue>& values) {
    std::unique_lock lock(table_mutex_);

    auto it = rows_.find(row_id);
    if (it == rows_.end()) {
        return false;
    }

    auto& row = *it->second;
    auto old_values = row.get_all_values();

    // Create updated row for validation
    TableRow updated_row = row;
    for (const auto& [column, value] : values) {
        updated_row.set_value(column, value);
    }

    // Validate updated row
    if (!schema_->validate_row(updated_row)) {
        auto errors = schema_->get_validation_errors(updated_row);
        std::string error_msg = "Row validation failed: ";
        for (const auto& error : errors) {
            error_msg += error + "; ";
        }
        throw std::runtime_error(error_msg);
    }

    // Update indexes
    update_indexes_on_update(row, updated_row);

    // Apply changes to actual row
    for (const auto& [column, value] : values) {
        row.set_value(column, value);
    }
    row.increment_version();

    // Update statistics
    total_updates_++;
    last_modified_ = std::chrono::system_clock::now();

    // Fire change event
    ChangeEvent event(ChangeType::RowUpdated, schema_->get_name());
    event.row_id = row_id;
    event.old_values = old_values;
    event.new_values = row.get_all_values();
    fire_change_event(event);

    return true;
}

bool Table::delete_row(std::size_t row_id) {
    std::unique_lock lock(table_mutex_);

    auto it = rows_.find(row_id);
    if (it == rows_.end()) {
        return false;
    }

    auto& row = *it->second;
    auto old_values = row.get_all_values();

    // Update indexes
    update_indexes_on_delete(row);

    // Remove the row
    rows_.erase(it);

    // Update statistics
    total_deletes_++;
    last_modified_ = std::chrono::system_clock::now();

    // Fire change event
    ChangeEvent event(ChangeType::RowDeleted, schema_->get_name());
    event.row_id = row_id;
    event.old_values = old_values;
    fire_change_event(event);

    return true;
}

std::optional<TableRow> Table::get_row(std::size_t row_id) const {
    std::shared_lock lock(table_mutex_);

    auto it = rows_.find(row_id);
    return it != rows_.end() ? std::optional<TableRow>(*it->second) : std::nullopt;
}

std::vector<TableRow> Table::get_all_rows() const {
    std::shared_lock lock(table_mutex_);

    std::vector<TableRow> result;
    result.reserve(rows_.size());

    for (const auto& [id, row] : rows_) {
        result.push_back(*row);
    }

    return result;
}

std::size_t Table::get_row_count() const {
    std::shared_lock lock(table_mutex_);
    return rows_.size();
}

std::vector<TableRow> Table::query(const TableQuery& query) const {
    std::shared_lock lock(table_mutex_);

    std::vector<TableRow> result;
    result.reserve(rows_.size());

    // Get all rows first
    for (const auto& [id, row] : rows_) {
        result.push_back(*row);
    }

    // Apply query filters
    return apply_query_filters(result, query);
}

std::vector<TableRow> Table::find_by_index(const std::string& index_name,
                                          const std::vector<CellValue>& key) const {
    std::shared_lock lock(table_mutex_);

    auto index_it = indexes_.find(index_name);
    if (index_it == indexes_.end()) {
        throw std::runtime_error("Index '" + index_name + "' not found");
    }

    auto row_ids = index_it->second->find_exact(key);
    std::vector<TableRow> result;
    result.reserve(row_ids.size());

    for (auto row_id : row_ids) {
        auto row_it = rows_.find(row_id);
        if (row_it != rows_.end()) {
            result.push_back(*row_it->second);
        }
    }

    return result;
}

void Table::evolve_schema(std::unique_ptr<TableSchema> new_schema) {
    std::unique_lock lock(table_mutex_);

    // Validate that evolution is possible
    if (new_schema->get_name() != schema_->get_name()) {
        throw std::runtime_error("Cannot change table name during schema evolution");
    }

    if (new_schema->get_version() <= schema_->get_version()) {
        throw std::runtime_error("New schema version must be greater than current version");
    }

    // Apply schema evolution
    schema_ = std::move(new_schema);

    // Rebuild indexes if necessary
    // This is a simplified approach - in practice, you'd want more sophisticated migration

    // Fire change event
    ChangeEvent event(ChangeType::SchemaChanged, schema_->get_name());
    fire_change_event(event);
}

void Table::create_index(const std::string& name, const std::vector<std::string>& columns,
                        bool unique) {
    std::unique_lock lock(table_mutex_);

    if (indexes_.find(name) != indexes_.end()) {
        throw std::runtime_error("Index '" + name + "' already exists");
    }

    // Validate columns exist
    for (const auto& column : columns) {
        if (!schema_->get_column(column)) {
            throw std::runtime_error("Column '" + column + "' not found");
        }
    }

    auto index = std::make_unique<TableIndex>(name, columns, unique);

    // Populate index with existing rows
    for (const auto& [id, row] : rows_) {
        index->insert(*row);
    }

    indexes_[name] = std::move(index);

    // Fire change event
    ChangeEvent event(ChangeType::IndexCreated, schema_->get_name());
    fire_change_event(event);
}

void Table::drop_index(const std::string& name) {
    std::unique_lock lock(table_mutex_);

    if (name == "__primary_key") {
        throw std::runtime_error("Cannot drop primary key index");
    }

    auto it = indexes_.find(name);
    if (it == indexes_.end()) {
        throw std::runtime_error("Index '" + name + "' not found");
    }

    indexes_.erase(it);

    // Fire change event
    ChangeEvent event(ChangeType::IndexDropped, schema_->get_name());
    fire_change_event(event);
}

std::vector<std::string> Table::get_index_names() const {
    std::shared_lock lock(table_mutex_);

    std::vector<std::string> names;
    names.reserve(indexes_.size());

    for (const auto& [name, index] : indexes_) {
        names.push_back(name);
    }

    return names;
}

void Table::add_change_callback(const std::string& name, ChangeCallback callback) {
    std::unique_lock lock(table_mutex_);
    change_callbacks_[name] = std::move(callback);
}

void Table::remove_change_callback(const std::string& name) {
    std::unique_lock lock(table_mutex_);
    change_callbacks_.erase(name);
}

std::unique_ptr<TableTransaction> Table::begin_transaction() {
    return std::make_unique<TableTransaction>(this, "tx_" + std::to_string(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()));
}

bool Table::save_to_file(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file) {
            return false;
        }

        file << to_json();
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to save table to file {}: {}", filename, e.what());
        return false;
    }
}

bool Table::load_from_file(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file) {
            return false;
        }

        std::string json((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

        return from_json(json);
    } catch (const std::exception& e) {
        Logger::error("Failed to load table from file {}: {}", filename, e.what());
        return false;
    }
}

std::string Table::to_json() const {
    std::shared_lock lock(table_mutex_);

    nlohmann::json j;
    j["schema"] = nlohmann::json::parse(schema_->to_json());

    nlohmann::json rows_json = nlohmann::json::array();
    for (const auto& [id, row] : rows_) {
        rows_json.push_back(nlohmann::json::parse(row->to_json()));
    }
    j["rows"] = rows_json;

    nlohmann::json indexes_json = nlohmann::json::array();
    for (const auto& [name, index] : indexes_) {
        nlohmann::json index_json;
        index_json["name"] = name;
        index_json["columns"] = index->get_columns();
        index_json["unique"] = index->is_unique();
        indexes_json.push_back(index_json);
    }
    j["indexes"] = indexes_json;

    // Statistics
    j["statistics"] = {
        {"total_inserts", total_inserts_.load()},
        {"total_updates", total_updates_.load()},
        {"total_deletes", total_deletes_.load()},
        {"created_at", std::chrono::duration_cast<std::chrono::milliseconds>(
            created_at_.time_since_epoch()).count()},
        {"last_modified", std::chrono::duration_cast<std::chrono::milliseconds>(
            last_modified_.load().time_since_epoch()).count()}
    };

    return j.dump(2);
}

bool Table::from_json(const std::string& json) {
    try {
        auto j = nlohmann::json::parse(json);
        Logger::info(__PRETTY_FUNCTION__);

        // Load schema
        auto schema_json = j["schema"].dump();
        auto new_schema = std::make_unique<TableSchema>("", 1);
        if (!new_schema->from_json(schema_json)) {
            return false;
        }
        schema_ = std::move(new_schema);
        Logger::info("Loaded schema: {}", schema_->get_name());

        // Load rows
        std::unique_lock lock(table_mutex_);
        rows_.clear();
        Logger::info("Loading rows...");

        for (const auto& row_json : j["rows"]) {
            auto row_str = row_json.dump();
            auto row = std::make_unique<TableRow>(0);
            if (row->from_json(row_str)) {
                auto row_id = row->get_id();
                rows_[row_id] = std::move(row);
                next_row_id_ = std::max(next_row_id_.load(), row_id + 1);
            }
        }

        // Clear indexes while holding the lock
        indexes_.clear();

        // Release lock before calling create_index to avoid deadlock
        lock.unlock();

        // Rebuild indexes
        Logger::info("Rebuilding indexes...");
        if (j.contains("indexes")) {
            for (const auto& index_json : j["indexes"]) {
                create_index(
                    index_json["name"],
                    index_json["columns"],
                    index_json.value("unique", false)
                );
            }
        }

        // Load statistics
        Logger::info("Loading statistics...");
        if (j.contains("statistics")) {
            const auto& stats = j["statistics"];
            total_inserts_ = stats.value("total_inserts", 0);
            total_updates_ = stats.value("total_updates", 0);
            total_deletes_ = stats.value("total_deletes", 0);

            if (stats.contains("created_at")) {
                created_at_ = std::chrono::system_clock::time_point{
                    std::chrono::milliseconds{stats["created_at"]}};
            }
            if (stats.contains("last_modified")) {
                last_modified_ = std::chrono::system_clock::time_point{
                    std::chrono::milliseconds{stats["last_modified"]}};
            }
        }

        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to parse Table JSON: {}", e.what());
        return false;
    }
}

Table::Statistics Table::get_statistics() const {
    std::shared_lock lock(table_mutex_);

    return {
        .row_count = rows_.size(),
        .index_count = indexes_.size(),
        .schema_version = schema_->get_version(),
        .created_at = created_at_,
        .last_modified = last_modified_.load(),
        .total_inserts = total_inserts_.load(),
        .total_updates = total_updates_.load(),
        .total_deletes = total_deletes_.load()
    };
}

// TableIterator implementation
TableIterator::TableIterator(const std::unordered_map<std::size_t, std::unique_ptr<TableRow>>& rows,
                             std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator it)
    : current_(it), end_(rows.end()) {}

TableIterator::reference TableIterator::operator*() const {
    return *(current_->second);
}

TableIterator::pointer TableIterator::operator->() const {
    return current_->second.get();
}

TableIterator& TableIterator::operator++() {
    ++current_;
    return *this;
}

TableIterator TableIterator::operator++(int) {
    auto temp = *this;
    ++current_;
    return temp;
}

bool TableIterator::operator==(const TableIterator& other) const {
    return current_ == other.current_;
}

bool TableIterator::operator!=(const TableIterator& other) const {
    return current_ != other.current_;
}

// TableConstIterator implementation
TableConstIterator::TableConstIterator(const std::unordered_map<std::size_t, std::unique_ptr<TableRow>>& rows,
                                       std::unordered_map<std::size_t, std::unique_ptr<TableRow>>::const_iterator it)
    : current_(it), end_(rows.end()) {}

TableConstIterator::reference TableConstIterator::operator*() const {
    return *(current_->second);
}

TableConstIterator::pointer TableConstIterator::operator->() const {
    return current_->second.get();
}

TableConstIterator& TableConstIterator::operator++() {
    ++current_;
    return *this;
}

TableConstIterator TableConstIterator::operator++(int) {
    auto temp = *this;
    ++current_;
    return temp;
}

bool TableConstIterator::operator==(const TableConstIterator& other) const {
    return current_ == other.current_;
}

bool TableConstIterator::operator!=(const TableConstIterator& other) const {
    return current_ != other.current_;
}

// Table copy constructor
Table::Table(const Table& other)
    : schema_(std::make_unique<TableSchema>(*other.schema_))
    , created_at_(std::chrono::system_clock::now())
    , last_modified_(std::chrono::system_clock::now())
    , next_row_id_(other.next_row_id_.load())
    , total_inserts_(other.total_inserts_.load())
    , total_updates_(other.total_updates_.load())
    , total_deletes_(other.total_deletes_.load())
    , concurrent_access_enabled_(other.concurrent_access_enabled_) {

    std::shared_lock other_lock(other.table_mutex_);

    // Deep copy all rows
    for (const auto& [id, row_ptr] : other.rows_) {
        rows_[id] = std::make_unique<TableRow>(*row_ptr);
    }

    // Recreate indexes
    for (const auto& [name, index_ptr] : other.indexes_) {
        indexes_[name] = std::make_unique<TableIndex>(*index_ptr);
    }

    // Copy change callbacks (shallow copy of function objects)
    change_callbacks_ = other.change_callbacks_;
}

// Table copy assignment operator
Table& Table::operator=(const Table& other) {
    if (this != &other) {
        std::unique_lock this_lock(table_mutex_, std::defer_lock);
        std::shared_lock other_lock(other.table_mutex_, std::defer_lock);
        std::lock(this_lock, other_lock);

        // Clear current state
        rows_.clear();
        indexes_.clear();
        change_callbacks_.clear();

        // Copy schema
        schema_ = std::make_unique<TableSchema>(*other.schema_);

        // Copy metadata
        next_row_id_ = other.next_row_id_.load();
        total_inserts_ = other.total_inserts_.load();
        total_updates_ = other.total_updates_.load();
        total_deletes_ = other.total_deletes_.load();
        concurrent_access_enabled_ = other.concurrent_access_enabled_;
        last_modified_ = std::chrono::system_clock::now();

        // Deep copy all rows
        for (const auto& [id, row_ptr] : other.rows_) {
            rows_[id] = std::make_unique<TableRow>(*row_ptr);
        }

        // Recreate indexes
        for (const auto& [name, index_ptr] : other.indexes_) {
            indexes_[name] = std::make_unique<TableIndex>(*index_ptr);
        }

        // Copy change callbacks
        change_callbacks_ = other.change_callbacks_;
    }
    return *this;
}

// Table move constructor
Table::Table(Table&& other) noexcept
    : schema_(std::move(other.schema_))
    , rows_(std::move(other.rows_))
    , indexes_(std::move(other.indexes_))
    , change_callbacks_(std::move(other.change_callbacks_))
    , created_at_(other.created_at_)
    , last_modified_(other.last_modified_.load())
    , next_row_id_(other.next_row_id_.load())
    , total_inserts_(other.total_inserts_.load())
    , total_updates_(other.total_updates_.load())
    , total_deletes_(other.total_deletes_.load())
    , concurrent_access_enabled_(other.concurrent_access_enabled_) {

    // Reset the other table to a valid but empty state
    other.next_row_id_ = 1;
    other.total_inserts_ = 0;
    other.total_updates_ = 0;
    other.total_deletes_ = 0;
    other.created_at_ = std::chrono::system_clock::now();
    other.last_modified_ = std::chrono::system_clock::now();
}

// Table move assignment operator
Table& Table::operator=(Table&& other) noexcept {
    if (this != &other) {
        std::unique_lock this_lock(table_mutex_, std::defer_lock);
        std::unique_lock other_lock(other.table_mutex_, std::defer_lock);
        std::lock(this_lock, other_lock);

        // Move all data
        schema_ = std::move(other.schema_);
        rows_ = std::move(other.rows_);
        indexes_ = std::move(other.indexes_);
        change_callbacks_ = std::move(other.change_callbacks_);
        created_at_ = other.created_at_;
        last_modified_ = other.last_modified_.load();
        next_row_id_ = other.next_row_id_.load();
        total_inserts_ = other.total_inserts_.load();
        total_updates_ = other.total_updates_.load();
        total_deletes_ = other.total_deletes_.load();
        concurrent_access_enabled_ = other.concurrent_access_enabled_;

        // Reset the other table
        other.next_row_id_ = 1;
        other.total_inserts_ = 0;
        other.total_updates_ = 0;
        other.total_deletes_ = 0;
        other.created_at_ = std::chrono::system_clock::now();
        other.last_modified_ = std::chrono::system_clock::now();
    }
    return *this;
}

// Iterator methods
Table::iterator Table::begin() {
    return TableIterator(rows_, rows_.begin());
}

Table::iterator Table::end() {
    return TableIterator(rows_, rows_.end());
}

Table::const_iterator Table::begin() const {
    return TableConstIterator(rows_, rows_.begin());
}

Table::const_iterator Table::end() const {
    return TableConstIterator(rows_, rows_.end());
}

Table::const_iterator Table::cbegin() const {
    return TableConstIterator(rows_, rows_.begin());
}

Table::const_iterator Table::cend() const {
    return TableConstIterator(rows_, rows_.end());
}

// Utility methods
void Table::clear() {
    std::unique_lock lock(table_mutex_);

    size_t initial_row_count = rows_.size();

    // Fire delete events for all rows
    if (!change_callbacks_.empty()) {
        for (const auto& [id, row_ptr] : rows_) {
            ChangeEvent event{ChangeType::RowDeleted, schema_->get_name()};
            event.row_id = id;
            event.old_values = row_ptr->get_all_values();
            fire_change_event(event);
        }
    }

    // Clear all data
    rows_.clear();

    // Clear indexes
    indexes_.clear();

    // Update statistics
    total_deletes_ += initial_row_count;
    last_modified_ = std::chrono::system_clock::now();

    // Release lock before recreating primary key index to avoid deadlock
    lock.unlock();

    // Recreate primary key index if needed
    if (!schema_->get_primary_key().empty()) {
        create_index("__primary_key", schema_->get_primary_key(), true);
    }
}

bool Table::empty() const {
    std::shared_lock lock(table_mutex_);
    return rows_.empty();
}

std::unique_ptr<Table> Table::clone() const {
    std::shared_lock lock(table_mutex_);

    // Create a new table with the same schema
    auto cloned_table = std::make_unique<Table>(std::make_unique<TableSchema>(*schema_));

    // Copy all configuration
    cloned_table->concurrent_access_enabled_ = concurrent_access_enabled_;

    // Copy all rows
    for (const auto& [id, row_ptr] : rows_) {
        cloned_table->rows_[id] = std::make_unique<TableRow>(*row_ptr);
    }

    // Recreate indexes with same configuration
    for (const auto& [name, index_ptr] : indexes_) {
        cloned_table->indexes_[name] = std::make_unique<TableIndex>(*index_ptr);
    }

    // Copy change callbacks
    cloned_table->change_callbacks_ = change_callbacks_;

    // Copy statistics (but update timing)
    cloned_table->next_row_id_ = next_row_id_.load();
    cloned_table->total_inserts_ = total_inserts_.load();
    cloned_table->total_updates_ = total_updates_.load();
    cloned_table->total_deletes_ = total_deletes_.load();
    cloned_table->created_at_ = std::chrono::system_clock::now();
    cloned_table->last_modified_ = std::chrono::system_clock::now();

    return cloned_table;
}

void Table::merge_from(const Table& other) {
    if (this == &other) {
        return; // Cannot merge from self
    }

    std::unique_lock this_lock(table_mutex_, std::defer_lock);
    std::shared_lock other_lock(other.table_mutex_, std::defer_lock);
    std::lock(this_lock, other_lock);

    // Verify schema compatibility (same column names and types)
    const auto& this_columns = schema_->get_columns();
    const auto& other_columns = other.schema_->get_columns();

    if (this_columns.size() != other_columns.size()) {
        throw std::runtime_error("Cannot merge tables with different schemas: column count mismatch");
    }

    for (const auto& column : this_columns) {
        auto other_it = std::find_if(other_columns.begin(), other_columns.end(),
            [&column](const ColumnDefinition& other_col) {
                return other_col.name == column.name && other_col.type == column.type;
            });

        if (other_it == other_columns.end()) {
            throw std::runtime_error("Cannot merge tables with different schemas: column '" +
                                    column.name + "' type mismatch or missing");
        }
    }

    // Merge rows (avoiding ID conflicts)
    std::size_t id_offset = next_row_id_.load();

    for (const auto& [other_id, other_row_ptr] : other.rows_) {
        // Create a new row with adjusted ID
        auto new_id = id_offset + other_id;
        auto new_row = std::make_unique<TableRow>(*other_row_ptr);
        new_row->set_id(new_id);

        // Validate the row against this table's schema
        if (!schema_->validate_row(*new_row)) {
            continue; // Skip invalid rows
        }

        // Insert the row
        rows_[new_id] = std::move(new_row);

        // Update indexes
        update_indexes_on_insert(*rows_[new_id]);

        // Fire change event
        if (!change_callbacks_.empty()) {
            ChangeEvent event{ChangeType::RowInserted, schema_->get_name()};
            event.row_id = new_id;
            event.new_values = rows_[new_id]->get_all_values();
            fire_change_event(event);
        }

        total_inserts_++;
    }

    // Update next row ID to avoid future conflicts
    next_row_id_ = id_offset + other.next_row_id_.load();
    last_modified_ = std::chrono::system_clock::now();
}

void Table::swap(Table& other) noexcept {
    if (this == &other) {
        return;
    }

    std::unique_lock this_lock(table_mutex_, std::defer_lock);
    std::unique_lock other_lock(other.table_mutex_, std::defer_lock);
    std::lock(this_lock, other_lock);

    // Swap all data members
    std::swap(schema_, other.schema_);
    std::swap(rows_, other.rows_);
    std::swap(indexes_, other.indexes_);
    std::swap(change_callbacks_, other.change_callbacks_);
    std::swap(concurrent_access_enabled_, other.concurrent_access_enabled_);
    std::swap(created_at_, other.created_at_);

    // Swap atomic values
    auto this_next_id = next_row_id_.load();
    auto this_inserts = total_inserts_.load();
    auto this_updates = total_updates_.load();
    auto this_deletes = total_deletes_.load();
    auto this_modified = last_modified_.load();

    next_row_id_ = other.next_row_id_.load();
    total_inserts_ = other.total_inserts_.load();
    total_updates_ = other.total_updates_.load();
    total_deletes_ = other.total_deletes_.load();
    last_modified_ = other.last_modified_.load();

    other.next_row_id_ = this_next_id;
    other.total_inserts_ = this_inserts;
    other.total_updates_ = this_updates;
    other.total_deletes_ = this_deletes;
    other.last_modified_ = this_modified;
}

// Private helper methods
void Table::fire_change_event(const ChangeEvent& event) {
    for (const auto& [name, callback] : change_callbacks_) {
        try {
            callback(event);
        } catch (const std::exception& e) {
            Logger::error("Change callback '{}' failed: {}", name, e.what());
        }
    }
}

bool Table::validate_row_constraints(const TableRow& row) const {
    return schema_->validate_row(row);
}

void Table::update_indexes_on_insert(const TableRow& row) {
    for (const auto& [name, index] : indexes_) {
        index->insert(row);
    }
}

void Table::update_indexes_on_update(const TableRow& old_row, const TableRow& new_row) {
    for (const auto& [name, index] : indexes_) {
        index->update(old_row, new_row);
    }
}

void Table::update_indexes_on_delete(const TableRow& row) {
    for (const auto& [name, index] : indexes_) {
        index->remove(row);
    }
}

bool Table::evaluate_condition(const TableRow& row, const QueryCondition& condition) const {
    auto value = row.get_value(condition.column);
    return cell_utils::compare_values(value, condition.value, condition.op);
}

std::vector<TableRow> Table::apply_query_filters(const std::vector<TableRow>& rows,
                                                 const TableQuery& query) const {
    std::vector<TableRow> filtered = rows;

    // Apply WHERE conditions
    if (!query.get_conditions().empty()) {
        filtered.erase(
            std::remove_if(filtered.begin(), filtered.end(),
                [this, &query](const TableRow& row) {
                    for (const auto& condition : query.get_conditions()) {
                        if (!evaluate_condition(row, condition)) {
                            return true;  // Remove this row
                        }
                    }
                    return false;  // Keep this row
                }),
            filtered.end()
        );
    }

    // Apply ORDER BY
    if (!query.get_order_by().empty()) {
        std::sort(filtered.begin(), filtered.end(),
            [&query](const TableRow& a, const TableRow& b) {
                for (const auto& [column, ascending] : query.get_order_by()) {
                    auto val_a = a.get_value(column);
                    auto val_b = b.get_value(column);

                    if (val_a != val_b) {
                        bool result = cell_utils::compare_values(val_a, val_b, QueryOperator::LessThan);
                        return ascending ? result : !result;
                    }
                }
                return false;  // Equal
            });
    }

    // Apply OFFSET and LIMIT
    std::size_t start = std::min(query.get_offset(), filtered.size());
    std::size_t count = std::min(query.get_limit(), filtered.size() - start);

    if (start > 0 || count < filtered.size()) {
        std::vector<TableRow> result;
        result.reserve(count);
        auto begin_it = filtered.begin() + start;
        auto end_it = begin_it + count;
        std::copy(begin_it, end_it, std::back_inserter(result));
        return result;
    }

    return filtered;
}

// TablePager implementation
TablePager::TablePager(const Table& table, const TableDumpOptions& options)
    : table_(table), options_(options) {}

void TablePager::cache_filtered_rows() const {
    if (!rows_cached_) {
        if (options_.filter_query.get_conditions().empty() &&
            options_.columns_to_show.empty()) {
            // No filtering needed, get all rows
            filtered_rows_ = table_.get_all_rows();
        } else {
            // Apply filters
            filtered_rows_ = table_.query(options_.filter_query);
        }
        rows_cached_ = true;
    }
}

std::size_t TablePager::get_total_pages() const {
    cache_filtered_rows();
    if (options_.page_size == 0 || filtered_rows_.empty()) {
        return 1;
    }
    return (filtered_rows_.size() + options_.page_size - 1) / options_.page_size;
}

std::size_t TablePager::get_total_rows() const {
    cache_filtered_rows();
    return filtered_rows_.size();
}

void TablePager::show_page(std::size_t page_number) const {
    cache_filtered_rows();

    if (options_.page_size == 0) {
        // No paging, show all rows
        std::cout << format_table_page(filtered_rows_) << std::endl;
        return;
    }

    std::size_t total_pages = get_total_pages();
    if (page_number >= total_pages) {
        std::cout << "Page " << page_number << " does not exist. Total pages: " << total_pages << std::endl;
        return;
    }

    std::size_t start_idx = page_number * options_.page_size;
    std::size_t end_idx = std::min(start_idx + options_.page_size, filtered_rows_.size());

    std::vector<TableRow> page_rows(filtered_rows_.begin() + start_idx,
                                    filtered_rows_.begin() + end_idx);

    std::cout << format_table_page(page_rows) << std::endl;
    print_navigation_info();
}

void TablePager::show_next_page() {
    if (current_page_ + 1 < get_total_pages()) {
        current_page_++;
        show_page(current_page_);
    } else {
        std::cout << "Already at last page." << std::endl;
    }
}

void TablePager::show_previous_page() {
    if (current_page_ > 0) {
        current_page_--;
        show_page(current_page_);
    } else {
        std::cout << "Already at first page." << std::endl;
    }
}

void TablePager::show_first_page() {
    current_page_ = 0;
    show_page(current_page_);
}

void TablePager::show_last_page() {
    current_page_ = get_total_pages() - 1;
    show_page(current_page_);
}

std::string TablePager::get_page_as_string(std::size_t page_number) const {
    cache_filtered_rows();

    if (options_.page_size == 0) {
        return format_table_page(filtered_rows_);
    }

    std::size_t total_pages = get_total_pages();
    if (page_number >= total_pages) {
        return "Page " + std::to_string(page_number) + " does not exist. Total pages: " + std::to_string(total_pages);
    }

    std::size_t start_idx = page_number * options_.page_size;
    std::size_t end_idx = std::min(start_idx + options_.page_size, filtered_rows_.size());

    std::vector<TableRow> page_rows(filtered_rows_.begin() + start_idx,
                                    filtered_rows_.begin() + end_idx);

    return format_table_page(page_rows);
}

void TablePager::start_interactive_mode() {
    current_page_ = 0;
    show_page(current_page_);

    std::string input;
    std::cout << "\nCommands: n(ext), p(rev), f(irst), l(ast), q(uit), <number> (go to page)" << std::endl;

    while (true) {
        std::cout << "Page " << (current_page_ + 1) << "/" << get_total_pages() << " > ";
        std::getline(std::cin, input);

        if (input.empty()) {
            show_next_page();
        } else if (input == "q" || input == "quit") {
            break;
        } else if (input == "n" || input == "next") {
            show_next_page();
        } else if (input == "p" || input == "prev") {
            show_previous_page();
        } else if (input == "f" || input == "first") {
            show_first_page();
        } else if (input == "l" || input == "last") {
            show_last_page();
        } else {
            try {
                std::size_t page_num = std::stoull(input);
                if (page_num > 0) {
                    current_page_ = page_num - 1;  // Convert to 0-based
                    show_page(current_page_);
                }
            } catch (const std::exception&) {
                std::cout << "Invalid command. Use n, p, f, l, q, or page number." << std::endl;
            }
        }
    }
}

void TablePager::print_navigation_info() const {
    if (options_.page_size > 0) {
        std::cout << "\n--- Page " << (current_page_ + 1) << "/" << get_total_pages()
                  << " (showing " << std::min(options_.page_size, get_total_rows() - current_page_ * options_.page_size)
                  << " of " << get_total_rows() << " total rows) ---" << std::endl;
    }
}

std::vector<std::string> TablePager::get_display_columns() const {
    if (!options_.columns_to_show.empty()) {
        return options_.columns_to_show;
    }

    // Get all column names from schema
    std::vector<std::string> columns;
    for (const auto& col : table_.get_schema().get_columns()) {
        columns.push_back(col.name);
    }
    return columns;
}

std::string TablePager::truncate_value(const std::string& value, std::size_t max_width) const {
    if (!options_.truncate_long_values || value.length() <= max_width) {
        return value;
    }

    if (max_width <= 3) {
        return "...";
    }

    return value.substr(0, max_width - 3) + "...";
}

std::string TablePager::format_table_page(const std::vector<TableRow>& page_rows) const {
    switch (options_.format) {
        case TableOutputFormat::ASCII:
            return format_ascii_table(page_rows);
        case TableOutputFormat::CSV:
            return format_csv_table(page_rows);
        case TableOutputFormat::TSV:
            return format_csv_table(page_rows);  // Same as CSV but with tabs
        case TableOutputFormat::JSON:
            return format_json_table(page_rows);
        case TableOutputFormat::Markdown:
            return format_markdown_table(page_rows);
        default:
            return format_ascii_table(page_rows);
    }
}

std::string TablePager::format_ascii_table(const std::vector<TableRow>& rows) const {
    auto columns = get_display_columns();
    if (columns.empty()) {
        return "No columns to display.";
    }

    std::ostringstream result;
    std::vector<std::size_t> column_widths;

    // Calculate column widths
    for (const auto& col : columns) {
        std::size_t max_width = options_.show_headers ? col.length() : 0;

        for (const auto& row : rows) {
            std::string value = cell_utils::to_string(row.get_value(col));
            if (cell_utils::is_null(row.get_value(col))) {
                value = options_.null_representation;
            }
            max_width = std::max(max_width, value.length());
        }

        column_widths.push_back(std::min(max_width, options_.max_column_width));
    }

    // Add row number column if enabled
    std::size_t row_num_width = 0;
    if (options_.show_row_numbers) {
        row_num_width = std::to_string(rows.size()).length();
        row_num_width = std::max(row_num_width, std::string("Row").length());
    }

    // Draw top border (simple ASCII)
    result << "+";
    if (options_.show_row_numbers) {
        result << std::string(row_num_width + 2, '-') << "+";
    }
    for (std::size_t i = 0; i < columns.size(); ++i) {
        result << std::string(column_widths[i] + 2, '-');
        if (i < columns.size() - 1) result << "+";
    }
    result << "+\n";

    // Draw headers
    if (options_.show_headers) {
        result << "|";
        if (options_.show_row_numbers) {
            result << " " << std::setw(row_num_width) << std::left << "Row" << " |";
        }
        for (std::size_t i = 0; i < columns.size(); ++i) {
            std::string header = truncate_value(columns[i], column_widths[i]);
            result << " " << std::setw(column_widths[i]) << std::left << header << " ";
            if (i < columns.size() - 1) result << "|";
        }
        result << "|\n";

        // Draw separator
        result << "+";
        if (options_.show_row_numbers) {
            result << std::string(row_num_width + 2, '-') << "+";
        }
        for (std::size_t i = 0; i < columns.size(); ++i) {
            result << std::string(column_widths[i] + 2, '-');
            if (i < columns.size() - 1) result << "+";
        }
        result << "+\n";
    }

    // Draw data rows
    for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
        const auto& row = rows[row_idx];
        result << "|";

        if (options_.show_row_numbers) {
            result << " " << std::setw(row_num_width) << std::right << (row_idx + 1) << " |";
        }

        for (std::size_t i = 0; i < columns.size(); ++i) {
            std::string value = cell_utils::to_string(row.get_value(columns[i]));
            if (cell_utils::is_null(row.get_value(columns[i]))) {
                value = options_.null_representation;
            }
            value = truncate_value(value, column_widths[i]);

            result << " " << std::setw(column_widths[i]) << std::left << value << " ";
            if (i < columns.size() - 1) result << "|";
        }
        result << "|\n";
    }

    // Draw bottom border
    result << "+";
    if (options_.show_row_numbers) {
        result << std::string(row_num_width + 2, '-') << "+";
    }
    for (std::size_t i = 0; i < columns.size(); ++i) {
        result << std::string(column_widths[i] + 2, '-');
        if (i < columns.size() - 1) result << "+";
    }
    result << "+";

    return result.str();
}

std::string TablePager::format_csv_table(const std::vector<TableRow>& rows) const {
    auto columns = get_display_columns();
    std::ostringstream result;
    char separator = (options_.format == TableOutputFormat::TSV) ? '\t' : ',';

    // Write headers
    if (options_.show_headers) {
        if (options_.show_row_numbers) {
            result << "Row" << separator;
        }
        for (std::size_t i = 0; i < columns.size(); ++i) {
            result << columns[i];
            if (i < columns.size() - 1) result << separator;
        }
        result << "\n";
    }

    // Write data rows
    for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
        const auto& row = rows[row_idx];

        if (options_.show_row_numbers) {
            result << (row_idx + 1) << separator;
        }

        for (std::size_t i = 0; i < columns.size(); ++i) {
            std::string value = cell_utils::to_string(row.get_value(columns[i]));
            if (cell_utils::is_null(row.get_value(columns[i]))) {
                value = options_.null_representation;
            }

            // Escape CSV values if needed
            if (options_.format == TableOutputFormat::CSV &&
                (value.find(',') != std::string::npos || value.find('"') != std::string::npos)) {
                std::string escaped = "\"";
                for (char c : value) {
                    if (c == '"') escaped += "\"\"";
                    else escaped += c;
                }
                escaped += "\"";
                value = escaped;
            }

            result << value;
            if (i < columns.size() - 1) result << separator;
        }
        result << "\n";
    }

    return result.str();
}

std::string TablePager::format_json_table(const std::vector<TableRow>& rows) const {
    auto columns = get_display_columns();
    nlohmann::json result = nlohmann::json::array();

    for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
        const auto& row = rows[row_idx];
        nlohmann::json row_json;

        if (options_.show_row_numbers) {
            row_json["_row_number"] = row_idx + 1;
        }

        for (const auto& col : columns) {
            auto value = row.get_value(col);
            if (cell_utils::is_null(value)) {
                row_json[col] = nullptr;
            } else {
                std::visit([&](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        row_json[col] = nullptr;
                    } else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
                        row_json[col] = std::chrono::duration_cast<std::chrono::milliseconds>(
                            v.time_since_epoch()).count();
                    } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
                        // Convert to hex string
                        std::string hex_str;
                        std::ostringstream hex_stream;
                        for (auto byte : v) {
                            hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
                        }
                        hex_str = hex_stream.str();
                        row_json[col] = hex_str;
                    } else {
                        row_json[col] = v;
                    }
                }, value);
            }
        }

        result.push_back(row_json);
    }

    return result.dump(2);
}

std::string TablePager::format_markdown_table(const std::vector<TableRow>& rows) const {
    auto columns = get_display_columns();
    std::ostringstream result;

    // Write headers
    if (options_.show_headers) {
        result << "|";
        if (options_.show_row_numbers) {
            result << " Row |";
        }
        for (const auto& col : columns) {
            result << " " << col << " |";
        }
        result << "\n";

        // Write separator
        result << "|";
        if (options_.show_row_numbers) {
            result << "---:|";
        }
        for (std::size_t i = 0; i < columns.size(); ++i) {
            result << "----|";
        }
        result << "\n";
    }

    // Write data rows
    for (std::size_t row_idx = 0; row_idx < rows.size(); ++row_idx) {
        const auto& row = rows[row_idx];
        result << "|";

        if (options_.show_row_numbers) {
            result << " " << (row_idx + 1) << " |";
        }

        for (const auto& col : columns) {
            std::string value = cell_utils::to_string(row.get_value(col));
            if (cell_utils::is_null(row.get_value(col))) {
                value = options_.null_representation;
            }
            value = truncate_value(value, options_.max_column_width);

            // Escape markdown special characters
            std::string escaped;
            for (char c : value) {
                if (c == '|') escaped += "\\|";
                else escaped += c;
            }

            result << " " << escaped << " |";
        }
        result << "\n";
    }

    return result.str();
}

// Table dump methods implementation
void Table::dump(const TableDumpOptions& options) const {
    dump_to_stream(std::cout, options);
}

void Table::dump_to_stream(std::ostream& stream, const TableDumpOptions& options) const {
    stream << dump_to_string(options);
}

std::string Table::dump_to_string(const TableDumpOptions& options) const {
    auto pager = create_pager(options);
    if (options.page_size == 0) {
        return pager->get_page_as_string(0);
    } else {
        // For paged output, return first page with navigation info
        std::ostringstream result;
        result << pager->get_page_as_string(0);
        if (pager->get_total_pages() > 1) {
            result << "\n--- Page 1/" << pager->get_total_pages()
                   << " (showing " << std::min(options.page_size, pager->get_total_rows())
                   << " of " << pager->get_total_rows() << " total rows) ---";
        }
        return result.str();
    }
}

std::unique_ptr<TablePager> Table::create_pager(const TableDumpOptions& options) const {
    return std::make_unique<TablePager>(*this, options);
}

void Table::print_summary() const {
    std::shared_lock lock(table_mutex_);

    std::cout << "=== Table Summary ===" << std::endl;
    std::cout << "Name: " << schema_->get_name() << std::endl;
    std::cout << "Schema Version: " << schema_->get_version() << std::endl;
    std::cout << "Rows: " << rows_.size() << std::endl;
    std::cout << "Columns: " << schema_->get_columns().size() << std::endl;
    std::cout << "Indexes: " << indexes_.size() << std::endl;

    if (!schema_->get_primary_key().empty()) {
        std::cout << "Primary Key: ";
        for (std::size_t i = 0; i < schema_->get_primary_key().size(); i++) {
            std::cout << schema_->get_primary_key()[i];
            if (i < schema_->get_primary_key().size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }

    auto stats = get_statistics();
    std::cout << "Created: " << std::chrono::duration_cast<std::chrono::seconds>(
        stats.created_at.time_since_epoch()).count() << " seconds since epoch" << std::endl;
    std::cout << "Last Modified: " << std::chrono::duration_cast<std::chrono::seconds>(
        stats.last_modified.time_since_epoch()).count() << " seconds since epoch" << std::endl;
}

void Table::print_schema() const {
    std::shared_lock lock(table_mutex_);

    std::cout << "=== Table Schema ===" << std::endl;
    std::cout << "Table: " << schema_->get_name() << " (version " << schema_->get_version() << ")" << std::endl;
    std::cout << std::endl;

    // Column information table
    std::cout << "+--------------------+-------------+----------+-----------------------------+" << std::endl;
    std::cout << "| Column Name        | Type        | Nullable | Description                 |" << std::endl;
    std::cout << "+--------------------+-------------+----------+-----------------------------+" << std::endl;

    for (const auto& col : schema_->get_columns()) {
        std::string name = col.name.length() > 18 ? col.name.substr(0, 15) + "..." : col.name;
        std::string type = column_type_to_string(col.type);
        std::string nullable = col.nullable ? "Yes" : "No";
        std::string desc = col.description.length() > 27 ? col.description.substr(0, 24) + "..." : col.description;

        std::cout << "| " << std::setw(18) << std::left << name
                  << " | " << std::setw(11) << std::left << type
                  << " | " << std::setw(8) << std::left << nullable
                  << " | " << std::setw(27) << std::left << desc << " |" << std::endl;
    }

    std::cout << "+--------------------+-------------+----------+-----------------------------+" << std::endl;

    // Primary key info
    if (!schema_->get_primary_key().empty()) {
        std::cout << std::endl << "Primary Key: ";
        for (std::size_t i = 0; i < schema_->get_primary_key().size(); i++) {
            std::cout << schema_->get_primary_key()[i];
            if (i < schema_->get_primary_key().size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }

    // Index information
    if (!indexes_.empty()) {
        std::cout << std::endl << "Indexes:" << std::endl;
        for (const auto& [name, index] : indexes_) {
            std::cout << "  - " << name << " (";
            const auto& cols = index->get_columns();
            for (std::size_t i = 0; i < cols.size(); ++i) {
                std::cout << cols[i];
                if (i < cols.size() - 1) std::cout << ", ";
            }
            std::cout << ")";
            if (index->is_unique()) std::cout << " [UNIQUE]";
            std::cout << std::endl;
        }
    }
}

void Table::print_statistics() const {
    auto stats = get_statistics();

    std::cout << "=== Table Statistics ===" << std::endl;
    std::cout << "Table: " << schema_->get_name() << std::endl;
    std::cout << "Schema Version: " << stats.schema_version << std::endl;
    std::cout << std::endl;

    std::cout << "Data Statistics:" << std::endl;
    std::cout << "  Rows: " << stats.row_count << std::endl;
    std::cout << "  Indexes: " << stats.index_count << std::endl;
    std::cout << std::endl;

    std::cout << "Operation Statistics:" << std::endl;
    std::cout << "  Total Inserts: " << stats.total_inserts << std::endl;
    std::cout << "  Total Updates: " << stats.total_updates << std::endl;
    std::cout << "  Total Deletes: " << stats.total_deletes << std::endl;
    std::cout << "  Total Operations: " << (stats.total_inserts + stats.total_updates + stats.total_deletes) << std::endl;
    std::cout << std::endl;

    auto created_time_t = std::chrono::system_clock::to_time_t(stats.created_at);
    auto modified_time_t = std::chrono::system_clock::to_time_t(stats.last_modified);

    std::cout << "Timing Information:" << std::endl;
    std::cout << "  Created: " << std::put_time(std::localtime(&created_time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
    std::cout << "  Last Modified: " << std::put_time(std::localtime(&modified_time_t), "%Y-%m-%d %H:%M:%S") << std::endl;

    // Memory estimation (rough)
    std::size_t estimated_memory = stats.row_count * 100;  // Rough estimate
    estimated_memory += stats.index_count * 1000;  // Rough index overhead

    std::cout << std::endl;
    std::cout << "Estimated Memory Usage: ~" << estimated_memory << " bytes" << std::endl;

    // Performance metrics if available
    if (stats.total_inserts + stats.total_updates + stats.total_deletes > 0) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            stats.last_modified - stats.created_at).count();
        if (duration > 0) {
            double ops_per_sec = static_cast<double>(stats.total_inserts + stats.total_updates + stats.total_deletes) / duration;
            std::cout << "Average Operations/Second: " << std::fixed << std::setprecision(2) << ops_per_sec << std::endl;
        }
    }
}

// cell_utils namespace implementation
namespace cell_utils {

std::string to_string(const CellValue& value) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "NULL";
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return v;
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
            auto time_t = std::chrono::system_clock::to_time_t(v);
            std::ostringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            return ss.str();
        } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
            std::ostringstream ss;
            ss << "0x";
            for (auto byte : v) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
            }
            return ss.str();
        } else {
            return "UNKNOWN";
        }
    }, value);
}

CellValue from_string(const std::string& str, ColumnType type) {
    switch (type) {
        case ColumnType::Integer:
            try {
                return static_cast<std::int64_t>(std::stoll(str));
            } catch (const std::exception&) {
                return std::monostate{};
            }
        case ColumnType::Double:
            try {
                return std::stod(str);
            } catch (const std::exception&) {
                return std::monostate{};
            }
        case ColumnType::String:
            return str;
        case ColumnType::Boolean:
            if (str == "true" || str == "1") return true;
            if (str == "false" || str == "0") return false;
            return std::monostate{};
        case ColumnType::DateTime:
            // Simplified datetime parsing - in a real implementation you'd use proper parsing
            try {
                auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                return std::chrono::system_clock::from_time_t(time_t);
            } catch (const std::exception&) {
                return std::monostate{};
            }
        case ColumnType::Binary:
            // Simplified binary parsing - expect hex string
            if (str.substr(0, 2) == "0x") {
                std::vector<std::uint8_t> data;
                for (std::size_t i = 2; i < str.length(); i += 2) {
                    try {
                        auto byte = static_cast<std::uint8_t>(std::stoul(str.substr(i, 2), nullptr, 16));
                        data.push_back(byte);
                    } catch (const std::exception&) {
                        return std::monostate{};
                    }
                }
                return data;
            }
            return std::monostate{};
        case ColumnType::Json:
            return str;  // JSON as string
        default:
            return std::monostate{};
    }
}

bool compare_values(const CellValue& left, const CellValue& right, QueryOperator op) {
    // Handle null values
    bool left_null = is_null(left);
    bool right_null = is_null(right);

    if (op == QueryOperator::IsNull) {
        return left_null;
    }
    if (op == QueryOperator::IsNotNull) {
        return !left_null;
    }

    if (left_null || right_null) {
        return false;  // NULL comparisons are always false except for IS NULL/IS NOT NULL
    }

    // Type must match for most comparisons
    if (left.index() != right.index()) {
        return false;
    }

    switch (op) {
        case QueryOperator::Equal:
            return left == right;
        case QueryOperator::NotEqual:
            return left != right;
        case QueryOperator::LessThan:
            return left < right;
        case QueryOperator::LessThanOrEqual:
            return left <= right;
        case QueryOperator::GreaterThan:
            return left > right;
        case QueryOperator::GreaterThanOrEqual:
            return left >= right;
        case QueryOperator::Like:
            // Simple LIKE implementation for strings
            if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
                const auto& left_str = std::get<std::string>(left);
                const auto& right_str = std::get<std::string>(right);
                return left_str.find(right_str) != std::string::npos;
            }
            return false;
        case QueryOperator::In:
            // For simplicity, not implementing IN operator here
            return false;
        case QueryOperator::Between:
            // For simplicity, not implementing BETWEEN operator here
            return false;
        default:
            return false;
    }
}

ColumnType get_value_type(const CellValue& value) {
    return std::visit([](const auto& v) -> ColumnType {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t>) {
            return ColumnType::Integer;
        } else if constexpr (std::is_same_v<T, double>) {
            return ColumnType::Double;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return ColumnType::String;
        } else if constexpr (std::is_same_v<T, bool>) {
            return ColumnType::Boolean;
        } else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
            return ColumnType::DateTime;
        } else if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
            return ColumnType::Binary;
        } else {
            return ColumnType::String;  // Default fallback
        }
    }, value);
}

bool is_null(const CellValue& value) {
    return std::holds_alternative<std::monostate>(value);
}

CellValue make_null() {
    return std::monostate{};
}

} // namespace cell_utils

} // namespace base
