/*
 * @file cli_args_example.cpp
 * @brief Example demonstrating command line argument support
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"

using namespace base;

class CliArgsApp : public Application {
public:
    CliArgsApp() : Application(create_config()) {}

protected:
    bool on_initialize() override {
        Logger::info("Application initializing with configuration:");
        Logger::info("  - Name: {}", config().name);
        Logger::info("  - Version: {}", config().version);
        Logger::info("  - Daemon mode: {}", config().daemonize ? "enabled" : "disabled");
        Logger::info("  - PID file: {}", config().daemon_pid_file.empty() ? "none" : config().daemon_pid_file);
        Logger::info("  - Log file: {}", config().daemon_log_file.empty() ? "console" : config().daemon_log_file);
        Logger::info("  - Working directory: {}", config().daemon_work_dir);

        // Schedule a simple task to show the application is working
        schedule_recurring_task([this]() {
            static int counter = 0;
            Logger::info("Application heartbeat #{}", ++counter);
        }, std::chrono::seconds(5));

        return true;
    }

    bool on_start() override {
        Logger::info("CLI Args Example application started successfully");
        return true;
    }

    bool on_stop() override {
        Logger::info("CLI Args Example application stopping...");
        return true;
    }

private:
    ApplicationConfig create_config() {
        ApplicationConfig config;
        config.name = "cli_args_example";
        config.version = "1.0.0";
        config.description = "Example application demonstrating command line argument support";

        // Default settings (can be overridden by command line)
        config.worker_threads = 1;
        config.enable_health_check = false;

        // Default daemon settings
        config.daemon_work_dir = "/tmp";
        config.daemon_pid_file = "/tmp/cli_args_example.pid";
        config.daemon_log_file = "/tmp/cli_args_example.log";

        return config;
    }
};

// Use the BASE_APPLICATION_MAIN macro for automatic command line support
BASE_APPLICATION_MAIN(CliArgsApp)
