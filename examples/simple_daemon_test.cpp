/*
 * @file simple_daemon_test.cpp
 * @brief Simple test to verify daemonization works
 */

#include "application.h"
#include <iostream>
#include <fstream>

using namespace base;

class SimpleDaemon : public Application {
public:
    SimpleDaemon() : Application(create_config()) {}

protected:
    bool on_initialize() override {
        // Write to a test file to prove we're running
        std::ofstream test_file("/tmp/daemon_test_init.txt");
        test_file << "Daemon initialized at PID: " << getpid() << std::endl;
        test_file.close();

        // Schedule a task to write periodically
        schedule_recurring_task([this]() {
            std::ofstream test_file("/tmp/daemon_test_running.txt", std::ios::app);
            test_file << "Daemon running at PID: " << getpid() << " at " << std::time(nullptr) << std::endl;
            test_file.close();
        }, std::chrono::seconds(5));

        return true;
    }

    bool on_start() override {
        std::ofstream test_file("/tmp/daemon_test_started.txt");
        test_file << "Daemon started at PID: " << getpid() << std::endl;
        test_file.close();
        return true;
    }

private:
    ApplicationConfig create_config() {
        ApplicationConfig config;
        config.name = "simple_daemon_test";
        config.daemonize = true;
        config.daemon_work_dir = "/tmp";
        config.daemon_pid_file = "/tmp/simple_daemon.pid";
        config.daemon_log_file = "/tmp/simple_daemon.log";
        config.daemon_close_fds = false;  // Keep FDs open for debugging
        config.worker_threads = 1;
        config.enable_health_check = false;  // Disable health check for simplicity
        return config;
    }
};

int main() {
    try {
        std::cout << "Starting simple daemon test..." << std::endl;
        SimpleDaemon daemon;
        return daemon.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
