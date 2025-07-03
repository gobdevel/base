/*
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * SPDX-License-Identifier: MIT
 */

#include <logger.h>
#include <config.h>

int main() {
    std::string toml_config = R"(
    [base_example]

    [base_example.app]
    name = "base_example"
    version = "1.0.0"
    debug_mode = true
    )";
    // Initialize the logger with console output
    base::Logger::init();

    // Log messages at different levels
    base::Logger::info("Application started");
    base::Logger::warn("Low memory warning: {}MB remaining", 128);
    base::Logger::error("Failed to connect to database: {}", "Connection timeout");

    // Load configuration from a string
    auto& config_manager = base::ConfigManager::instance();
    if (config_manager.load_from_string(toml_config, "base_example")) {
        base::Logger::info("Configuration loaded successfully for app 'base_example'");
        auto app_config = config_manager.get_app_config("base_example");
        base::Logger::info("App Name: {}", app_config.name);
        base::Logger::info("App Version: {}", app_config.version);
    } else {
        base::Logger::error("Failed to load configuration for app 'base_example'");
    }

    return 0;
}
