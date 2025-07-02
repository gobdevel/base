/*
 * @file minimal_test.cpp
 * @brief Minimal test of Application framework without CLI
 */

#include "application.h"
#include "logger.h"
#include <iostream>

using namespace crux;

class MinimalApp : public Application {
public:
    MinimalApp() : Application({
        .name = "Minimal Test",
        .version = "1.0.0",
        .worker_threads = 1,
        .enable_cli = false  // Completely disable CLI
    }) {}

protected:
    bool on_start() override {
        Logger::info("Minimal app started");

        // Post a task to shutdown immediately
        post_task([this]() {
            std::cout << "Minimal app test successful!\n";
            shutdown();
        });

        return true;
    }
};

int main() {
    try {
        Logger::set_level(LogLevel::Info);

        MinimalApp app;
        int result = app.run();

        std::cout << "Application exited with code: " << result << std::endl;
        return result;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
