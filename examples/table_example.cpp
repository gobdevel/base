#include "tables.h"
#include "logger.h"
#include <iostream>

using namespace base;

int main() {
    // Initialize logger
    base::Logger::init();

    try {
        // Create a schema for a simple table
        auto schema = std::make_unique<TableSchema>("users", 1);

        // Add columns
        schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
        schema->add_column(ColumnDefinition("name", ColumnType::String, false));
        schema->add_column(ColumnDefinition("email", ColumnType::String, false));
        schema->add_column(ColumnDefinition("age", ColumnType::Integer, true));

        // Set primary key
        schema->set_primary_key({"id"});

        // Create table
        auto table = std::make_unique<Table>(std::move(schema));

        base::Logger::info("Created table with schema: {}", table->get_schema().get_name());

        // Insert some rows
        std::unordered_map<std::string, CellValue> row1 = {
            {"id", static_cast<std::int64_t>(1)},
            {"name", std::string("John Doe")},
            {"email", std::string("john@example.com")},
            {"age", static_cast<std::int64_t>(30)}
        };

        std::unordered_map<std::string, CellValue> row2 = {
            {"id", static_cast<std::int64_t>(2)},
            {"name", std::string("Jane Smith")},
            {"email", std::string("jane@example.com")},
            {"age", static_cast<std::int64_t>(25)}
        };

        auto row1_id = table->insert_row(row1);
        auto row2_id = table->insert_row(row2);

        base::Logger::info("Inserted rows with IDs: {} and {}", row1_id, row2_id);

        // Query all rows
        auto all_rows = table->get_all_rows();
        base::Logger::info("Table contains {} rows", all_rows.size());

        // Create an index
        table->create_index("name_index", {"name"});
        base::Logger::info("Created index on 'name' column");

        // Query by index
        auto query = TableQuery();
        query.where("name", QueryOperator::Equal, std::string("John Doe"));
        auto results = table->query(query);

        if (!results.empty()) {
            base::Logger::info("Found user with name 'John Doe'");
        }

        // Test serialization
        auto json_data = table->to_json();
        base::Logger::info("Table serialized to JSON successfully (length: {})", json_data.length());

        // Get statistics
        auto stats = table->get_statistics();
        base::Logger::info("Table statistics - Rows: {}, Inserts: {}, Version: {}",
                                stats.row_count, stats.total_inserts, stats.schema_version);

        base::Logger::info("Table implementation test completed successfully!");

    } catch (const std::exception& e) {
        base::Logger::error("Table test failed with exception: {}", e.what());
        return 1;
    }

    return 0;
}
