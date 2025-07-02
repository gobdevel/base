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
    [crux_example]

    [crux_example.app]
    name = "crux_example"
    version = "1.0.0"
    debug_mode = true
    )";
    // Initialize the logger with console output
    crux::Logger::init();

    // Log messages at different levels
    crux::Logger::info("Application started");
    crux::Logger::warn("Low memory warning: {}MB remaining", 128);
    crux::Logger::error("Failed to connect to database: {}", "Connection timeout");

    // Load configuration from a string
    auto& config_manager = crux::ConfigManager::instance();
    if (config_manager.load_from_string(toml_config, "crux_example")) {
        crux::Logger::info("Configuration loaded successfully for app 'crux_example'");
        auto app_config = config_manager.get_app_config("crux_example");
        crux::Logger::info("App Name: {}", app_config.name);
        crux::Logger::info("App Version: {}", app_config.version);
    } else {
        crux::Logger::error("Failed to load configuration for app 'crux_example'");
    }

    return 0;
}
