#include "logger.h"
#include <iostream>

int main() {
    // Initialize logger
    base::Logger::init();

    std::cout << "Testing ComponentLogger - automatic component prepending...\n";

    // Create component loggers that automatically prepend component names
    auto database = base::Logger::get_component_logger("Database");
    auto network = base::Logger::get_component_logger("Network");
    auto auth = base::Logger::get_component_logger("Authentication");

    // Each logger automatically includes its component name
    database.info("Connection established to primary server");
    database.debug("Query executed in 45ms");
    database.warn("Connection pool nearly full: 95% utilization");

    network.info("HTTP request to api.example.com");
    network.error("Connection timeout after 30 seconds");

    auth.info("User 'admin' logged in successfully");
    auth.critical("Multiple failed login attempts detected!");

    std::cout << "\nTesting convenience macros...\n";

    // Test the convenience macros
    COMPONENT_LOGGER(cache);
    COMPONENT_LOGGER_NAMED(storage, "FileStorage");

    cache.info("Cache hit rate: 85%");
    cache.warn("Memory usage high: 90%");

    storage.info("File backup completed successfully");
    storage.error("Disk space low: 5% remaining");

    std::cout << "\nComponent logger test completed!\n";
    std::cout << "Each component automatically prepends its name to log messages.\n";

    return 0;
}
