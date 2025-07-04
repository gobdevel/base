# Table Transactions and Concurrency

## Overview

The Table system provides ACID-like transaction support with thread-safe operations, rollback capabilities, and efficient concurrency control. This guide covers transaction management, concurrency patterns, and best practices for multi-threaded applications.

## Table of Contents

1. [Transaction Basics](#transaction-basics)
2. [Transaction Lifecycle](#transaction-lifecycle)
3. [ACID Properties](#acid-properties)
4. [Concurrency Control](#concurrency-control)
5. [Thread Safety](#thread-safety)
6. [Performance Considerations](#performance-considerations)
7. [Best Practices](#best-practices)
8. [Examples](#examples)

## Transaction Basics

### Creating Transactions

```cpp
#include "tables.h"
using namespace base;

// Begin a new transaction
auto transaction = table->begin_transaction();

// Perform operations within transaction
try {
    transaction->insert_row({
        {"id", static_cast<std::int64_t>(1)},
        {"name", std::string("John Doe")},
        {"email", std::string("john@example.com")}
    });

    transaction->update_row(2, {
        {"email", std::string("updated@example.com")}
    });

    // Commit all changes
    transaction->commit();

} catch (const std::exception& e) {
    // Rollback on error
    transaction->rollback();
    throw;
}
```

### Transaction States

```cpp
enum class TransactionState {
    Active,     // Transaction is active and accepting operations
    Committed,  // Transaction has been committed successfully
    RolledBack  // Transaction has been rolled back
};

// Check transaction state
auto state = transaction->get_state();
if (state == TransactionState::Active) {
    // Can continue adding operations
}
```

## Transaction Lifecycle

### 1. Begin Transaction

```cpp
// Create new transaction
auto txn = table->begin_transaction();

// Transaction is now in Active state
assert(txn->get_state() == TransactionState::Active);
```

### 2. Perform Operations

```cpp
// Insert operations
auto row_id1 = txn->insert_row({
    {"name", std::string("Alice")},
    {"email", std::string("alice@example.com")}
});

auto row_id2 = txn->insert_row({
    {"name", std::string("Bob")},
    {"email", std::string("bob@example.com")}
});

// Update operations
txn->update_row(row_id1, {
    {"email", std::string("alice.updated@example.com")}
});

// Delete operations
txn->delete_row(row_id2);
```

### 3. Commit or Rollback

```cpp
// Option 1: Commit (make changes permanent)
try {
    txn->commit();
    std::cout << "Transaction committed successfully" << std::endl;
} catch (const TableException& e) {
    std::cout << "Commit failed: " << e.what() << std::endl;
}

// Option 2: Rollback (discard all changes)
txn->rollback();
std::cout << "Transaction rolled back" << std::endl;
```

### Auto-Rollback with RAII

```cpp
class AutoTransaction {
private:
    std::unique_ptr<TableTransaction> txn_;
    bool committed_;

public:
    explicit AutoTransaction(Table* table)
        : txn_(table->begin_transaction()), committed_(false) {}

    ~AutoTransaction() {
        if (!committed_ && txn_->get_state() == TransactionState::Active) {
            txn_->rollback();
        }
    }

    TableTransaction* operator->() { return txn_.get(); }
    TableTransaction& operator*() { return *txn_; }

    void commit() {
        txn_->commit();
        committed_ = true;
    }
};

// Usage with auto-rollback
{
    AutoTransaction txn(table.get());

    txn->insert_row({{"name", std::string("Test")}});
    txn->update_row(1, {{"status", std::string("updated")}});

    txn.commit();  // Explicit commit

    // Auto-rollback happens in destructor if not committed
}
```

## ACID Properties

### Atomicity

All operations in a transaction are atomic - either all succeed or all fail:

```cpp
auto txn = table->begin_transaction();

try {
    // Multiple related operations
    auto user_id = txn->insert_row({
        {"name", std::string("John Doe")},
        {"email", std::string("john@example.com")}
    });

    txn->insert_row({
        {"user_id", user_id},
        {"action", std::string("user_created")},
        {"timestamp", std::string("2025-01-15T10:30:00Z")}
    });

    // Either both inserts succeed, or both are rolled back
    txn->commit();

} catch (const std::exception& e) {
    // Atomic rollback - no partial state
    txn->rollback();
}
```

### Consistency

Transactions maintain data consistency by validating constraints:

```cpp
auto txn = table->begin_transaction();

try {
    // This will throw if it violates primary key uniqueness
    txn->insert_row({
        {"id", static_cast<std::int64_t>(1)},  // Duplicate primary key
        {"name", std::string("Duplicate")}
    });

    txn->commit();  // Never reached if constraint violation occurs

} catch (const TableException& e) {
    // Consistency maintained - invalid state prevented
    txn->rollback();
}
```

### Isolation

Transactions are isolated from each other:

```cpp
// Thread 1
auto txn1 = table->begin_transaction();
txn1->insert_row({{"name", std::string("Alice")}});
// Changes not visible to other transactions until commit

// Thread 2 (running concurrently)
auto txn2 = table->begin_transaction();
auto rows = txn2->get_all_rows();  // Won't see Alice until txn1 commits
```

### Durability

Committed transactions persist across system restarts:

```cpp
auto txn = table->begin_transaction();
txn->insert_row({{"name", std::string("Important Data")}});
txn->commit();

// Data is now durable - will survive system restart
table->save_to_file("important_data.json");
```

## Concurrency Control

### Reader-Writer Locks

The system uses shared_mutex for efficient concurrency:

```cpp
// Multiple readers can access simultaneously
std::vector<std::thread> readers;
for (int i = 0; i < 10; ++i) {
    readers.emplace_back([&table]() {
        // Concurrent reads are allowed
        auto rows = table->get_all_rows();
        // Process rows...
    });
}

// Writers get exclusive access
std::thread writer([&table]() {
    auto txn = table->begin_transaction();
    // Exclusive write access
    txn->insert_row({{"name", std::string("New Data")}});
    txn->commit();
});

for (auto& r : readers) r.join();
writer.join();
```

### Concurrent Transaction Example

```cpp
#include <thread>
#include <vector>
#include <atomic>

void concurrent_transactions_example(Table* table) {
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    std::vector<std::thread> workers;

    // Create multiple worker threads
    for (int i = 0; i < 10; ++i) {
        workers.emplace_back([&, i]() {
            try {
                auto txn = table->begin_transaction();

                // Each thread inserts unique data
                txn->insert_row({
                    {"id", static_cast<std::int64_t>(1000 + i)},
                    {"worker_id", static_cast<std::int64_t>(i)},
                    {"data", std::string("worker_") + std::to_string(i)}
                });

                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                txn->commit();
                success_count++;

            } catch (const std::exception& e) {
                failure_count++;
                std::cout << "Worker " << i << " failed: " << e.what() << std::endl;
            }
        });
    }

    // Wait for all workers to complete
    for (auto& worker : workers) {
        worker.join();
    }

    std::cout << "Successful transactions: " << success_count << std::endl;
    std::cout << "Failed transactions: " << failure_count << std::endl;
}
```

## Thread Safety

### Safe Operations

All table operations are thread-safe:

```cpp
// Safe to call from multiple threads
table->insert_row({{"name", std::string("Thread Safe")}});
table->get_row(1);
table->query(query);
table->create_index("safe_idx", {"name"});

// Transactions are also thread-safe
auto txn1 = table->begin_transaction();  // Thread 1
auto txn2 = table->begin_transaction();  // Thread 2
```

### Lock-Free Read Operations

Read operations use shared locks for high concurrency:

```cpp
// Multiple threads can read simultaneously
std::vector<std::thread> readers;
for (int i = 0; i < 100; ++i) {
    readers.emplace_back([&table]() {
        auto rows = table->get_all_rows();
        auto count = table->get_row_count();

        TableQuery query;
        query.where("active", QueryOperator::Equal, true);
        auto active_rows = table->query(query);
    });
}

for (auto& reader : readers) {
    reader.join();
}
```

### Write Synchronization

Write operations are synchronized with exclusive locks:

```cpp
// Only one writer at a time, but automatic synchronization
std::vector<std::thread> writers;
for (int i = 0; i < 10; ++i) {
    writers.emplace_back([&table, i]() {
        auto txn = table->begin_transaction();
        txn->insert_row({
            {"id", static_cast<std::int64_t>(i)},
            {"data", std::string("writer_") + std::to_string(i)}
        });
        txn->commit();
    });
}

for (auto& writer : writers) {
    writer.join();
}
```

## Performance Considerations

### Transaction Batching

Batch operations in transactions for better performance:

```cpp
// Inefficient: One transaction per operation
for (int i = 0; i < 1000; ++i) {
    auto txn = table->begin_transaction();
    txn->insert_row({{"id", static_cast<std::int64_t>(i)}});
    txn->commit();
}

// Efficient: Batch operations in single transaction
auto txn = table->begin_transaction();
for (int i = 0; i < 1000; ++i) {
    txn->insert_row({{"id", static_cast<std::int64_t>(i)}});
}
txn->commit();
```

### Lock Duration Minimization

Keep transactions short to minimize lock contention:

```cpp
// Bad: Long-running transaction
auto txn = table->begin_transaction();
txn->insert_row({{"data", std::string("step1")}});

// Expensive operation while holding transaction
std::this_thread::sleep_for(std::chrono::seconds(10));

txn->insert_row({{"data", std::string("step2")}});
txn->commit();

// Good: Minimize transaction duration
{
    auto txn = table->begin_transaction();
    txn->insert_row({{"data", std::string("step1")}});
    txn->commit();
}

// Do expensive work outside transaction
std::this_thread::sleep_for(std::chrono::seconds(10));

{
    auto txn = table->begin_transaction();
    txn->insert_row({{"data", std::string("step2")}});
    txn->commit();
}
```

### Read-Heavy Workloads

Optimize for read-heavy workloads:

```cpp
// Prefer direct reads when possible (no transaction needed)
auto row = table->get_row(1);
auto all_rows = table->get_all_rows();

TableQuery query;
query.where("status", QueryOperator::Equal, std::string("active"));
auto active_rows = table->query(query);

// Use transactions only when data consistency is critical
auto txn = table->begin_transaction();
auto snapshot_rows = txn->get_all_rows();  // Consistent snapshot
txn->commit();
```

## Best Practices

### 1. Use RAII for Transaction Management

```cpp
class ScopedTransaction {
private:
    std::unique_ptr<TableTransaction> txn_;
    bool committed_ = false;

public:
    explicit ScopedTransaction(Table* table) : txn_(table->begin_transaction()) {}

    ~ScopedTransaction() {
        if (!committed_ && txn_->get_state() == TransactionState::Active) {
            txn_->rollback();
        }
    }

    void commit() {
        txn_->commit();
        committed_ = true;
    }

    TableTransaction* get() { return txn_.get(); }
};
```

### 2. Handle Exceptions Properly

```cpp
void safe_transaction_example(Table* table) {
    auto txn = table->begin_transaction();

    try {
        // Perform operations
        txn->insert_row({{"critical_data", std::string("important")}});

        // This might throw
        risky_operation();

        txn->commit();

    } catch (const TableException& e) {
        txn->rollback();
        std::cout << "Table error: " << e.what() << std::endl;
        throw;

    } catch (const std::exception& e) {
        txn->rollback();
        std::cout << "General error: " << e.what() << std::endl;
        throw;
    }
}
```

### 3. Avoid Long-Running Transactions

```cpp
// Bad: Long transaction blocks other operations
void bad_batch_insert(Table* table, const std::vector<RowData>& data) {
    auto txn = table->begin_transaction();

    for (const auto& row : data) {  // Could be thousands of rows
        txn->insert_row(row);

        // Expensive processing per row
        process_row(row);  // Takes 100ms per row
    }

    txn->commit();  // Transaction held for minutes
}

// Good: Batch processing with smaller transactions
void good_batch_insert(Table* table, const std::vector<RowData>& data) {
    const size_t batch_size = 100;

    for (size_t i = 0; i < data.size(); i += batch_size) {
        auto txn = table->begin_transaction();

        size_t end = std::min(i + batch_size, data.size());
        for (size_t j = i; j < end; ++j) {
            txn->insert_row(data[j]);
        }

        txn->commit();  // Short transaction duration

        // Do expensive processing outside transaction
        for (size_t j = i; j < end; ++j) {
            process_row(data[j]);
        }
    }
}
```

### 4. Use Appropriate Isolation Levels

```cpp
// For data analysis (snapshot consistency needed)
auto analyze_data(Table* table) {
    auto txn = table->begin_transaction();

    // Get consistent snapshot of all data
    auto rows = txn->get_all_rows();

    // Analyze data with guaranteed consistency
    auto result = perform_analysis(rows);

    txn->commit();  // Or rollback - no changes made
    return result;
}

// For simple reads (no transaction needed)
auto get_user_info(Table* table, std::int64_t user_id) {
    // Direct read is sufficient for simple queries
    return table->get_row(user_id);
}
```

## Examples

### Bank Transfer Simulation

```cpp
void transfer_funds(Table* accounts, std::int64_t from_id, std::int64_t to_id, double amount) {
    auto txn = accounts->begin_transaction();

    try {
        // Get source account
        auto from_row = txn->get_row(from_id);
        if (!from_row) {
            throw std::runtime_error("Source account not found");
        }

        // Get destination account
        auto to_row = txn->get_row(to_id);
        if (!to_row) {
            throw std::runtime_error("Destination account not found");
        }

        // Get current balances
        auto from_balance_val = from_row->get_cell_value("balance");
        auto to_balance_val = to_row->get_cell_value("balance");

        if (!from_balance_val || !to_balance_val) {
            throw std::runtime_error("Invalid account balance");
        }

        double from_balance = std::get<double>(*from_balance_val);
        double to_balance = std::get<double>(*to_balance_val);

        // Check sufficient funds
        if (from_balance < amount) {
            throw std::runtime_error("Insufficient funds");
        }

        // Perform transfer
        txn->update_row(from_id, {{"balance", from_balance - amount}});
        txn->update_row(to_id, {{"balance", to_balance + amount}});

        // Log transaction
        txn->insert_row({
            {"from_account", from_id},
            {"to_account", to_id},
            {"amount", amount},
            {"timestamp", current_timestamp()}
        });

        // Atomic commit - either all operations succeed or none
        txn->commit();

        std::cout << "Transfer completed successfully" << std::endl;

    } catch (const std::exception& e) {
        txn->rollback();
        std::cout << "Transfer failed: " << e.what() << std::endl;
        throw;
    }
}
```

### Concurrent Order Processing

```cpp
void process_orders_concurrently(Table* orders, Table* inventory) {
    std::vector<std::thread> workers;
    std::atomic<int> processed_count{0};

    // Get pending orders
    TableQuery pending_query;
    pending_query.where("status", QueryOperator::Equal, std::string("pending"));
    auto pending_orders = orders->query(pending_query);

    // Process orders concurrently
    for (const auto& order : pending_orders) {
        workers.emplace_back([&, order]() {
            try {
                auto txn = orders->begin_transaction();

                auto order_id_val = order.get_cell_value("id");
                auto product_id_val = order.get_cell_value("product_id");
                auto quantity_val = order.get_cell_value("quantity");

                if (!order_id_val || !product_id_val || !quantity_val) {
                    return;
                }

                auto order_id = std::get<std::int64_t>(*order_id_val);
                auto product_id = std::get<std::int64_t>(*product_id_val);
                auto quantity = std::get<std::int64_t>(*quantity_val);

                // Check inventory (in separate transaction for inventory table)
                auto inv_txn = inventory->begin_transaction();
                auto product = inv_txn->get_row(product_id);

                if (product) {
                    auto stock_val = product->get_cell_value("stock");
                    if (stock_val && std::get<std::int64_t>(*stock_val) >= quantity) {
                        // Update inventory
                        auto current_stock = std::get<std::int64_t>(*stock_val);
                        inv_txn->update_row(product_id, {
                            {"stock", current_stock - quantity}
                        });
                        inv_txn->commit();

                        // Update order status
                        txn->update_row(order_id, {
                            {"status", std::string("fulfilled")},
                            {"fulfilled_at", current_timestamp()}
                        });
                        txn->commit();

                        processed_count++;
                    } else {
                        inv_txn->rollback();
                        txn->rollback();
                    }
                } else {
                    inv_txn->rollback();
                    txn->rollback();
                }

            } catch (const std::exception& e) {
                std::cout << "Order processing error: " << e.what() << std::endl;
            }
        });
    }

    // Wait for all workers
    for (auto& worker : workers) {
        worker.join();
    }

    std::cout << "Processed " << processed_count << " orders" << std::endl;
}
```

---

_See also: [Table API Reference](Tables-API.md), [Performance Guide](Tables-Performance.md), [Best Practices](Tables-BestPractices.md)_
