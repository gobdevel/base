/*
 * @file table_dump_example.cpp
 * @brief Example demonstrating table dump/print functionality with paging
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "tables.h"
#include "logger.h"
#include <iostream>

using namespace base;

int main() {
    Logger::init();

    std::cout << "=== Table Dump/Print API Example ===" << std::endl;

    // Create a table with sample data
    auto schema = std::make_unique<TableSchema>("employees", 1);
    schema->add_column(ColumnDefinition("id", ColumnType::Integer, false));
    schema->add_column(ColumnDefinition("name", ColumnType::String, false));
    schema->add_column(ColumnDefinition("email", ColumnType::String, true));
    schema->add_column(ColumnDefinition("salary", ColumnType::Double, true));
    schema->add_column(ColumnDefinition("active", ColumnType::Boolean, false));
    schema->set_primary_key({"id"});

    auto table = std::make_unique<Table>(std::move(schema));

    // Insert sample data
    std::vector<std::unordered_map<std::string, CellValue>> sample_data = {
        {{"id", 1L}, {"name", std::string("Alice Johnson")}, {"email", std::string("alice@company.com")}, {"salary", 75000.0}, {"active", true}},
        {{"id", 2L}, {"name", std::string("Bob Smith")}, {"email", std::string("bob@company.com")}, {"salary", 68000.0}, {"active", true}},
        {{"id", 3L}, {"name", std::string("Carol Davis")}, {"email", std::string("carol@company.com")}, {"salary", 82000.0}, {"active", false}},
        {{"id", 4L}, {"name", std::string("David Wilson")}, {"email", std::string("david@company.com")}, {"salary", 71000.0}, {"active", true}},
        {{"id", 5L}, {"name", std::string("Eva Martinez")}, {"email", std::string("eva@company.com")}, {"salary", 79000.0}, {"active", true}},
        {{"id", 6L}, {"name", std::string("Frank Brown")}, {"email", std::string("frank@company.com")}, {"salary", 65000.0}, {"active", false}},
        {{"id", 7L}, {"name", std::string("Grace Lee")}, {"email", std::string("grace@company.com")}, {"salary", 88000.0}, {"active", true}},
        {{"id", 8L}, {"name", std::string("Henry Chen")}, {"email", std::string("henry@company.com")}, {"salary", 73000.0}, {"active", true}},
        {{"id", 9L}, {"name", std::string("Ivy Taylor")}, {"email", std::string("ivy@company.com")}, {"salary", 76000.0}, {"active", false}},
        {{"id", 10L}, {"name", std::string("Jack Adams")}, {"email", std::string("jack@company.com")}, {"salary", 69000.0}, {"active", true}}
    };

    for (const auto& row_data : sample_data) {
        table->insert_row(row_data);
    }

    std::cout << "\n1. Default ASCII table format:" << std::endl;
    table->dump();

    std::cout << "\n\n2. CSV format:" << std::endl;
    TableDumpOptions csv_options;
    csv_options.format = TableOutputFormat::CSV;
    csv_options.show_row_numbers = true;
    table->dump(csv_options);

    std::cout << "\n\n3. JSON format:" << std::endl;
    TableDumpOptions json_options;
    json_options.format = TableOutputFormat::JSON;
    json_options.show_row_numbers = true;
    table->dump(json_options);

    std::cout << "\n\n4. Markdown format:" << std::endl;
    TableDumpOptions md_options;
    md_options.format = TableOutputFormat::Markdown;
    md_options.show_row_numbers = true;
    table->dump(md_options);

    std::cout << "\n\n5. Paged output (3 rows per page):" << std::endl;
    TableDumpOptions paged_options;
    paged_options.page_size = 3;
    paged_options.show_row_numbers = true;

    auto pager = table->create_pager(paged_options);

    std::cout << "Page 1:" << std::endl;
    pager->show_page(0);

    std::cout << "\nPage 2:" << std::endl;
    pager->show_page(1);

    std::cout << "\nPage 3:" << std::endl;
    pager->show_page(2);

    std::cout << "\n\n6. Filtered output (active employees only):" << std::endl;
    TableDumpOptions filtered_options;
    filtered_options.filter_query.where("active", QueryOperator::Equal, true);
    filtered_options.show_row_numbers = true;
    table->dump(filtered_options);

    std::cout << "\n\n7. Specific columns only:" << std::endl;
    TableDumpOptions column_options;
    column_options.columns_to_show = {"name", "salary", "active"};
    column_options.show_row_numbers = true;
    table->dump(column_options);

    std::cout << "\n\n8. Table summary:" << std::endl;
    table->print_summary();

    std::cout << "\n\n9. Table schema:" << std::endl;
    table->print_schema();

    std::cout << "\n\n10. Table statistics:" << std::endl;
    table->print_statistics();

    std::cout << "\n=== Table Dump Example Completed ===" << std::endl;

    return 0;
}
