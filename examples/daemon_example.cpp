/*
 * @file daemon_example.cpp
 * @brief Comprehensive daemon application demonstration
 *
 * Features demonstrated:
 * - Daemonization (background process)
 * - PID file management
 * - Signal handling for daemons
 * - Log file management
 * - User/group switching
 * - Command line argument processing
 * - Graceful shutdown
 * - Status monitoring
 *
 * Usage:
 *   ./daemon_example --daemon --pid-file /var/run/myapp.pid --log-file /var/log/myapp.log
 *   ./daemon_example --help
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <application.h>
#include <logger.h>
#include <iostream>
#include <fstream>
#include <atomic>

using namespace base;

/**
 * @brief Example daemon service component
 */
class ServiceComponent : public ApplicationComponent {
public:
    bool initialize(Application& app) override {
        Logger::info("Initializing service component");
        app_ = &app;

        // Schedule periodic work
        app.schedule_recurring_task([this]() {
            perform_service_work();
        }, std::chrono::seconds(10));

        return true;
    }

    bool start() override {
        Logger::info("Starting service component");
        is_running_ = true;
        start_time_ = std::chrono::steady_clock::now();
        return true;
    }

    bool stop() override {
        Logger::info("Stopping service component");
        is_running_ = false;
        return true;
    }

    std::string_view name() const override { return "Service"; }

    bool health_check() const override {
        // Simulate health check
        auto uptime = std::chrono::steady_clock::now() - start_time_;
        auto uptime_minutes = std::chrono::duration_cast<std::chrono::minutes>(uptime).count();

        // Report unhealthy if we've been running too long (for demo purposes)
        bool healthy = is_running_ && uptime_minutes < 60;

        if (!healthy) {
            Logger::warn("Service health check failed - uptime: {} minutes", uptime_minutes);
        }

        return healthy;
    }

    std::chrono::steady_clock::time_point get_start_time() const { return start_time_; }
    std::size_t get_work_count() const { return work_count_.load(); }

private:
    void perform_service_work() {
        work_count_++;

        Logger::info("Service work iteration #{}", work_count_.load());

        // Simulate different types of work
        if (work_count_ % 3 == 0) {
            Logger::debug("Performing database maintenance");
        }
        if (work_count_ % 5 == 0) {
            Logger::debug("Cleaning up temporary files");
        }
        if (work_count_ % 7 == 0) {
            Logger::warn("Service warning: high memory usage detected");
        }

        // Simulate service completion after demo period
        if (work_count_ >= 20) {
            Logger::info("Service work completed - shutting down daemon");
            app_->shutdown();
        }
    }

    Application* app_ = nullptr;
    bool is_running_ = false;
    std::atomic<std::size_t> work_count_{0};
    std::chrono::steady_clock::time_point start_time_;
};

/**
 * @brief Comprehensive daemon application
 */
class DaemonApp : public Application {
public:
    DaemonApp() : Application(create_daemon_config()) {}

protected:
    bool on_initialize() override {
        Logger::info("=== Daemon Application Demo ===");
        Logger::info("PID: {}", getpid());
        Logger::info("Working directory: {}", std::filesystem::current_path().string());

        // Add service component
        add_component(std::make_unique<ServiceComponent>());

        // Set up status reporting
        schedule_recurring_task([this]() {
            report_daemon_status();
        }, std::chrono::seconds(30));

        // Set up periodic statistics
        schedule_recurring_task([this]() {
            log_daemon_statistics();
        }, std::chrono::minutes(1));

        return true;
    }

    bool on_start() override {
        Logger::info("Daemon application started successfully");
        Logger::info("Configuration:");
        Logger::info("  Name: {}", config().name);
        Logger::info("  Version: {}", config().version);
        Logger::info("  PID file: {}", config().daemon_pid_file);
        Logger::info("  Working directory: {}", config().daemon_work_dir);
        Logger::info("  User: {}", config().daemon_user.empty() ? "current" : config().daemon_user);
        Logger::info("  Log file: {}", config().daemon_log_file.empty() ? "default" : config().daemon_log_file);

        return true;
    }

    bool on_stop() override {
        Logger::info("Daemon application stopping gracefully...");
        return true;
    }

    void on_cleanup() override {
        Logger::info("Daemon cleanup completed");
    }

    void on_signal(int signal) override {
        switch (signal) {
            case SIGTERM:
                Logger::info("Received SIGTERM - initiating graceful shutdown");
                shutdown();
                break;
            case SIGINT:
                Logger::info("Received SIGINT - initiating graceful shutdown");
                shutdown();
                break;
            case SIGHUP:
                Logger::info("Received SIGHUP - reloading configuration");
                reload_config();
                break;
            case SIGUSR1:
                Logger::info("Received SIGUSR1 - dumping daemon status");
                dump_comprehensive_status();
                break;
            case SIGUSR2:
                Logger::info("Received SIGUSR2 - rotating logs");
                rotate_logs();
                break;
            default:
                Logger::debug("Received signal: {} - ignoring", signal);
                break;
        }
    }

    void on_error(const std::exception& error) override {
        Logger::error("Daemon error: {}", error.what());

        // Implement error recovery for production daemons
        error_count_++;
        if (error_count_ > 10) {
            Logger::critical("Too many errors - shutting down daemon");
            force_shutdown();
        }
    }

private:
    static ApplicationConfig create_daemon_config() {
        ApplicationConfig config;
        config.name = "daemon_example";
        config.version = "1.0.0";
        config.description = "Comprehensive daemon application demonstration";

        // Daemon configuration
        config.daemonize = false;  // Will be set by command line parsing
        config.daemon_work_dir = "/tmp";
        config.daemon_pid_file = "/tmp/daemon_example.pid";
        config.daemon_log_file = "/tmp/daemon_example.log";
        config.daemon_user = "";  // Run as current user
        config.daemon_group = "";
        config.daemon_umask = 022;
        config.daemon_close_fds = true;

        // Application settings
        config.worker_threads = 2;
        config.enable_health_check = true;
        config.health_check_interval = std::chrono::seconds(15);

        return config;
    }

    void report_daemon_status() {
        Logger::info("=== Daemon Status Report ===");
        Logger::info("Uptime: {} seconds", get_uptime_seconds());
        Logger::info("State: {}", state_to_string(state()));
        Logger::info("Worker threads: {}", config().worker_threads);
        Logger::info("Managed threads: {}", managed_thread_count());

        // Component status
        if (auto* service = get_component("Service")) {
            Logger::info("Service component: health = {}", service->health_check());
            if (auto* svc = dynamic_cast<ServiceComponent*>(service)) {
                Logger::info("Service work iterations: {}", svc->get_work_count());
            }
        }
    }

    void log_daemon_statistics() {
        Logger::info("=== Daemon Statistics ===");
        Logger::info("Total errors: {}", error_count_.load());
        Logger::info("Memory usage: {} MB", get_memory_usage_mb());
        Logger::info("CPU time: {} ms", get_cpu_time_ms());
    }

    void dump_comprehensive_status() {
        Logger::info("=== Comprehensive Daemon Status ===");
        Logger::info("Process ID: {}", getpid());
        Logger::info("Parent PID: {}", getppid());
        Logger::info("Working directory: {}", std::filesystem::current_path().string());
        Logger::info("Configuration file: {}", config().config_file);
        Logger::info("PID file: {}", config().daemon_pid_file);
        Logger::info("Log file: {}", config().daemon_log_file);
        Logger::info("User/Group: {}/{}",
                    config().daemon_user.empty() ? "current" : config().daemon_user,
                    config().daemon_group.empty() ? "current" : config().daemon_group);
        Logger::info("Umask: {:o}", config().daemon_umask);

        // Runtime statistics
        report_daemon_status();
        log_daemon_statistics();
    }

    void rotate_logs() {
        Logger::info("Log rotation requested");
        // In a real daemon, you would implement log rotation here
        Logger::flush();
        Logger::info("Log rotation completed");
    }

    std::string state_to_string(ApplicationState s) const {
        switch (s) {
            case ApplicationState::Created: return "Created";
            case ApplicationState::Initialized: return "Initialized";
            case ApplicationState::Starting: return "Starting";
            case ApplicationState::Running: return "Running";
            case ApplicationState::Stopping: return "Stopping";
            case ApplicationState::Stopped: return "Stopped";
            case ApplicationState::Failed: return "Failed";
            default: return "Unknown";
        }
    }

    long get_uptime_seconds() const {
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
    }

    // Simplified system metrics (in a real daemon, use proper system APIs)
    size_t get_memory_usage_mb() const {
        // Simulate memory usage
        return 45 + (get_uptime_seconds() / 10);
    }

    size_t get_cpu_time_ms() const {
        // Simulate CPU time
        return get_uptime_seconds() * 100;
    }

    std::atomic<int> error_count_{0};
};

int main(int argc, char* argv[]) {
    try {
        DaemonApp app;

        std::cout << "Daemon Application Example\n";
        std::cout << "==========================\n";
        std::cout << "Usage: " << argv[0] << " [options]\n";
        std::cout << "Options:\n";
        std::cout << "  --daemon          Run as daemon (background)\n";
        std::cout << "  --pid-file FILE   Specify PID file location\n";
        std::cout << "  --log-file FILE   Specify log file location\n";
        std::cout << "  --user USER       Run as specified user\n";
        std::cout << "  --help            Show this help\n";
        std::cout << "\nSignals:\n";
        std::cout << "  SIGTERM/SIGINT - Graceful shutdown\n";
        std::cout << "  SIGHUP - Reload configuration\n";
        std::cout << "  SIGUSR1 - Dump status\n";
        std::cout << "  SIGUSR2 - Rotate logs\n";
        std::cout << "\n";

        return app.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal daemon error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown fatal daemon error occurred" << std::endl;
        return EXIT_FAILURE;
    }
}
