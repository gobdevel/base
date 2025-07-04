# Table System Examples and Tutorials

## Overview

This guide provides comprehensive examples and step-by-step tutorials for using the Table system, from basic operations to advanced real-world applications. Each example includes complete, runnable code with detailed explanations.

## Table of Contents

1. [Basic CRUD Operations](#basic-crud-operations)
2. [Indexing and Querying](#indexing-and-querying)
3. [Transactions and Concurrency](#transactions-and-concurrency)
4. [Schema Evolution](#schema-evolution)
5. [Serialization and Persistence](#serialization-and-persistence)
6. [Real-World Applications](#real-world-applications)
7. [Integration Patterns](#integration-patterns)
8. [Performance Optimization](#performance-optimization)

## Basic CRUD Operations

### Tutorial 1: Creating Your First Table

```cpp
#include "tables.h"
#include "logger.h"
#include <iostream>
#include <memory>

using namespace base;

int main() {
    // Initialize logging (optional but recommended)
    base::Logger::init();

    // Step 1: Create a schema for a simple user table
    auto schema = std::make_unique<TableSchema>("users", 1);

    // Add columns with appropriate types
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));        // Primary key
    schema->add_column(ColumnDefinition("username", ColumnType::String, false));   // Required
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));      // Required
    schema->add_column(ColumnDefinition("age", ColumnType::Integer, true));        // Optional
    schema->add_column(ColumnDefinition("is_active", ColumnType::Boolean, true));  // Optional
    schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, false)); // Required

    // Set the primary key
    schema->set_primary_key({"id"});

    // Step 2: Create the table
    auto table = std::make_unique<Table>(std::move(schema));

    std::cout << "Created table with " << table->get_schema().get_columns().size()
              << " columns" << std::endl;

    // Step 3: Insert some users
    std::vector<std::unordered_map<std::string, CellValue>> users = {
        {
            {"id", static_cast<std::int64_t>(1)},
            {"username", std::string("alice")},
            {"email", std::string("alice@example.com")},
            {"age", static_cast<std::int64_t>(25)},
            {"is_active", true},
            {"created_at", std::string("2025-01-15T10:00:00Z")}
        },
        {
            {"id", static_cast<std::int64_t>(2)},
            {"username", std::string("bob")},
            {"email", std::string("bob@example.com")},
            {"age", static_cast<std::int64_t>(30)},
            {"is_active", true},
            {"created_at", std::string("2025-01-15T11:00:00Z")}
        },
        {
            {"id", static_cast<std::int64_t>(3)},
            {"username", std::string("charlie")},
            {"email", std::string("charlie@example.com")},
            // age is optional, so we can omit it
            {"is_active", false},
            {"created_at", std::string("2025-01-15T12:00:00Z")}
        }
    };

    // Insert users one by one
    for (const auto& user : users) {
        try {
            auto row_id = table->insert_row(user);
            std::cout << "Inserted user with row ID: " << row_id << std::endl;
        } catch (const TableException& e) {
            std::cerr << "Failed to insert user: " << e.what() << std::endl;
        }
    }

    // Step 4: Read data back
    std::cout << "\nAll users in table:" << std::endl;
    auto all_rows = table->get_all_rows();
    for (const auto& row : all_rows) {
        auto username = row.get_cell_value("username");
        auto email = row.get_cell_value("email");
        auto age = row.get_cell_value("age");
        auto is_active = row.get_cell_value("is_active");

        std::cout << "  User: " << std::get<std::string>(*username)
                  << " (" << std::get<std::string>(*email) << ")";

        if (age) {
            std::cout << ", Age: " << std::get<std::int64_t>(*age);
        }

        if (is_active) {
            std::cout << ", Active: " << (std::get<bool>(*is_active) ? "Yes" : "No");
        }

        std::cout << std::endl;
    }

    std::cout << "\nTotal users: " << table->get_row_count() << std::endl;

    return 0;
}
```

### Tutorial 2: Update and Delete Operations

```cpp
#include "tables.h"
#include <iostream>

using namespace base;

void demonstrate_update_delete() {
    // Create and populate a simple table (reusing code from Tutorial 1)
    auto schema = std::make_unique<TableSchema>("products", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("price", ColumnType::Double, false));
    schema->add_column(ColumnDefinition("stock", ColumnType::Integer, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Insert initial products
    std::vector<std::unordered_map<std::string, CellValue>> products = {
        {{"id", static_cast<std::int64_t>(1)}, {"name", std::string("Laptop")},
         {"price", 999.99}, {"stock", static_cast<std::int64_t>(10)}},
        {{"id", static_cast<std::int64_t>(2)}, {"name", std::string("Mouse")},
         {"price", 29.99}, {"stock", static_cast<std::int64_t>(50)}},
        {{"id", static_cast<std::int64_t>(3)}, {"name", std::string("Keyboard")},
         {"price", 79.99}, {"stock", static_cast<std::int64_t>(25)}}
    };

    for (const auto& product : products) {
        table->insert_row(product);
    }

    std::cout << "Initial products:" << std::endl;
    print_products(*table);

    // Update operations
    std::cout << "\n=== Update Operations ===" << std::endl;

    // Update price of laptop (row_id 1)
    std::unordered_map<std::string, CellValue> price_update = {
        {"price", 899.99}  // Reduce laptop price
    };

    if (table->update_row(1, price_update)) {
        std::cout << "Updated laptop price successfully" << std::endl;
    }

    // Update stock for mouse (row_id 2)
    std::unordered_map<std::string, CellValue> stock_update = {
        {"stock", static_cast<std::int64_t>(45)}  // Sold 5 mice
    };

    if (table->update_row(2, stock_update)) {
        std::cout << "Updated mouse stock successfully" << std::endl;
    }

    // Update multiple fields for keyboard (row_id 3)
    std::unordered_map<std::string, CellValue> multi_update = {
        {"name", std::string("Mechanical Keyboard")},  // Better name
        {"price", 99.99},                              // Increase price
        {"stock", static_cast<std::int64_t>(20)}       // Sold 5 keyboards
    };

    if (table->update_row(3, multi_update)) {
        std::cout << "Updated keyboard details successfully" << std::endl;
    }

    std::cout << "\nProducts after updates:" << std::endl;
    print_products(*table);

    // Delete operations
    std::cout << "\n=== Delete Operations ===" << std::endl;

    // Delete the mouse (row_id 2)
    if (table->delete_row(2)) {
        std::cout << "Deleted mouse successfully" << std::endl;
    }

    std::cout << "\nProducts after deletion:" << std::endl;
    print_products(*table);

    // Try to access deleted row
    auto deleted_row = table->get_row(2);
    if (!deleted_row) {
        std::cout << "Confirmed: Row 2 (mouse) no longer exists" << std::endl;
    }

    // Try to update deleted row (should fail)
    if (!table->update_row(2, {{"price", 19.99}})) {
        std::cout << "Cannot update deleted row (expected behavior)" << std::endl;
    }
}

void print_products(const Table& table) {
    auto rows = table.get_all_rows();
    for (const auto& row : rows) {
        auto id = std::get<std::int64_t>(*row.get_cell_value("id"));
        auto name = std::get<std::string>(*row.get_cell_value("name"));
        auto price = std::get<double>(*row.get_cell_value("price"));
        auto stock = std::get<std::int64_t>(*row.get_cell_value("stock"));

        std::cout << "  ID: " << id << ", Name: " << name
                  << ", Price: $" << price << ", Stock: " << stock << std::endl;
    }
}
```

## Indexing and Querying

### Tutorial 3: Creating and Using Indexes

```cpp
#include "tables.h"
#include <iostream>
#include <chrono>

using namespace base;

void demonstrate_indexing() {
    // Create a larger table for meaningful indexing demonstration
    auto schema = std::make_unique<TableSchema>("employees", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("employee_id", ColumnType::String, false));
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("department", ColumnType::String, false));
    schema->add_column(ColumnDefinition("salary", ColumnType::Double, false));
    schema->add_column(ColumnDefinition("hire_date", ColumnType::DateTime, false));
    schema->add_column(ColumnDefinition("is_manager", ColumnType::Boolean, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Populate with sample employees
    std::vector<std::string> departments = {"Engineering", "Sales", "Marketing", "HR", "Finance"};
    std::vector<std::string> first_names = {"John", "Jane", "Michael", "Sarah", "David", "Lisa", "Robert", "Emily"};
    std::vector<std::string> last_names = {"Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis"};

    std::cout << "Populating employee table..." << std::endl;
    for (int i = 1; i <= 1000; ++i) {
        std::unordered_map<std::string, CellValue> employee = {
            {"id", static_cast<std::int64_t>(i)},
            {"employee_id", std::string("EMP") + std::to_string(1000 + i)},
            {"first_name", first_names[i % first_names.size()]},
            {"last_name", last_names[i % last_names.size()]},
            {"department", departments[i % departments.size()]},
            {"salary", 50000.0 + (i % 100) * 1000.0},  // Salary range: 50k-149k
            {"hire_date", std::string("202") + std::to_string(i % 5) + "-01-01T00:00:00Z"},
            {"is_manager", (i % 10 == 0)}  // Every 10th employee is a manager
        };

        table->insert_row(employee);
    }

    std::cout << "Populated table with " << table->get_row_count() << " employees" << std::endl;

    // Demonstrate queries without indexes (slow)
    std::cout << "\n=== Queries WITHOUT Indexes ===" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // Find employees in Engineering department
    TableQuery eng_query;
    eng_query.where("department", QueryOperator::Equal, std::string("Engineering"));
    auto eng_employees = table->query(eng_query);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Found " << eng_employees.size() << " engineering employees in "
              << duration << " microseconds (full table scan)" << std::endl;

    // Now create indexes for common query patterns
    std::cout << "\n=== Creating Indexes ===" << std::endl;

    // Single column indexes
    table->create_index("employee_id_idx", {"employee_id"});
    table->create_index("department_idx", {"department"});
    table->create_index("salary_idx", {"salary"});
    table->create_index("hire_date_idx", {"hire_date"});

    // Composite indexes for multi-column queries
    table->create_index("dept_salary_idx", {"department", "salary"});
    table->create_index("manager_dept_idx", {"is_manager", "department"});

    std::cout << "Created 6 indexes" << std::endl;

    // Demonstrate queries with indexes (fast)
    std::cout << "\n=== Queries WITH Indexes ===" << std::endl;

    // 1. Direct index lookup by employee_id
    start = std::chrono::high_resolution_clock::now();
    auto specific_employee = table->find_by_index("employee_id_idx", {std::string("EMP1500")});
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Found employee EMP1500 in " << duration
              << " microseconds (index lookup)" << std::endl;

    // 2. Department-based query (using index)
    start = std::chrono::high_resolution_clock::now();
    auto sales_employees = table->find_by_index("department_idx", {std::string("Sales")});
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Found " << sales_employees.size() << " sales employees in "
              << duration << " microseconds (index lookup)" << std::endl;

    // 3. Composite index query (department + salary range)
    start = std::chrono::high_resolution_clock::now();

    TableQuery high_paid_eng;
    high_paid_eng.where("department", QueryOperator::Equal, std::string("Engineering"))
                 .where("salary", QueryOperator::GreaterThan, 100000.0);
    auto high_paid_engineers = table->query(high_paid_eng);

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Found " << high_paid_engineers.size()
              << " high-paid engineers in " << duration
              << " microseconds (composite index)" << std::endl;

    // 4. Range query with ordering
    start = std::chrono::high_resolution_clock::now();

    TableQuery salary_range;
    salary_range.where("salary", QueryOperator::GreaterThanOrEqual, 80000.0)
                .where("salary", QueryOperator::LessThanOrEqual, 120000.0)
                .order_by("salary", SortOrder::Descending)
                .limit(10);
    auto mid_range_salaries = table->query(salary_range);

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Found top 10 employees in salary range 80k-120k in "
              << duration << " microseconds" << std::endl;

    // Display some results
    std::cout << "\nSample high-paid engineers:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), high_paid_engineers.size()); ++i) {
        const auto& emp = high_paid_engineers[i];
        auto first_name = std::get<std::string>(*emp.get_cell_value("first_name"));
        auto last_name = std::get<std::string>(*emp.get_cell_value("last_name"));
        auto salary = std::get<double>(*emp.get_cell_value("salary"));

        std::cout << "  " << first_name << " " << last_name
                  << " - $" << salary << std::endl;
    }
}
```

### Tutorial 4: Advanced Query Patterns

```cpp
#include "tables.h"
#include <iostream>
#include <algorithm>

using namespace base;

void demonstrate_advanced_queries() {
    // Use the employee table from Tutorial 3
    auto table = create_sample_employee_table();

    std::cout << "=== Advanced Query Patterns ===" << std::endl;

    // Pattern 1: Complex filtering with multiple conditions
    std::cout << "\n1. Complex Filtering:" << std::endl;

    TableQuery complex_filter;
    complex_filter.where("department", QueryOperator::Equal, std::string("Engineering"))
                  .where("salary", QueryOperator::GreaterThan, 75000.0)
                  .where("is_manager", QueryOperator::Equal, false)
                  .order_by("salary", SortOrder::Descending);

    auto senior_engineers = table->query(complex_filter);
    std::cout << "Found " << senior_engineers.size()
              << " senior engineers (non-managers earning >$75k)" << std::endl;

    // Pattern 2: Date range queries
    std::cout << "\n2. Date Range Queries:" << std::endl;

    TableQuery recent_hires;
    recent_hires.where("hire_date", QueryOperator::GreaterThanOrEqual, std::string("2023-01-01T00:00:00Z"))
                .where("hire_date", QueryOperator::LessThan, std::string("2024-01-01T00:00:00Z"))
                .order_by("hire_date", SortOrder::Ascending);

    auto hires_2023 = table->query(recent_hires);
    std::cout << "Found " << hires_2023.size() << " employees hired in 2023" << std::endl;

    // Pattern 3: Top-N queries with different criteria
    std::cout << "\n3. Top-N Queries:" << std::endl;

    // Highest paid employees
    TableQuery top_earners;
    top_earners.order_by("salary", SortOrder::Descending).limit(5);
    auto highest_paid = table->query(top_earners);

    std::cout << "Top 5 highest paid employees:" << std::endl;
    for (const auto& emp : highest_paid) {
        auto name = std::get<std::string>(*emp.get_cell_value("first_name")) + " " +
                   std::get<std::string>(*emp.get_cell_value("last_name"));
        auto salary = std::get<double>(*emp.get_cell_value("salary"));
        auto dept = std::get<std::string>(*emp.get_cell_value("department"));

        std::cout << "  " << name << " (" << dept << ") - $" << salary << std::endl;
    }

    // Pattern 4: Aggregation-style queries (manual aggregation)
    std::cout << "\n4. Department Statistics:" << std::endl;

    std::vector<std::string> departments = {"Engineering", "Sales", "Marketing", "HR", "Finance"};

    for (const auto& dept : departments) {
        TableQuery dept_query;
        dept_query.where("department", QueryOperator::Equal, dept);
        auto dept_employees = table->query(dept_query);

        if (!dept_employees.empty()) {
            // Calculate statistics
            double total_salary = 0;
            double min_salary = std::numeric_limits<double>::max();
            double max_salary = 0;
            size_t manager_count = 0;

            for (const auto& emp : dept_employees) {
                auto salary = std::get<double>(*emp.get_cell_value("salary"));
                auto is_manager = std::get<bool>(*emp.get_cell_value("is_manager"));

                total_salary += salary;
                min_salary = std::min(min_salary, salary);
                max_salary = std::max(max_salary, salary);

                if (is_manager) manager_count++;
            }

            double avg_salary = total_salary / dept_employees.size();

            std::cout << "  " << dept << ":" << std::endl;
            std::cout << "    Employees: " << dept_employees.size() << std::endl;
            std::cout << "    Managers: " << manager_count << std::endl;
            std::cout << "    Avg Salary: $" << std::fixed << std::setprecision(0) << avg_salary << std::endl;
            std::cout << "    Salary Range: $" << min_salary << " - $" << max_salary << std::endl;
        }
    }

    // Pattern 5: Search-like queries with LIKE operator
    std::cout << "\n5. Search Patterns:" << std::endl;

    // Find employees with names starting with "J"
    TableQuery name_search;
    name_search.where("first_name", QueryOperator::Like, std::string("J%"));
    auto j_names = table->query(name_search);

    std::cout << "Found " << j_names.size() << " employees with first names starting with 'J'" << std::endl;

    // Find employees with specific employee ID patterns
    TableQuery id_pattern;
    id_pattern.where("employee_id", QueryOperator::Like, std::string("EMP15%"));
    auto id_matches = table->query(id_pattern);

    std::cout << "Found " << id_matches.size() << " employees with IDs starting with 'EMP15'" << std::endl;
}

std::unique_ptr<Table> create_sample_employee_table() {
    auto schema = std::make_unique<TableSchema>("employees", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("employee_id", ColumnType::String, false));
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("department", ColumnType::String, false));
    schema->add_column(ColumnDefinition("salary", ColumnType::Double, false));
    schema->add_column(ColumnDefinition("hire_date", ColumnType::DateTime, false));
    schema->add_column(ColumnDefinition("is_manager", ColumnType::Boolean, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Create indexes
    table->create_index("employee_id_idx", {"employee_id"});
    table->create_index("department_idx", {"department"});
    table->create_index("salary_idx", {"salary"});
    table->create_index("hire_date_idx", {"hire_date"});
    table->create_index("dept_salary_idx", {"department", "salary"});

    // Populate with realistic data (simplified for example)
    std::vector<std::string> departments = {"Engineering", "Sales", "Marketing", "HR", "Finance"};
    std::vector<std::string> first_names = {"John", "Jane", "Michael", "Sarah", "David", "Lisa", "Robert", "Emily"};
    std::vector<std::string> last_names = {"Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis"};

    for (int i = 1; i <= 200; ++i) {
        std::unordered_map<std::string, CellValue> employee = {
            {"id", static_cast<std::int64_t>(i)},
            {"employee_id", std::string("EMP") + std::to_string(1500 + i)},
            {"first_name", first_names[i % first_names.size()]},
            {"last_name", last_names[i % last_names.size()]},
            {"department", departments[i % departments.size()]},
            {"salary", 50000.0 + (i % 100) * 1000.0},
            {"hire_date", std::string("202") + std::to_string(2 + (i % 3)) + "-" +
                         std::to_string(1 + (i % 12)) + "-01T00:00:00Z"},
            {"is_manager", (i % 15 == 0)}
        };

        table->insert_row(employee);
    }

    return table;
}
```

## Transactions and Concurrency

### Tutorial 5: Transaction Management

```cpp
#include "tables.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

using namespace base;

// RAII Transaction wrapper for automatic cleanup
class AutoTransaction {
private:
    std::unique_ptr<TableTransaction> txn_;
    bool committed_;

public:
    explicit AutoTransaction(Table* table)
        : txn_(table->begin_transaction()), committed_(false) {}

    ~AutoTransaction() {
        if (!committed_ && txn_ && txn_->get_state() == TransactionState::Active) {
            txn_->rollback();
            std::cout << "Transaction auto-rolled back" << std::endl;
        }
    }

    TableTransaction* operator->() { return txn_.get(); }
    TableTransaction& operator*() { return *txn_; }

    void commit() {
        if (txn_ && txn_->get_state() == TransactionState::Active) {
            txn_->commit();
            committed_ = true;
        }
    }

    void rollback() {
        if (txn_ && txn_->get_state() == TransactionState::Active) {
            txn_->rollback();
            committed_ = true;  // Prevent auto-rollback
        }
    }
};

void demonstrate_transactions() {
    // Create a banking account table
    auto schema = std::make_unique<TableSchema>("accounts", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("account_number", ColumnType::String, false));
    schema->add_column(ColumnDefinition("owner_name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("balance", ColumnType::Double, false));
    schema->add_column(ColumnDefinition("account_type", ColumnType::String, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));
    table->create_index("account_number_idx", {"account_number"});

    // Create initial accounts
    std::vector<std::unordered_map<std::string, CellValue>> accounts = {
        {{"id", static_cast<std::int64_t>(1)}, {"account_number", std::string("ACC001")},
         {"owner_name", std::string("Alice Smith")}, {"balance", 1000.0}, {"account_type", std::string("Checking")}},
        {{"id", static_cast<std::int64_t>(2)}, {"account_number", std::string("ACC002")},
         {"owner_name", std::string("Bob Johnson")}, {"balance", 500.0}, {"account_type", std::string("Savings")}},
        {{"id", static_cast<std::int64_t>(3)}, {"account_number", std::string("ACC003")},
         {"owner_name", std::string("Charlie Brown")}, {"balance", 2000.0}, {"account_type", std::string("Checking")}}
    };

    for (const auto& account : accounts) {
        table->insert_row(account);
    }

    std::cout << "=== Transaction Examples ===" << std::endl;
    print_account_balances(*table);

    // Example 1: Simple transaction with commit
    std::cout << "\n1. Simple Transaction (Deposit to Alice's account):" << std::endl;
    {
        AutoTransaction txn(table.get());

        // Get Alice's current balance
        auto alice_row = txn->get_row(1);
        if (alice_row) {
            auto current_balance = std::get<double>(*alice_row->get_cell_value("balance"));
            double deposit_amount = 250.0;

            // Update balance
            txn->update_row(1, {{"balance", current_balance + deposit_amount}});

            std::cout << "Deposited $" << deposit_amount << " to Alice's account" << std::endl;
            txn.commit();
            std::cout << "Transaction committed successfully" << std::endl;
        }
    }
    print_account_balances(*table);

    // Example 2: Transaction with rollback (insufficient funds)
    std::cout << "\n2. Transaction with Rollback (Insufficient funds):" << std::endl;
    {
        AutoTransaction txn(table.get());

        try {
            // Try to withdraw more than Bob has
            auto bob_row = txn->get_row(2);
            if (bob_row) {
                auto current_balance = std::get<double>(*bob_row->get_cell_value("balance"));
                double withdrawal_amount = 1000.0;  // More than Bob's $500 balance

                if (current_balance < withdrawal_amount) {
                    throw std::runtime_error("Insufficient funds");
                }

                txn->update_row(2, {{"balance", current_balance - withdrawal_amount}});
                txn.commit();
            }

        } catch (const std::exception& e) {
            std::cout << "Transaction failed: " << e.what() << std::endl;
            txn.rollback();
            std::cout << "Transaction rolled back" << std::endl;
        }
    }
    print_account_balances(*table);

    // Example 3: Complex transaction (Transfer between accounts)
    std::cout << "\n3. Complex Transaction (Transfer $300 from Charlie to Bob):" << std::endl;
    {
        AutoTransaction txn(table.get());

        try {
            double transfer_amount = 300.0;

            // Get current balances
            auto charlie_row = txn->get_row(3);
            auto bob_row = txn->get_row(2);

            if (!charlie_row || !bob_row) {
                throw std::runtime_error("Account not found");
            }

            auto charlie_balance = std::get<double>(*charlie_row->get_cell_value("balance"));
            auto bob_balance = std::get<double>(*bob_row->get_cell_value("balance"));

            // Check sufficient funds
            if (charlie_balance < transfer_amount) {
                throw std::runtime_error("Insufficient funds for transfer");
            }

            // Perform transfer (both operations or neither)
            txn->update_row(3, {{"balance", charlie_balance - transfer_amount}});  // Debit Charlie
            txn->update_row(2, {{"balance", bob_balance + transfer_amount}});      // Credit Bob

            std::cout << "Transferred $" << transfer_amount << " from Charlie to Bob" << std::endl;
            txn.commit();
            std::cout << "Transfer completed successfully" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "Transfer failed: " << e.what() << std::endl;
            txn.rollback();
            std::cout << "Transfer rolled back" << std::endl;
        }
    }
    print_account_balances(*table);

    // Example 4: Concurrent transactions
    std::cout << "\n4. Concurrent Transactions:" << std::endl;
    demonstrate_concurrent_transactions(*table);
}

void print_account_balances(const Table& table) {
    std::cout << "\nCurrent account balances:" << std::endl;
    auto rows = table.get_all_rows();
    for (const auto& row : rows) {
        auto account_number = std::get<std::string>(*row.get_cell_value("account_number"));
        auto owner_name = std::get<std::string>(*row.get_cell_value("owner_name"));
        auto balance = std::get<double>(*row.get_cell_value("balance"));

        std::cout << "  " << account_number << " (" << owner_name << "): $"
                  << std::fixed << std::setprecision(2) << balance << std::endl;
    }
}

void demonstrate_concurrent_transactions(Table& table) {
    std::atomic<int> successful_transactions{0};
    std::atomic<int> failed_transactions{0};

    std::vector<std::thread> workers;

    // Create multiple concurrent transactions
    for (int i = 0; i < 5; ++i) {
        workers.emplace_back([&, i]() {
            try {
                AutoTransaction txn(&table);

                // Each thread tries to deposit to a different account
                std::size_t account_id = (i % 3) + 1;
                double deposit = 50.0 + (i * 10.0);

                auto account_row = txn->get_row(account_id);
                if (account_row) {
                    auto current_balance = std::get<double>(*account_row->get_cell_value("balance"));

                    // Simulate some processing time
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                    txn->update_row(account_id, {{"balance", current_balance + deposit}});
                    txn.commit();

                    successful_transactions++;
                    std::cout << "Thread " << i << " deposited $" << deposit
                             << " to account " << account_id << std::endl;
                } else {
                    failed_transactions++;
                }

            } catch (const std::exception& e) {
                failed_transactions++;
                std::cout << "Thread " << i << " failed: " << e.what() << std::endl;
            }
        });
    }

    // Wait for all threads to complete
    for (auto& worker : workers) {
        worker.join();
    }

    std::cout << "Concurrent transaction results:" << std::endl;
    std::cout << "  Successful: " << successful_transactions << std::endl;
    std::cout << "  Failed: " << failed_transactions << std::endl;

    print_account_balances(table);
}
```

## Schema Evolution

### Tutorial 6: Schema Versioning and Migration

```cpp
#include "tables.h"
#include <iostream>

using namespace base;

void demonstrate_schema_evolution() {
    std::cout << "=== Schema Evolution Example ===" << std::endl;

    // Create initial schema (version 1)
    auto schema_v1 = create_user_schema_v1();
    auto table = std::make_unique<Table>(std::move(schema_v1));

    // Populate with initial data
    std::vector<std::unordered_map<std::string, CellValue>> initial_users = {
        {{"id", static_cast<std::int64_t>(1)}, {"username", std::string("alice")},
         {"email", std::string("alice@example.com")}},
        {{"id", static_cast<std::int64_t>(2)}, {"username", std::string("bob")},
         {"email", std::string("bob@example.com")}},
        {{"id", static_cast<std::int64_t>(3)}, {"username", std::string("charlie")},
         {"email", std::string("charlie@example.com")}}
    };

    for (const auto& user : initial_users) {
        table->insert_row(user);
    }

    std::cout << "Schema v1 - Initial table created with " << table->get_row_count() << " users" << std::endl;
    print_schema_info(table->get_schema());

    // Save current state
    table->save_to_file("users_v1.json");
    std::cout << "Saved v1 data to users_v1.json" << std::endl;

    // Evolve to version 2 (add new columns)
    std::cout << "\n--- Evolving to Schema v2 ---" << std::endl;
    auto schema_v2 = create_user_schema_v2();

    try {
        table->evolve_schema(std::move(schema_v2));
        std::cout << "Successfully evolved to schema v2" << std::endl;
        print_schema_info(table->get_schema());

        // Verify existing data is preserved
        std::cout << "Existing data after evolution:" << std::endl;
        print_user_data(*table);

        // Add new data with v2 fields
        std::unordered_map<std::string, CellValue> new_user = {
            {"id", static_cast<std::int64_t>(4)},
            {"username", std::string("diana")},
            {"email", std::string("diana@example.com")},
            {"first_name", std::string("Diana")},        // New in v2
            {"last_name", std::string("Prince")},        // New in v2
            {"created_at", std::string("2025-01-15T12:00:00Z")}  // New in v2
        };

        table->insert_row(new_user);
        std::cout << "Added new user with v2 fields" << std::endl;

        // Update existing user with new fields
        table->update_row(1, {
            {"first_name", std::string("Alice")},
            {"last_name", std::string("Wonderland")},
            {"created_at", std::string("2025-01-15T10:00:00Z")}
        });
        std::cout << "Updated existing user with v2 fields" << std::endl;

    } catch (const TableException& e) {
        std::cout << "Schema evolution failed: " << e.what() << std::endl;
        return;
    }

    // Evolve to version 3 (add more complex fields)
    std::cout << "\n--- Evolving to Schema v3 ---" << std::endl;
    auto schema_v3 = create_user_schema_v3();

    try {
        table->evolve_schema(std::move(schema_v3));
        std::cout << "Successfully evolved to schema v3" << std::endl;
        print_schema_info(table->get_schema());

        // Add user with v3 features
        std::unordered_map<std::string, CellValue> v3_user = {
            {"id", static_cast<std::int64_t>(5)},
            {"username", std::string("eve")},
            {"email", std::string("eve@example.com")},
            {"first_name", std::string("Eve")},
            {"last_name", std::string("Smith")},
            {"created_at", std::string("2025-01-15T13:00:00Z")},
            {"is_verified", true},                       // New in v3
            {"login_count", static_cast<std::int64_t>(5)},  // New in v3
            {"profile_data", std::string("{\"theme\": \"dark\", \"language\": \"en\"}")}  // New in v3
        };

        table->insert_row(v3_user);
        std::cout << "Added user with v3 fields" << std::endl;

        // Save evolved data
        table->save_to_file("users_v3.json");
        std::cout << "Saved v3 data to users_v3.json" << std::endl;

        std::cout << "\nFinal table data:" << std::endl;
        print_user_data(*table);

    } catch (const TableException& e) {
        std::cout << "Schema evolution to v3 failed: " << e.what() << std::endl;
    }

    // Demonstrate backward compatibility
    std::cout << "\n--- Testing Backward Compatibility ---" << std::endl;
    test_backward_compatibility();
}

std::unique_ptr<TableSchema> create_user_schema_v1() {
    auto schema = std::make_unique<TableSchema>("users", 1);

    // Basic user information
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("username", ColumnType::String, false));
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));

    schema->set_primary_key({"id"});
    return schema;
}

std::unique_ptr<TableSchema> create_user_schema_v2() {
    auto schema = std::make_unique<TableSchema>("users", 2);

    // Keep existing columns
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("username", ColumnType::String, false));
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));

    // Add new optional columns (for backward compatibility)
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, true));

    schema->set_primary_key({"id"});
    return schema;
}

std::unique_ptr<TableSchema> create_user_schema_v3() {
    auto schema = std::make_unique<TableSchema>("users", 3);

    // Keep all v2 columns
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("username", ColumnType::String, false));
    schema->add_column(ColumnDefinition("email", ColumnType::String, false));
    schema->add_column(ColumnDefinition("first_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("last_name", ColumnType::String, true));
    schema->add_column(ColumnDefinition("created_at", ColumnType::DateTime, true));

    // Add v3 features
    schema->add_column(ColumnDefinition("is_verified", ColumnType::Boolean, true));
    schema->add_column(ColumnDefinition("login_count", ColumnType::Integer, true));
    schema->add_column(ColumnDefinition("profile_data", ColumnType::String, true));  // JSON blob

    schema->set_primary_key({"id"});
    return schema;
}

void print_schema_info(const TableSchema& schema) {
    std::cout << "Schema: " << schema.get_name() << " (version " << schema.get_version() << ")" << std::endl;
    std::cout << "Columns:" << std::endl;

    for (const auto& column : schema.get_columns()) {
        std::string type_name;
        switch (column.get_type()) {
            case ColumnType::Integer: type_name = "Integer"; break;
            case ColumnType::String: type_name = "String"; break;
            case ColumnType::Double: type_name = "Double"; break;
            case ColumnType::Boolean: type_name = "Boolean"; break;
            case ColumnType::DateTime: type_name = "DateTime"; break;
            case ColumnType::Blob: type_name = "Blob"; break;
        }

        std::cout << "  " << column.get_name() << " (" << type_name
                  << (column.is_nullable() ? ", nullable" : ", required") << ")" << std::endl;
    }

    auto pk_columns = schema.get_primary_key();
    std::cout << "Primary Key: ";
    for (size_t i = 0; i < pk_columns.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << pk_columns[i];
    }
    std::cout << std::endl;
}

void print_user_data(const Table& table) {
    auto rows = table.get_all_rows();
    for (const auto& row : rows) {
        auto id = std::get<std::int64_t>(*row.get_cell_value("id"));
        auto username = std::get<std::string>(*row.get_cell_value("username"));

        std::cout << "  User " << id << " (" << username << ")";

        // Show v2+ fields if they exist
        auto first_name = row.get_cell_value("first_name");
        auto last_name = row.get_cell_value("last_name");
        if (first_name && last_name) {
            std::cout << " - " << std::get<std::string>(*first_name)
                     << " " << std::get<std::string>(*last_name);
        }

        // Show v3+ fields if they exist
        auto is_verified = row.get_cell_value("is_verified");
        if (is_verified) {
            std::cout << " [" << (std::get<bool>(*is_verified) ? "Verified" : "Not Verified") << "]";
        }

        std::cout << std::endl;
    }
}

void test_backward_compatibility() {
    try {
        // Load v1 data with current (v3) code
        auto v1_table = Table::load_from_file("users_v1.json");
        std::cout << "Successfully loaded v1 data with current code" << std::endl;
        std::cout << "v1 table has " << v1_table->get_row_count() << " rows" << std::endl;

        // Load v3 data
        auto v3_table = Table::load_from_file("users_v3.json");
        std::cout << "Successfully loaded v3 data" << std::endl;
        std::cout << "v3 table has " << v3_table->get_row_count() << " rows" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Backward compatibility test failed: " << e.what() << std::endl;
    }
}
```

This comprehensive documentation includes multiple detailed examples and tutorials covering:

1. **Basic CRUD Operations** - Step-by-step table creation, insertion, updates, and deletions
2. **Indexing and Querying** - Creating indexes and performing complex queries
3. **Transactions and Concurrency** - Transaction management with RAII patterns and concurrent operations
4. **Schema Evolution** - Version management and backward compatibility

Each tutorial includes complete, runnable code examples with detailed explanations. The examples progress from simple to complex, showing real-world usage patterns and best practices.

Would you like me to continue with the remaining sections (Serialization, Real-World Applications, Integration Patterns, and Performance Optimization) to complete this comprehensive documentation suite?

---

_See also: [Table API Reference](Tables-API.md), [Best Practices](Tables-BestPractices.md), [Performance Guide](Tables-Performance.md)_
