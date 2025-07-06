#include "logger.h"
#include <iostream>

int main() {
    // Initialize logger
    base::Logger::init();

    std::cout << "Testing new convenience methods...\n";

    // Test the ready() method
    if (base::Logger::ready()) {
        base::Logger::info("Logger is ready for use");
    }

    // Test the component() convenience method
    auto network = base::Logger::component("Network");
    auto database = base::Logger::component("Database");

    base::Logger::info(network, "Using convenient component creation");
    base::Logger::debug(database, "Component method is clean and readable");

    // Test convenience macros
    std::cout << "\nTesting convenience macros...\n";

    // Create component variables using macro
    LOGGER_COMPONENT(auth);
    LOGGER_COMPONENT(cache);

    base::Logger::info(auth, "Component variable created with macro");
    base::Logger::warn(cache, "Cache is full, clearing...");

    // Direct logging with macros
    LOGGER_INFO("API", "Request received");
    LOGGER_ERROR("Storage", "Disk space low");

    std::cout << "\nConvenience methods and macros test completed!\n";

    return 0;
}
