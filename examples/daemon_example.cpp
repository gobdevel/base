/*
 * @file daemon_example.cpp
 * @brief Example demonstrating daemonization feature of the application framework
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include <iostream>

using namespace base;

class DaemonApp : public Application {
public:
    DaemonApp() : Application(create_daemon_config()) {}

protected:
    bool on_initialize() override {
        Logger::info("Daemon application initializing...");

        // Set up a simple recurring task to demonstrate the daemon is working
        schedule_recurring_task([this]() {
            static int counter = 0;
            Logger::info("Daemon heartbeat #{}", ++counter);
        }, std::chrono::seconds(30));

        return true;
    }

    bool on_start() override {
        Logger::info("Daemon application started successfully");
        return true;
    }

    bool on_stop() override {
        Logger::info("Daemon application stopping...");
        return true;
    }

private:
    ApplicationConfig create_daemon_config() {
        ApplicationConfig config;
        config.name = "daemon_example";
        config.version = "1.0.0";
        config.description = "Example daemon application with command line support";

        // Default daemon settings (can be overridden by command line)
        config.daemon_work_dir = "/tmp";
        config.daemon_pid_file = "/tmp/daemon_example.pid";
        config.daemon_log_file = "/tmp/daemon_example.log";
        config.daemon_umask = 022;
        config.daemon_close_fds = false;  // Don't close FDs to help with debugging

        // Application settings
        config.worker_threads = 2;
        config.enable_health_check = true;
        config.health_check_interval = std::chrono::seconds(60);

        return config;
    }
};

// Use the new BASE_APPLICATION_MAIN macro
BASE_APPLICATION_MAIN(DaemonApp)
