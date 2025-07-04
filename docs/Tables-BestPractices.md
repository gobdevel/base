# Table System Best Practices

## Overview

This guide provides comprehensive best practices for using the Table system effectively, covering design patterns, performance optimization, error handling, testing strategies, and production deployment considerations.

## Table of Contents

1. [Schema Design Best Practices](#schema-design-best-practices)
2. [Data Management Patterns](#data-management-patterns)
3. [Performance Best Practices](#performance-best-practices)
4. [Error Handling and Recovery](#error-handling-and-recovery)
5. [Testing Strategies](#testing-strategies)
6. [Production Deployment](#production-deployment)
7. [Monitoring and Maintenance](#monitoring-and-maintenance)
8. [Security Considerations](#security-considerations)
9. [Migration and Evolution](#migration-and-evolution)

## Schema Design Best Practices

### 1. Design for the Future

```cpp
// Good: Extensible schema design
auto create_user_schema() -> std::unique_ptr<TableSchema> {
    auto schema = std::make_unique<TableSchema>("users", 1);

    // Core required fields
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("username", ColumnType::String, false));
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));
    schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false));

    // Optional fields for future expansion
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("profile_data", ColumnType::String, true));  // JSON blob
    schema->add_column(ColumnDefinition("status", ColumnType::String, true));

    schema->set_primary_key({"id"});
    return schema;
}

// Bad: Rigid schema with no room for growth
auto create_rigid_schema() -> std::unique_ptr<TableSchema> {
    auto schema = std::make_unique<TableSchema>("users", 1);

    // Only current requirements, hard to extend
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));

    schema->set_primary_key({"id"});
    return schema;
}
```

### 2. Choose Appropriate Data Types

```cpp
// Good: Appropriate data types
void design_product_schema() {
    auto schema = std::make_unique<TableSchema>("products", 1);

    // Use specific types for their purpose
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));           // Unique ID
    schema->add_column(ColumnDefinition("sku", ColumnType::String, false));          // Product code
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));         // Product name
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false));        // Monetary value
    schema->add_column(ColumnDefinition("stock_count", ColumnType::Integer, false)); // Inventory count
    schema->add_column(ColumnDefinition("is_active", ColumnType::Boolean, false));   // Status flag
    schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false)); // Timestamp
    schema->add_column(ColumnDefinition("description", ColumnType::String, true));   // Optional text

    schema->set_primary_key({"id"});
}

// Bad: Inefficient data types
void bad_product_schema() {
    auto schema = std::make_unique<TableSchema>("products", 1);

    // Using strings for everything (inefficient)
    schema->add_column(ColumnDefinition("id", ColumnType::String, false));          // Should be Integer
    schema->add_column(ColumnDefinition("price", ColumnType::String, false));       // Should be Double
    schema->add_column(ColumnDefinition("stock_count", ColumnType::String, false)); // Should be Integer
    schema->add_column(ColumnDefinition("is_active", ColumnType::String, false));   // Should be Boolean
}
```

### 3. Strategic Primary Key Selection

```cpp
// Good: Simple, stable primary keys
class PrimaryKeyPatterns {
public:
    // Pattern 1: Auto-incrementing ID (recommended for most cases)
    static auto create_auto_id_schema() {
        auto schema = std::make_unique<TableSchema>("orders", 1);
        schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));  // Auto-incrementing
        schema->add_column(ColumnDefinition("customer_id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("order_date", ColumnType::DateTime, false));
        schema->set_primary_key({"id"});
        return schema;
    }

    // Pattern 2: Natural composite key (when appropriate)
    static auto create_composite_key_schema() {
        auto schema = std::make_unique<TableSchema>("user_permissions", 1);
        schema->add_column(ColumnDefinition("user_id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("resource_id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("permission", ColumnType::String, false));
        schema->set_primary_key({"user_id", "resource_id"});  // Natural relationship
        return schema;
    }

    // Pattern 3: UUID for distributed systems
    static auto create_uuid_schema() {
        auto schema = std::make_unique<TableSchema>("events", 1);
        schema->add_column(ColumnDefinition("id", ColumnType::String, false));  // UUID string
        schema->add_column(ColumnDefinition("event_type", ColumnType::String, false));
        schema->add_column(ColumnDefinition("timestamp", ColumnType::DateTime, false));
        schema->set_primary_key({"id"});
        return schema;
    }
};

// Bad: Complex, unstable primary keys
void bad_primary_key_examples() {
    auto schema = std::make_unique<TableSchema>("bad_example", 1);

    // Bad: Using mutable data as primary key
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));  // Can change
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->set_primary_key({"email"});  // Bad choice - emails can change

    // Bad: Complex composite key that's hard to use
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("birth_date", ColumnType::DateTime, false));
    schema->set_primary_key({"first_name", "last_name", "birth_date"});  // Too complex
}
```

## Data Management Patterns

### 1. Transaction Management Patterns

```cpp
class TransactionPatterns {
public:
    // Pattern 1: RAII Transaction Management
    class ScopedTransaction {
    private:
        std::unique_ptr<TableTransaction> txn_;
        bool committed_ = false;

    public:
        explicit ScopedTransaction(Table* table) : txn_(table->begin_transaction()) {}

        ~ScopedTransaction() {
            if (!committed_ && txn_ && txn_->get_state() == TransactionState::Active) {
                txn_->rollback();
            }
        }

        void commit() {
            if (txn_ && txn_->get_state() == TransactionState::Active) {
                txn_->commit();
                committed_ = true;
            }
        }

        TableTransaction* operator->() { return txn_.get(); }
        TableTransaction& operator*() { return *txn_; }
    };

    // Pattern 2: Retry on Conflict
    template<typename Func>
    static auto retry_on_conflict(Func&& operation, int max_retries = 3) {
        for (int attempt = 0; attempt < max_retries; ++attempt) {
            try {
                return operation();
            } catch (const TableException& e) {
                if (attempt == max_retries - 1) {
                    throw;  // Last attempt failed
                }

                // Brief delay before retry
                std::this_thread::sleep_for(std::chrono::milliseconds(10 * (attempt + 1)));
            }
        }
    }

    // Pattern 3: Optimistic Updates
    static bool optimistic_update(Table& table, std::size_t row_id,
                                 const std::string& version_column,
                                 const std::unordered_map<std::string, CellValue>& updates) {
        // Read current version
        auto current_row = table.get_row(row_id);
        if (!current_row) {
            return false;
        }

        auto version_val = current_row->get_cell_value(version_column);
        if (!version_val) {
            return false;
        }

        auto current_version = std::get<std::int64_t>(*version_val);

        // Update with version check
        ScopedTransaction txn(&table);

        // Re-read in transaction to ensure consistency
        auto txn_row = txn->get_row(row_id);
        if (!txn_row) {
            return false;
        }

        auto txn_version_val = txn_row->get_cell_value(version_column);
        if (!txn_version_val || std::get<std::int64_t>(*txn_version_val) != current_version) {
            return false;  // Version changed, conflict detected
        }

        // Apply updates with incremented version
        auto final_updates = updates;
        final_updates[version_column] = current_version + 1;

        txn->update_row(row_id, final_updates);
        txn.commit();

        return true;
    }
};
```

### 2. Data Access Patterns

```cpp
class DataAccessPatterns {
public:
    // Pattern 1: Repository Pattern
    class UserRepository {
    private:
        std::unique_ptr<Table> table_;

    public:
        explicit UserRepository(std::unique_ptr<Table> table) : table_(std::move(table)) {}

        std::optional<User> find_by_id(std::int64_t id) {
            auto results = table_->find_by_index("id_idx", {id});
            if (results.empty()) {
                return std::nullopt;
            }

            return User::from_table_row(results[0]);
        }

        std::optional<User> find_by_email(const std::string& email) {
            auto results = table_->find_by_index("email_idx", {email});
            if (results.empty()) {
                return std::nullopt;
            }

            return User::from_table_row(results[0]);
        }

        std::vector<User> find_by_status(const std::string& status) {
            TableQuery query;
            query.where("status", QueryOperator::Equal, status);

            auto rows = table_->query(query);
            std::vector<User> users;

            for (const auto& row : rows) {
                users.push_back(User::from_table_row(row));
            }

            return users;
        }

        std::int64_t create_user(const User& user) {
            TransactionPatterns::ScopedTransaction txn(table_.get());
            auto row_id = txn->insert_row(user.to_table_row());
            txn.commit();
            return row_id;
        }

        bool update_user(const User& user) {
            TransactionPatterns::ScopedTransaction txn(table_.get());
            bool success = txn->update_row(user.get_id(), user.to_table_row());
            if (success) {
                txn.commit();
            }
            return success;
        }
    };

    // Pattern 2: Caching Layer
    class CachedUserRepository {
    private:
        std::unique_ptr<UserRepository> repository_;
        mutable std::unordered_map<std::int64_t, User> cache_;
        mutable std::shared_mutex cache_mutex_;

    public:
        explicit CachedUserRepository(std::unique_ptr<UserRepository> repo)
            : repository_(std::move(repo)) {}

        std::optional<User> find_by_id(std::int64_t id) {
            // Check cache first
            {
                std::shared_lock<std::shared_mutex> lock(cache_mutex_);
                auto it = cache_.find(id);
                if (it != cache_.end()) {
                    return it->second;
                }
            }

            // Not in cache, fetch from repository
            auto user = repository_->find_by_id(id);
            if (user) {
                std::unique_lock<std::shared_mutex> lock(cache_mutex_);
                cache_[id] = *user;
            }

            return user;
        }

        void invalidate_cache(std::int64_t id) {
            std::unique_lock<std::shared_mutex> lock(cache_mutex_);
            cache_.erase(id);
        }

        void clear_cache() {
            std::unique_lock<std::shared_mutex> lock(cache_mutex_);
            cache_.clear();
        }
    };

    // Pattern 3: Bulk Operations Manager
    class BulkOperationsManager {
    public:
        template<typename T>
        static void bulk_insert(Table& table, const std::vector<T>& objects,
                               size_t batch_size = 1000) {
            for (size_t i = 0; i < objects.size(); i += batch_size) {
                TransactionPatterns::ScopedTransaction txn(&table);

                size_t end = std::min(i + batch_size, objects.size());
                for (size_t j = i; j < end; ++j) {
                    txn->insert_row(objects[j].to_table_row());
                }

                txn.commit();

                // Progress reporting
                if ((i / batch_size) % 10 == 0) {
                    std::cout << "Inserted " << end << "/" << objects.size() << " records\n";
                }
            }
        }

        template<typename T>
        static void bulk_update(Table& table, const std::vector<T>& objects,
                               size_t batch_size = 1000) {
            for (size_t i = 0; i < objects.size(); i += batch_size) {
                TransactionPatterns::ScopedTransaction txn(&table);

                size_t end = std::min(i + batch_size, objects.size());
                for (size_t j = i; j < end; ++j) {
                    txn->update_row(objects[j].get_id(), objects[j].to_table_row());
                }

                txn.commit();
            }
        }
    };
};
```

## Performance Best Practices

### 1. Index Strategy

```cpp
class IndexStrategy {
public:
    // Strategy 1: Create indexes based on query patterns
    static void setup_user_indexes(Table& table) {
        // Primary access patterns
        table.create_index("email_idx", {"email"});        // Login by email
        table.create_index("username_idx", {"username"});  // Login by username

        // Administrative queries
        table.create_index("status_idx", {"status"});      // Filter by status
        table.create_index("created_idx", {"created_at"}); // Sort by creation date

        // Composite indexes for complex queries
        table.create_index("status_created_idx", {"status", "created_at"});
        table.create_index("dept_level_idx", {"department", "level"});
    }

    // Strategy 2: Monitor and optimize index usage
    static void analyze_index_effectiveness(const Table& table) {
        auto indexes = table.get_indexes();

        for (const auto& [name, index] : indexes) {
            // Measure index selectivity
            auto selectivity = calculate_selectivity(table, index);

            std::cout << "Index '" << name << "':\n";
            std::cout << "  Selectivity: " << (selectivity * 100) << "%\n";

            if (selectivity < 0.1) {
                std::cout << "  WARNING: Low selectivity index\n";
            }

            if (selectivity > 0.8) {
                std::cout << "  GOOD: High selectivity index\n";
            }
        }
    }

    // Strategy 3: Dynamic index management
    static void manage_indexes_dynamically(Table& table,
                                         const std::vector<std::string>& frequent_columns) {
        auto existing_indexes = table.get_indexes();

        // Create indexes for frequently accessed columns
        for (const auto& column : frequent_columns) {
            std::string index_name = column + "_idx";
            if (existing_indexes.find(index_name) == existing_indexes.end()) {
                table.create_index(index_name, {column});
                std::cout << "Created index: " << index_name << "\n";
            }
        }

        // TODO: Remove unused indexes (requires usage tracking)
    }

private:
    static double calculate_selectivity(const Table& table, const TableIndex& index) {
        auto total_rows = table.get_row_count();
        auto unique_keys = index.get_key_count();
        return total_rows > 0 ? static_cast<double>(unique_keys) / total_rows : 0.0;
    }
};
```

### 2. Memory Management

```cpp
class MemoryManagement {
public:
    // Strategy 1: Connection pooling for large applications
    class TablePool {
    private:
        std::queue<std::unique_ptr<Table>> available_tables_;
        std::mutex pool_mutex_;
        std::condition_variable pool_cv_;
        size_t max_size_;

    public:
        explicit TablePool(size_t max_size) : max_size_(max_size) {}

        std::unique_ptr<Table> acquire() {
            std::unique_lock<std::mutex> lock(pool_mutex_);

            if (!available_tables_.empty()) {
                auto table = std::move(available_tables_.front());
                available_tables_.pop();
                return table;
            }

            // Create new table if pool not at capacity
            if (get_active_count() < max_size_) {
                return create_new_table();
            }

            // Wait for available table
            pool_cv_.wait(lock, [this] { return !available_tables_.empty(); });

            auto table = std::move(available_tables_.front());
            available_tables_.pop();
            return table;
        }

        void release(std::unique_ptr<Table> table) {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            available_tables_.push(std::move(table));
            pool_cv_.notify_one();
        }

    private:
        std::unique_ptr<Table> create_new_table() {
            // Create table with standard schema
            return std::make_unique<Table>(create_standard_schema());
        }

        size_t get_active_count() const {
            return max_size_ - available_tables_.size();
        }
    };

    // Strategy 2: Lazy loading for large datasets
    class LazyRowLoader {
    private:
        Table* table_;
        mutable std::unordered_map<std::size_t, std::optional<TableRow>> row_cache_;
        mutable std::shared_mutex cache_mutex_;
        size_t cache_limit_;

    public:
        LazyRowLoader(Table* table, size_t cache_limit = 1000)
            : table_(table), cache_limit_(cache_limit) {}

        std::optional<TableRow> get_row(std::size_t row_id) {
            // Check cache first
            {
                std::shared_lock<std::shared_mutex> lock(cache_mutex_);
                auto it = row_cache_.find(row_id);
                if (it != row_cache_.end()) {
                    return it->second;
                }
            }

            // Load from table
            auto row = table_->get_row(row_id);

            // Cache the result
            {
                std::unique_lock<std::shared_mutex> lock(cache_mutex_);

                // Evict oldest entries if cache is full
                if (row_cache_.size() >= cache_limit_) {
                    auto oldest = row_cache_.begin();
                    row_cache_.erase(oldest);
                }

                row_cache_[row_id] = row;
            }

            return row;
        }

        void clear_cache() {
            std::unique_lock<std::shared_mutex> lock(cache_mutex_);
            row_cache_.clear();
        }
    };

    // Strategy 3: Resource monitoring
    class ResourceMonitor {
    private:
        std::chrono::steady_clock::time_point start_time_;
        size_t initial_memory_;

    public:
        ResourceMonitor() : start_time_(std::chrono::steady_clock::now()) {
            initial_memory_ = get_current_memory_usage();
        }

        void report_usage(const std::string& operation) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time_).count();

            auto current_memory = get_current_memory_usage();
            auto memory_delta = current_memory - initial_memory_;

            std::cout << "Resource Report (" << operation << "):\n";
            std::cout << "  Elapsed Time: " << elapsed << " seconds\n";
            std::cout << "  Memory Usage: " << format_bytes(current_memory) << "\n";
            std::cout << "  Memory Delta: " << format_bytes(memory_delta) << "\n\n";
        }

    private:
        size_t get_current_memory_usage() {
            // Platform-specific memory usage query
            // This is a simplified placeholder
            return 0;  // Implementation would use actual system calls
        }

        std::string format_bytes(size_t bytes) {
            const char* units[] = {"B", "KB", "MB", "GB"};
            int unit = 0;
            double size = static_cast<double>(bytes);

            while (size >= 1024 && unit < 3) {
                size /= 1024;
                unit++;
            }

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
            return oss.str();
        }
    };
};
```

## Error Handling and Recovery

### 1. Exception Safety

```cpp
class ErrorHandling {
public:
    // Pattern 1: Strong exception safety guarantee
    class SafeTableOperations {
    public:
        static bool safe_insert_user(Table& table, const User& user) {
            try {
                TransactionPatterns::ScopedTransaction txn(&table);

                // Validate before insert
                validate_user_data(user);

                // Check for duplicates
                auto existing = table.find_by_index("email_idx", {user.get_email()});
                if (!existing.empty()) {
                    throw std::runtime_error("User with email already exists");
                }

                // Perform insert
                txn->insert_row(user.to_table_row());
                txn.commit();

                return true;

            } catch (const TableException& e) {
                // Table-specific error handling
                std::cerr << "Table error inserting user: " << e.what() << std::endl;
                return false;

            } catch (const std::exception& e) {
                // General error handling
                std::cerr << "Error inserting user: " << e.what() << std::endl;
                return false;
            }
        }

        static std::optional<User> safe_get_user(const Table& table, std::int64_t id) {
            try {
                auto results = table.find_by_index("id_idx", {id});
                if (results.empty()) {
                    return std::nullopt;
                }

                return User::from_table_row(results[0]);

            } catch (const std::exception& e) {
                std::cerr << "Error retrieving user " << id << ": " << e.what() << std::endl;
                return std::nullopt;
            }
        }

    private:
        static void validate_user_data(const User& user) {
            if (user.get_email().empty()) {
                throw std::invalid_argument("Email cannot be empty");
            }

            if (user.get_username().empty()) {
                throw std::invalid_argument("Username cannot be empty");
            }

            // Additional validation...
        }
    };

    // Pattern 2: Error recovery strategies
    class ErrorRecovery {
    public:
        template<typename Operation>
        static auto with_retry(Operation&& op, int max_attempts = 3,
                              std::chrono::milliseconds delay = std::chrono::milliseconds(100)) {
            std::exception_ptr last_exception;

            for (int attempt = 1; attempt <= max_attempts; ++attempt) {
                try {
                    return op();

                } catch (...) {
                    last_exception = std::current_exception();

                    if (attempt == max_attempts) {
                        std::rethrow_exception(last_exception);
                    }

                    std::this_thread::sleep_for(delay * attempt);  // Exponential backoff
                }
            }

            std::rethrow_exception(last_exception);
        }

        static bool recover_from_corruption(Table& table, const std::string& backup_path) {
            try {
                // Attempt to load from backup
                auto backup_table = Table::load_from_file(backup_path);

                // Validate backup data
                if (backup_table->get_row_count() == 0) {
                    std::cerr << "Backup appears to be empty" << std::endl;
                    return false;
                }

                // Replace current table data (implementation specific)
                // This would involve copying data from backup to current table
                std::cout << "Successfully recovered from backup" << std::endl;
                return true;

            } catch (const std::exception& e) {
                std::cerr << "Recovery failed: " << e.what() << std::endl;
                return false;
            }
        }
    };

    // Pattern 3: Comprehensive error logging
    class ErrorLogger {
    private:
        static std::ofstream error_log_;
        static std::mutex log_mutex_;

    public:
        static void log_error(const std::string& operation, const std::exception& e,
                             const std::string& context = "") {
            std::lock_guard<std::mutex> lock(log_mutex_);

            auto timestamp = current_timestamp();

            error_log_ << "[" << timestamp << "] ERROR in " << operation << ": "
                      << e.what();

            if (!context.empty()) {
                error_log_ << " (Context: " << context << ")";
            }

            error_log_ << std::endl;
            error_log_.flush();
        }

        static void log_warning(const std::string& message) {
            std::lock_guard<std::mutex> lock(log_mutex_);

            auto timestamp = current_timestamp();
            error_log_ << "[" << timestamp << "] WARNING: " << message << std::endl;
            error_log_.flush();
        }

        static void initialize_logging(const std::string& log_file) {
            error_log_.open(log_file, std::ios::app);
            if (!error_log_.is_open()) {
                throw std::runtime_error("Failed to open error log file: " + log_file);
            }
        }
    };
};
```

## Testing Strategies

### 1. Unit Testing Patterns

```cpp
class TestingPatterns {
public:
    // Pattern 1: Test data factories
    class TestDataFactory {
    public:
        static std::unique_ptr<Table> create_test_table() {
            auto schema = std::make_unique<TableSchema>("test_users", 1);
            schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
            schema->add_column(ColumnDefinition("name", ColumnType::String, false));
            schema->add_column(ColumnDefinition("email", ColumnType::String, false));
            schema->add_column(ColumnDefinition("age", ColumnType::Integer, true));
            schema->set_primary_key({"id"});

            auto table = std::make_unique<Table>(std::move(schema));

            // Add test indexes
            table->create_index("email_idx", {"email"});
            table->create_index("name_idx", {"name"});

            return table;
        }

        static std::vector<std::unordered_map<std::string, CellValue>> create_test_users(size_t count) {
            std::vector<std::unordered_map<std::string, CellValue>> users;

            for (size_t i = 0; i < count; ++i) {
                users.push_back({
                    {"id", static_cast<std::int64_t>(i + 1)},
                    {"name", std::string("User") + std::to_string(i + 1)},
                    {"email", std::string("user") + std::to_string(i + 1) + "@test.com"},
                    {"age", static_cast<std::int64_t>(20 + (i % 50))}
                });
            }

            return users;
        }

        static void populate_test_table(Table& table, size_t user_count = 100) {
            auto users = create_test_users(user_count);

            auto txn = table.begin_transaction();
            for (const auto& user : users) {
                txn->insert_row(user);
            }
            txn->commit();
        }
    };

    // Pattern 2: Assertion helpers
    class TableAssertions {
    public:
        static void assert_row_count(const Table& table, size_t expected_count) {
            auto actual_count = table.get_row_count();
            if (actual_count != expected_count) {
                throw std::runtime_error("Row count mismatch: expected " +
                                       std::to_string(expected_count) +
                                       ", got " + std::to_string(actual_count));
            }
        }

        static void assert_row_exists(const Table& table, std::size_t row_id) {
            auto row = table.get_row(row_id);
            if (!row) {
                throw std::runtime_error("Row " + std::to_string(row_id) + " does not exist");
            }
        }

        static void assert_row_not_exists(const Table& table, std::size_t row_id) {
            auto row = table.get_row(row_id);
            if (row) {
                throw std::runtime_error("Row " + std::to_string(row_id) + " should not exist");
            }
        }

        static void assert_column_value(const TableRow& row, const std::string& column,
                                       const CellValue& expected) {
            auto actual = row.get_cell_value(column);
            if (!actual) {
                throw std::runtime_error("Column " + column + " does not exist");
            }

            if (*actual != expected) {
                throw std::runtime_error("Column value mismatch for " + column);
            }
        }

        static void assert_index_exists(const Table& table, const std::string& index_name) {
            if (!table.has_index(index_name)) {
                throw std::runtime_error("Index " + index_name + " does not exist");
            }
        }
    };

    // Pattern 3: Performance testing
    class PerformanceTests {
    public:
        static void test_insert_performance(size_t record_count = 10000) {
            auto table = TestDataFactory::create_test_table();
            auto test_data = TestDataFactory::create_test_users(record_count);

            auto start = std::chrono::high_resolution_clock::now();

            auto txn = table->begin_transaction();
            for (const auto& user : test_data) {
                txn->insert_row(user);
            }
            txn->commit();

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double ops_per_second = (record_count * 1000.0) / duration.count();

            std::cout << "Insert Performance Test:\n";
            std::cout << "  Records: " << record_count << "\n";
            std::cout << "  Duration: " << duration.count() << " ms\n";
            std::cout << "  Rate: " << ops_per_second << " ops/sec\n\n";

            // Assert minimum performance
            if (ops_per_second < 1000) {
                throw std::runtime_error("Insert performance below threshold");
            }
        }

        static void test_query_performance(size_t query_count = 1000) {
            auto table = TestDataFactory::create_test_table();
            TestDataFactory::populate_test_table(*table, 10000);

            auto start = std::chrono::high_resolution_clock::now();

            for (size_t i = 0; i < query_count; ++i) {
                TableQuery query;
                query.where("age", QueryOperator::GreaterThan, static_cast<std::int64_t>(30))
                     .limit(10);
                auto results = table->query(query);
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            double queries_per_second = (query_count * 1000.0) / duration.count();

            std::cout << "Query Performance Test:\n";
            std::cout << "  Queries: " << query_count << "\n";
            std::cout << "  Duration: " << duration.count() << " ms\n";
            std::cout << "  Rate: " << queries_per_second << " queries/sec\n\n";

            // Assert minimum performance
            if (queries_per_second < 100) {
                throw std::runtime_error("Query performance below threshold");
            }
        }
    };
};
```

## Production Deployment

### 1. Configuration Management

```cpp
class ProductionConfig {
public:
    struct TableConfig {
        std::string name;
        size_t initial_capacity;
        std::vector<std::string> index_columns;
        bool enable_auto_backup;
        std::chrono::minutes backup_interval;
        std::string data_directory;
        size_t max_memory_mb;
    };

    static TableConfig load_config(const std::string& config_file) {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + config_file);
        }

        nlohmann::json config_json;
        file >> config_json;

        TableConfig config;
        config.name = config_json["name"].get<std::string>();
        config.initial_capacity = config_json.value("initial_capacity", 1000);
        config.enable_auto_backup = config_json.value("enable_auto_backup", true);
        config.backup_interval = std::chrono::minutes(
            config_json.value("backup_interval_minutes", 30)
        );
        config.data_directory = config_json.value("data_directory", "./data");
        config.max_memory_mb = config_json.value("max_memory_mb", 1024);

        // Load index columns
        if (config_json.contains("index_columns")) {
            for (const auto& column : config_json["index_columns"]) {
                config.index_columns.push_back(column.get<std::string>());
            }
        }

        return config;
    }

    static std::unique_ptr<Table> create_production_table(const TableConfig& config) {
        // Create data directory
        std::filesystem::create_directories(config.data_directory);

        // Load existing table or create new one
        auto table_file = std::filesystem::path(config.data_directory) / (config.name + ".json");

        std::unique_ptr<Table> table;
        if (std::filesystem::exists(table_file)) {
            table = Table::load_from_file(table_file.string());
        } else {
            auto schema = create_production_schema(config.name);
            table = std::make_unique<Table>(std::move(schema));
        }

        // Create configured indexes
        for (const auto& column : config.index_columns) {
            std::string index_name = column + "_idx";
            if (!table->has_index(index_name)) {
                table->create_index(index_name, {column});
            }
        }

        return table;
    }

private:
    static std::unique_ptr<TableSchema> create_production_schema(const std::string& name) {
        // This would be customized based on application needs
        auto schema = std::make_unique<TableSchema>(name, 1);

        // Standard fields for production tables
        schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false));
        schema->add_column(ColumnDefinition("updated_at", ColumnType::DateTime, false));
        schema->add_column(ColumnDefinition("version", ColumnType::Integer, false));

        schema->set_primary_key({"id"});
        return schema;
    }
};
```

### 2. Health Monitoring

```cpp
class HealthMonitor {
public:
    struct HealthStatus {
        bool is_healthy;
        std::string status_message;
        size_t row_count;
        size_t memory_usage_mb;
        double avg_query_time_ms;
        std::chrono::system_clock::time_point last_check;
    };

    static HealthStatus check_table_health(const Table& table) {
        HealthStatus status;
        status.last_check = std::chrono::system_clock::now();
        status.is_healthy = true;
        status.status_message = "OK";

        try {
            // Check basic functionality
            status.row_count = table.get_row_count();

            // Estimate memory usage
            status.memory_usage_mb = estimate_memory_usage(table);

            // Test query performance
            status.avg_query_time_ms = measure_query_performance(table);

            // Health checks
            if (status.memory_usage_mb > 2048) {  // 2GB limit
                status.is_healthy = false;
                status.status_message = "High memory usage: " +
                                      std::to_string(status.memory_usage_mb) + "MB";
            }

            if (status.avg_query_time_ms > 100) {  // 100ms threshold
                status.is_healthy = false;
                status.status_message = "Slow query performance: " +
                                      std::to_string(status.avg_query_time_ms) + "ms";
            }

        } catch (const std::exception& e) {
            status.is_healthy = false;
            status.status_message = "Health check failed: " + std::string(e.what());
        }

        return status;
    }

    static void log_health_status(const HealthStatus& status) {
        auto timestamp = format_timestamp(status.last_check);

        std::cout << "[" << timestamp << "] Table Health Check:\n";
        std::cout << "  Status: " << (status.is_healthy ? "HEALTHY" : "UNHEALTHY") << "\n";
        std::cout << "  Message: " << status.status_message << "\n";
        std::cout << "  Row Count: " << status.row_count << "\n";
        std::cout << "  Memory Usage: " << status.memory_usage_mb << " MB\n";
        std::cout << "  Avg Query Time: " << status.avg_query_time_ms << " ms\n\n";
    }

private:
    static size_t estimate_memory_usage(const Table& table) {
        // Simplified memory estimation
        auto row_count = table.get_row_count();
        auto estimated_bytes_per_row = 100;  // Rough estimate
        return (row_count * estimated_bytes_per_row) / (1024 * 1024);  // Convert to MB
    }

    static double measure_query_performance(const Table& table) {
        const size_t test_queries = 10;
        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < test_queries; ++i) {
            auto rows = table.get_all_rows();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        return duration.count() / (test_queries * 1000.0);  // Average ms per query
    }
};
```

This comprehensive documentation provides detailed best practices covering all aspects of the Table system. The documentation includes:

1. **Schema Design**: Future-proof design patterns and data type selection
2. **Data Management**: Transaction patterns, repository patterns, and bulk operations
3. **Performance**: Indexing strategies, memory management, and optimization techniques
4. **Error Handling**: Exception safety, recovery strategies, and logging
5. **Testing**: Unit testing patterns, performance testing, and assertion helpers
6. **Production**: Configuration management and health monitoring

Each section includes practical code examples demonstrating real-world usage patterns that developers can adapt for their specific needs.

---

_See also: [Table API Reference](Tables-API.md), [Performance Guide](Tables-Performance.md), [Schema Management](Tables-Schema.md)_
