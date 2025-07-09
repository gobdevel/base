/*
 * @file application.cpp
 * @brief Modern C++20 application framework implementation
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include "cli.h"

#include <algorithm>
#include <csignal>
#include <future>
#include <iomanip>
#include <iostream>

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#endif

namespace base {

Application::Application(ApplicationConfig config)
    : config_(std::move(config))
    , signals_(io_context_)
{
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    // Initialize static instance for signal handling
    if (!signal_instance_) {
        signal_instance_ = this;
    }
#endif
    Logger::info("Application '{}' created (version: {})", config_.name, config_.version);
    Logger::debug("Worker threads: {}", config_.worker_threads);
}

Application::~Application() {
    if (state_.load() != ApplicationState::Stopped &&
        state_.load() != ApplicationState::Failed) {
        Logger::warn("Application destroyed without proper shutdown");
        force_shutdown();
    }
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    remove_pid_file();
    // Clear static instance
    if (signal_instance_ == this) {
        signal_instance_ = nullptr;
    }
#endif
    Logger::info("Application '{}' destroyed", config_.name);
}

int Application::run() {
    Logger::info("Starting application '{}' v{}", config_.name, config_.version);
    Logger::info("Description: {}", config_.description);

    try {
        // Daemonize if requested (must be done before any other initialization)
        if (!daemonize()) {
            Logger::critical("Daemonization failed");
            change_state(ApplicationState::Failed);
            return EXIT_FAILURE;
        }

        // Initialize the application
        if (!initialize_internal()) {
            Logger::critical("Application initialization failed");
            change_state(ApplicationState::Failed);
            return EXIT_FAILURE;
        }

        // Start the application
        if (!start_internal()) {
            Logger::critical("Application startup failed");
            change_state(ApplicationState::Failed);
            return EXIT_FAILURE;
        }

        // Run the event loop
        Logger::debug("Starting event loop with {} worker threads", config_.worker_threads);
        start_worker_threads();

        Logger::info("Application '{}' started successfully", config_.name);
        change_state(ApplicationState::Running);

        // Wait for shutdown signal
        {
            std::unique_lock<std::mutex> lock(stop_mutex_);
            stop_condition_.wait(lock, [this] {
                return state_.load() == ApplicationState::Stopping ||
                       state_.load() == ApplicationState::Stopped ||
                       state_.load() == ApplicationState::Failed;
            });
        }

        Logger::info("Application shutdown initiated");
        stop_internal();

        Logger::info("Application '{}' stopped successfully", config_.name);
        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        Logger::critical("Fatal application error: {}", e.what());
        handle_exception(e);
        change_state(ApplicationState::Failed);
        return EXIT_FAILURE;
    } catch (...) {
        Logger::critical("Unknown fatal application error");
        change_state(ApplicationState::Failed);
        return EXIT_FAILURE;
    }
}

int Application::run(int argc, char* argv[]) {
    // Parse command line arguments first
    if (config_.parse_command_line) {
        if (!parse_command_line_args(argc, argv)) {
            return EXIT_SUCCESS; // Help or version was shown, exit cleanly
        }
    }

    // Apply command line overrides to configuration
    apply_command_line_overrides();

    // Continue with normal startup
    return run();
}

bool Application::parse_command_line_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            show_help(argv[0]);
            config_.show_help_and_exit = true;
            return false;
        }
        else if (arg == "--version" || arg == "-v") {
            show_version();
            config_.show_version_and_exit = true;
            return false;
        }
        else if (arg == "--daemon" || arg == "-d") {
            config_.daemonize = true;
        }
        else if (arg == "--no-daemon" || arg == "-f") {
            config_.force_foreground = true;
            config_.daemonize = false;
        }
        else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                config_.custom_config_file = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a filename argument" << std::endl;
                return false;
            }
        }
        else if (arg == "--log-level" || arg == "-l") {
            if (i + 1 < argc) {
                config_.custom_log_level = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a level argument (debug|info|warn|error|critical)" << std::endl;
                return false;
            }
        }
        else if (arg == "--log-file") {
            if (i + 1 < argc) {
                config_.custom_log_file = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a filename argument" << std::endl;
                return false;
            }
        }
        else if (arg == "--pid-file") {
            if (i + 1 < argc) {
                config_.daemon_pid_file = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a filename argument" << std::endl;
                return false;
            }
        }
        else if (arg == "--work-dir") {
            if (i + 1 < argc) {
                config_.daemon_work_dir = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a directory argument" << std::endl;
                return false;
            }
        }
        else if (arg == "--user") {
            if (i + 1 < argc) {
                config_.daemon_user = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a username argument" << std::endl;
                return false;
            }
        }
        else if (arg == "--group") {
            if (i + 1 < argc) {
                config_.daemon_group = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a group name argument" << std::endl;
                return false;
            }
        }
        else if (arg.starts_with("--")) {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            return false;
        }
        else {
            std::cerr << "Error: Unexpected argument: " << arg << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            return false;
        }
    }

    return true; // Continue with application startup
}

void Application::show_help(const char* program_name) const {
    std::cout << config_.name << " v" << config_.version << "\n";
    std::cout << config_.description << "\n\n";
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message and exit\n";
    std::cout << "  -v, --version           Show version information and exit\n";
    std::cout << "  -d, --daemon            Run as daemon process\n";
    std::cout << "  -f, --no-daemon         Force foreground mode (override config)\n";
    std::cout << "  -c, --config FILE       Use specified configuration file\n";
    std::cout << "  -l, --log-level LEVEL   Set log level (debug|info|warn|error|critical)\n";
    std::cout << "      --log-file FILE     Write logs to specified file\n";
    std::cout << "      --pid-file FILE     Write process ID to specified file (daemon mode)\n";
    std::cout << "      --work-dir DIR      Set working directory (daemon mode)\n";
    std::cout << "      --user USER         Run as specified user (daemon mode)\n";
    std::cout << "      --group GROUP       Run as specified group (daemon mode)\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " --daemon --pid-file /var/run/app.pid\n";
    std::cout << "  " << program_name << " --config /etc/app.conf --log-level debug\n";
    std::cout << "  " << program_name << " --no-daemon --log-file app.log\n";
}

void Application::show_version() const {
    std::cout << config_.name << " version " << config_.version << "\n";
    std::cout << config_.description << "\n";
}

void Application::apply_command_line_overrides() {
    // Apply custom config file if specified
    if (!config_.custom_config_file.empty()) {
        config_.config_file = config_.custom_config_file;
    }

    // Apply custom log file if specified
    if (!config_.custom_log_file.empty()) {
        config_.daemon_log_file = config_.custom_log_file;
    }

    // Apply log level if specified
    if (!config_.custom_log_level.empty()) {
        // Note: This would need to be integrated with the logger initialization
        // For now, we just store it and it can be used when initializing the logger
        Logger::debug("Custom log level specified: {}", config_.custom_log_level);
    }

    // Ensure foreground mode overrides daemon mode
    if (config_.force_foreground) {
        config_.daemonize = false;
        Logger::debug("Forcing foreground mode (daemon mode disabled)");
    }
}

void Application::shutdown() {
    Logger::info("Graceful shutdown requested");
    change_state(ApplicationState::Stopping);

    // Wake up the main thread
    stop_condition_.notify_all();
}

void Application::force_shutdown() {
    Logger::warn("Force shutdown requested");
    change_state(ApplicationState::Stopping);

    // Request immediate stop for all managed threads
    stop_all_managed_threads();

    // Stop the IO context immediately
    io_context_.stop();

    // Wake up the main thread
    stop_condition_.notify_all();
}

bool Application::initialize_internal() {
    Logger::debug("Initializing application components");
    // State will be set to Initialized at the end of successful initialization

    try {
        // Load configuration if specified
        if (!config_.config_file.empty()) {
            auto& config_manager = ConfigManager::instance();
            if (config_manager.load_config(config_.config_file, config_.config_app_name)) {
                Logger::info("Configuration loaded from: {}", config_.config_file);

                // Configure logger from config
                config_manager.configure_logger(config_.config_app_name);
                Logger::info("Logger configured from configuration file");
            } else {
                Logger::warn("Failed to load configuration from: {}", config_.config_file);
            }
        }

        // Setup signal handling
        if (config_.daemonize) {
            // For daemon mode, use UNIX signal handlers since ASIO signal handling
            // doesn't work reliably after fork
            Logger::debug("Using UNIX signal handlers for daemon mode");
            setup_unix_signal_handlers();
        } else {
            // For normal mode, use ASIO signal handling
            setup_signal_handling();
        }

        // If we're daemonized, ensure signal handling works correctly after fork
        if (config_.daemonize) {
            Logger::debug("Signal handling configured for daemon process");
        }

        // Create work guard to keep IO context alive
        work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
            asio::make_work_guard(io_context_));

        // Call user initialization
        if (!on_initialize()) {
            Logger::error("User initialization failed");
            return false;
        }

        // Initialize components
        std::lock_guard<std::mutex> lock(components_mutex_);
        for (auto& component : components_) {
            Logger::debug("Initializing component: {}", component->name());
            if (!component->initialize(*this)) {
                Logger::error("Failed to initialize component: {}", component->name());
                return false;
            }
            Logger::debug("Component '{}' initialized successfully", component->name());
        }

        Logger::info("Application initialization completed");
        change_state(ApplicationState::Initialized);  // ✅ State change at END
        return true;

    } catch (const std::exception& e) {
        Logger::error("Exception during initialization: {}", e.what());
        handle_exception(e);
        return false;
    }
}

bool Application::start_internal() {
    Logger::debug("Starting application components");
    change_state(ApplicationState::Starting);  // ✅ Starting state is appropriate here

    try {
        // Start components
        {
            std::lock_guard<std::mutex> lock(components_mutex_);
            for (auto& component : components_) {
                Logger::debug("Starting component: {}", component->name());
                if (!component->start()) {
                    Logger::error("Failed to start component: {}", component->name());
                    return false;
                }
                Logger::debug("Component '{}' started successfully", component->name());
            }
        }

        // Call user startup
        if (!on_start()) {
            Logger::error("User startup failed");
            return false;
        }

        // Start health monitoring if enabled
        if (config_.enable_health_check) {
            start_health_monitoring();
        }

        // Start CLI if configured via ApplicationConfig
        if (config_.enable_cli) {
            CLIConfig cli_config{
                .enable = true,
                .bind_address = config_.cli_bind_address,
                .port = config_.cli_port,
                .enable_stdin = config_.cli_enable_stdin,
                .enable_tcp_server = config_.cli_enable_tcp
            };

            if (!enable_cli(cli_config)) {
                Logger::warn("Failed to start CLI as configured, but continuing startup");
            }
        }

        Logger::info("Application startup completed");
        return true;

    } catch (const std::exception& e) {
        Logger::error("Exception during startup: {}", e.what());
        handle_exception(e);
        return false;
    }
}

void Application::stop_internal() {
    Logger::debug("Stopping application");
    change_state(ApplicationState::Stopping);

    try {
        // Stop CLI first to prevent new commands
        disable_cli();

        // Stop health monitoring
        stop_health_monitoring();

        // Cancel all recurring tasks
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            for (auto& task : recurring_tasks_) {
                if (task.timer) {
                    task.timer->cancel();
                }
            }
            recurring_tasks_.clear();
        }

        // Call user shutdown
        on_stop();

        // Stop components in reverse order
        {
            std::lock_guard<std::mutex> lock(components_mutex_);
            for (auto it = components_.rbegin(); it != components_.rend(); ++it) {
                Logger::debug("Stopping component: {}", (*it)->name());
                try {
                    if (!(*it)->stop()) {
                        Logger::warn("Component '{}' reported stop failure", (*it)->name());
                    }
                } catch (const std::exception& e) {
                    Logger::error("Exception stopping component '{}': {}", (*it)->name(), e.what());
                }
                Logger::debug("Component '{}' stopped", (*it)->name());
            }
        }

        // Stop and cleanup managed threads using jthread's cooperative cancellation
        stop_all_managed_threads();

        // Give threads a moment to respond to stop tokens, then clear them
        // jthread destructors will automatically join when vector is cleared
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            std::lock_guard<std::mutex> lock(managed_threads_mutex_);
            managed_threads_.clear();  // jthreads auto-join in their destructors
        }

        // Stop worker threads
        stop_worker_threads();

        // Call user cleanup
        on_cleanup();

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
        // Remove PID file if we're daemonized
        if (config_.daemonize) {
            remove_pid_file();
        }
#endif

        change_state(ApplicationState::Stopped);
        Logger::info("Application shutdown completed");

    } catch (const std::exception& e) {
        Logger::error("Exception during shutdown: {}", e.what());
        handle_exception(e);
        change_state(ApplicationState::Failed);
    }
}

void Application::setup_signal_handling() {
    Logger::debug("Setting up signal handling");

    // Clear any existing signal handlers
    signals_.clear();

    for (int signal : config_.handled_signals) {
        signals_.add(signal);
        Logger::trace("Added signal handler for signal {}", signal);
    }

    signals_.async_wait([this](const asio::error_code& ec, int signal) {
        if (!ec) {
            handle_signal(signal);
            // Re-register for the next signal (only if not shutting down)
            if (state_.load() != ApplicationState::Stopping &&
                state_.load() != ApplicationState::Stopped) {
                setup_signal_handling();
            }
        } else if (ec != asio::error::operation_aborted) {
            Logger::debug("Signal wait error: {}", ec.message());
        }
    });
}

void Application::reset_signal_handling_after_fork() {
    Logger::debug("Resetting signal handling after fork");

    // Cancel existing signal handling
    signals_.cancel();

    // Re-initialize signal handling with the IO context
    // This is needed because fork() invalidates the signal handling setup
    setup_signal_handling();
}

void Application::handle_signal(int signal) {
    Logger::info("Received signal: {} ({})", signal, strsignal(signal));

    // Check for custom handler first
    auto it = signal_handlers_.find(signal);
    if (it != signal_handlers_.end()) {
        try {
            it->second(signal);
        } catch (const std::exception& e) {
            Logger::error("Exception in custom signal handler for signal {}: {}", signal, e.what());
        }
    }

    // Call virtual handler
    on_signal(signal);

    // Default handling for common signals
    switch (signal) {
        case SIGINT:
        case SIGTERM:
            Logger::info("Shutdown signal received, initiating graceful shutdown");
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
            // If we're a daemon, clean up PID file immediately upon shutdown signal
            if (config_.daemonize && !config_.daemon_pid_file.empty()) {
                Logger::debug("Cleaning up PID file due to shutdown signal");
                remove_pid_file();
            }
#endif
            shutdown();
            break;

        case SIGUSR1:
            Logger::info("USR1 signal received - performing health check");
            perform_health_check();
            break;

        case SIGUSR2:
            Logger::info("USR2 signal received - reloading configuration");
            // Try to reload configuration
            if (!config_.config_file.empty()) {
                auto& config_manager = ConfigManager::instance();
                if (config_manager.reload_config()) {
                    config_manager.configure_logger(config_.config_app_name);
                    Logger::info("Configuration reloaded successfully");
                } else {
                    Logger::warn("Failed to reload configuration");
                }
            }
            break;

        default:
            Logger::debug("Signal {} not handled by default handler", signal);
            break;
    }

    // Re-setup signal handling for next signal
    signals_.async_wait([this](const asio::error_code& ec, int signal) {
        if (!ec) {
            handle_signal(signal);
        }
    });
}

void Application::start_worker_threads() {
    for (std::size_t i = 0; i < config_.worker_threads; ++i) {
        worker_threads_.emplace_back([this, i](std::stop_token stop_token) {
            try {
                // Set up stop token callback to gracefully stop the IO context
                std::stop_callback stop_callback(stop_token, [this]() {
                    // This is THREAD-SAFE regardless of which thread calls it
                    io_context_.stop();
                });

                Logger::debug("Worker thread {} starting", i);

                // Pure event-driven execution: Block in run() until work_guard_ is released
                // The work_guard_ keeps the IO context alive, so run() will block waiting for events
                io_context_.run();

                Logger::debug("Worker thread {} stopping", i);

            } catch (const std::exception& e) {
                Logger::error("Exception in worker thread {}: {}", i, e.what());
                handle_exception(e);
            }
        });
    }
}

void Application::stop_worker_threads() {
    Logger::debug("Stopping worker threads");

    // Release work guard to allow IO context to stop naturally
    // This will cause io_context_.run() to return in all worker threads
    work_guard_.reset();

    // Also explicitly stop the IO context to wake up any blocking operations
    io_context_.stop();

    // Request stop for all worker threads (this triggers the stop_callback)
    for (auto& thread : worker_threads_) {
        thread.request_stop();
    }

    // Wait for all worker threads to finish (jthread automatically joins on destruction)
    worker_threads_.clear();

    Logger::debug("All worker threads stopped");
}

void Application::post_task(TaskFunction task, TaskPriority priority) {
    // Task posting with priority-based execution strategy
    switch (priority) {
        case TaskPriority::Critical:
            // Critical: Immediate execution when safe, otherwise high-priority queue
            // Use dispatch but with stack depth protection
            asio::dispatch(io_context_, [task = std::move(task)]() {
                try {
                    task();
                } catch (const std::exception& e) {
                    Logger::error("Exception in critical task: {}", e.what());
                } catch (...) {
                    Logger::error("Unknown exception in critical task");
                }
            });
            break;

        case TaskPriority::High:
            // High: Always use post to avoid stack depth issues, but prioritize
            // In a real implementation, you'd use a priority queue here
            asio::post(io_context_, [task = std::move(task)]() {
                try {
                    task();
                } catch (const std::exception& e) {
                    Logger::error("Exception in high-priority task: {}", e.what());
                } catch (...) {
                    Logger::error("Unknown exception in high-priority task");
                }
            });
            break;

        case TaskPriority::Normal:
        case TaskPriority::Low:
        default:
            // Normal/Low: Standard queued execution
            asio::post(io_context_, [task = std::move(task), priority]() {
                try {
                    task();
                } catch (const std::exception& e) {
                    Logger::error("Exception in task (priority {}): {}",
                                 static_cast<int>(priority), e.what());
                } catch (...) {
                    Logger::error("Unknown exception in task (priority {})",
                                 static_cast<int>(priority));
                }
            });
            break;
    }
}



void Application::post_delayed_task(TaskFunction task,
                                   std::chrono::milliseconds delay,
                                   TaskPriority priority) {
    auto timer = std::make_shared<asio::steady_timer>(io_context_, delay);
    timer->async_wait([task = std::move(task), priority, timer](const asio::error_code& ec) {
        if (!ec) {
            try {
                task();
            } catch (const std::exception& e) {
                Logger::error("Exception in delayed task (priority {}): {}",
                             static_cast<int>(priority), e.what());
            }
        }
    });
}

std::size_t Application::schedule_recurring_task(TaskFunction task,
                                                 std::chrono::milliseconds interval,
                                                 TaskPriority priority) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto task_id = next_task_id_++;
    auto timer = std::make_shared<asio::steady_timer>(io_context_);

    auto recurring_task = std::make_shared<RecurringTask>(RecurringTask{
        .id = task_id,
        .timer = timer,
        .function = std::move(task),
        .interval = interval,
        .priority = priority
    });

    recurring_tasks_.push_back(*recurring_task);

    // Start the first execution
    execute_recurring_task(recurring_task);

    Logger::debug("Scheduled recurring task {} with interval {}ms",
                  task_id, interval.count());

    return task_id;
}

void Application::cancel_recurring_task(std::size_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);

    auto it = std::find_if(recurring_tasks_.begin(), recurring_tasks_.end(),
                          [task_id](const RecurringTask& task) {
                              return task.id == task_id;
                          });

    if (it != recurring_tasks_.end()) {
        if (it->timer) {
            it->timer->cancel();
        }
        recurring_tasks_.erase(it);
        Logger::debug("Cancelled recurring task {}", task_id);
    } else {
        Logger::warn("Attempted to cancel non-existent task {}", task_id);
    }
}

void Application::execute_recurring_task(std::shared_ptr<RecurringTask> task) {
    task->timer->expires_after(task->interval);
    task->timer->async_wait([this, task](const asio::error_code& ec) {
        if (!ec && state_.load() == ApplicationState::Running) {
            try {
                task->function();
            } catch (const std::exception& e) {
                Logger::error("Exception in recurring task {} (priority {}): {}",
                             task->id, static_cast<int>(task->priority), e.what());
            }

            // Schedule next execution
            execute_recurring_task(task);
        }
    });
}

void Application::add_component(std::unique_ptr<ApplicationComponent> component) {
    std::lock_guard<std::mutex> lock(components_mutex_);

    Logger::debug("Adding component: {}", component->name());
    components_.push_back(std::move(component));
}

ApplicationComponent* Application::get_component(std::string_view name) const {
    std::lock_guard<std::mutex> lock(components_mutex_);

    auto it = std::find_if(components_.begin(), components_.end(),
                          [name](const auto& component) {
                              return component->name() == name;
                          });

    return it != components_.end() ? it->get() : nullptr;
}

void Application::set_signal_handler(int signal, SignalHandler handler) {
    signal_handlers_[signal] = std::move(handler);
    Logger::debug("Custom signal handler set for signal {}", signal);
}

void Application::set_error_handler(ErrorHandler handler) {
    error_handler_ = std::move(handler);
    Logger::debug("Custom error handler set");
}

bool Application::wait_for_stop(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(stop_mutex_);

    return stop_condition_.wait_for(lock, timeout, [this] {
        auto current_state = state_.load();
        return current_state == ApplicationState::Stopped ||
               current_state == ApplicationState::Failed;
    });
}

void Application::start_health_monitoring() {
    if (!config_.enable_health_check) {
        return;
    }

    Logger::debug("Starting health monitoring (interval: {}ms)",
                  config_.health_check_interval.count());

    health_timer_ = std::make_unique<asio::steady_timer>(io_context_);
    perform_health_check();
}

void Application::stop_health_monitoring() {
    if (health_timer_) {
        health_timer_->cancel();
        health_timer_.reset();
        Logger::debug("Health monitoring stopped");
    }
}

void Application::perform_health_check() {
    if (!health_timer_ || state_.load() != ApplicationState::Running) {
        return;
    }

    Logger::trace("Performing health check");

    try {
        bool all_healthy = true;

        // Check components
        {
            std::lock_guard<std::mutex> lock(components_mutex_);
            for (const auto& component : components_) {
                if (!component->health_check()) {
                    Logger::warn("Component '{}' failed health check", component->name());
                    all_healthy = false;
                }
            }
        }

        if (all_healthy) {
            Logger::trace("Health check passed");
        } else {
            Logger::warn("Health check failed - some components are unhealthy");
        }

    } catch (const std::exception& e) {
        Logger::error("Exception during health check: {}", e.what());
    }

    // Schedule next health check
    health_timer_->expires_after(config_.health_check_interval);
    health_timer_->async_wait([this](const asio::error_code& ec) {
        if (!ec) {
            perform_health_check();
        }
    });
}

void Application::change_state(ApplicationState new_state) {
    auto old_state = state_.exchange(new_state);

    if (old_state != new_state) {
        Logger::debug("Application state changed: {} -> {}",
                      static_cast<int>(old_state), static_cast<int>(new_state));
    }
}

void Application::handle_exception(const std::exception& e) {
    if (error_handler_) {
        try {
            error_handler_(e);
        } catch (const std::exception& handler_error) {
            Logger::critical("Exception in error handler: {}", handler_error.what());
        }
    }

    on_error(e);
}

void Application::on_signal(int signal) {
    // Default implementation - can be overridden by derived classes
    Logger::debug("Default signal handler for signal {}", signal);

    // Handle SIGHUP for config reload
    if (signal == SIGHUP) {
        Logger::info("Received SIGHUP signal - reloading configuration");
        if (!reload_config()) {
            Logger::warn("Failed to reload configuration");
        }
    }
}

bool Application::reload_config() {
    Logger::info("Reloading application configuration...");

    // Default implementation: reload logger configuration
    bool success = Logger::reload_config(config_.name);

    if (success) {
        Logger::info("Configuration reloaded successfully");
    } else {
        Logger::warn("Failed to reload configuration");
    }

    // Allow derived classes to handle additional config reload logic
    bool custom_success = on_config_reload();

    return success && custom_success;
}

bool Application::on_config_reload() {
    // Default implementation - can be overridden by derived classes
    Logger::debug("Default config reload handler");
    return true;
}

void Application::on_error(const std::exception& error) {
    // Default implementation - can be overridden by derived classes
    Logger::error("Default error handler: {}", error.what());
}

// ManagedThread Implementation (Simplified with std::jthread)

Application::ManagedThread::ManagedThread(std::string name, ThreadFunction func)
    : name_(std::move(name))
    , func_(std::move(func))
    , work_guard_(std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(io_context_.get_executor()))
    , messaging_context_(std::make_shared<ThreadMessagingContext>(name_, io_context_))
    , thread_([this](std::stop_token stop_token) { thread_main(stop_token); })
{
    Logger::info("Created managed thread: {}", name_);
}

void Application::ManagedThread::thread_main(std::stop_token stop_token) {
    Logger::info("Managed thread '{}' starting", name_);

    try {
        // Start messaging context (register with global bus)
        messaging_context_->start();

        // Set up stop token handler to integrate with ASIO event system
        std::stop_callback stop_callback(stop_token, [this]() {
            work_guard_.reset();  // Release work guard to allow io_context to stop
            io_context_.stop();
        });

        // Run user function if provided
        if (func_) {
            asio::post(io_context_, [this]() {
                try {
                    func_(*this);  // Simplified: only pass ManagedThread reference
                } catch (const std::exception& e) {
                    Logger::error("Exception in thread function '{}': {}", name_, e.what());
                }
            });
        }

        // Run the event loop until stop is requested
        io_context_.run();

    } catch (const std::exception& e) {
        Logger::error("Exception in managed thread '{}': {}", name_, e.what());
    }

    // Clean up
    messaging_context_->stop();
    Logger::info("Managed thread '{}' stopped", name_);
}

void Application::ManagedThread::post_task(std::function<void()> task) {
    asio::post(io_context_, [task = std::move(task)]() {
        try {
            task();
        } catch (const std::exception& e) {
            Logger::error("Exception in managed thread task: {}", e.what());
        } catch (...) {
            Logger::error("Unknown exception in managed thread task");
        }
    });
}

// Application Thread Management Methods

std::shared_ptr<ManagedThreadBase>
Application::create_thread(std::string name, std::function<void(ManagedThreadBase&)> thread_func) {
    // Convert the base interface function to the concrete ManagedThread function
    Application::ManagedThread::ThreadFunction concrete_func = nullptr;
    if (thread_func) {
        concrete_func = [thread_func](Application::ManagedThread& thread) {
            thread_func(static_cast<ManagedThreadBase&>(thread));
        };
    }

    auto managed_thread = std::make_shared<ManagedThread>(name, std::move(concrete_func));

    {
        std::lock_guard<std::mutex> lock(managed_threads_mutex_);
        managed_threads_.push_back(managed_thread);
    }

    return managed_thread;
}

std::size_t Application::managed_thread_count() const {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);
    return managed_threads_.size();
}

void Application::stop_all_managed_threads() {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);

    // Request cooperative stop using jthread's stop token
    for (auto& thread : managed_threads_) {
        if (thread) {
            thread->request_stop();
        }
    }

    // jthread destructors will automatically join when managed_threads_ is cleared
}

bool Application::any_managed_thread_stop_requested() const {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);

    return std::any_of(managed_threads_.begin(), managed_threads_.end(),
                      [](const auto& thread) {
                          return thread && thread->get_stop_token().stop_requested();
                      });
}

Application::ManagedThread* Application::get_managed_thread(const std::string& name) const {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);

    auto it = std::find_if(managed_threads_.begin(), managed_threads_.end(),
                          [&name](const auto& thread) {
                              return thread && thread->name() == name;
                          });

    return it != managed_threads_.end() ? it->get() : nullptr;
}

// CLI Management

CLI& Application::cli() const {
    return CLI::instance();
}

bool Application::enable_cli(const CLIConfig& config) {
    if (cli_enabled_) {
        Logger::warn("CLI already enabled");
        return true;
    }

    Logger::info("Enabling CLI");
    auto& cli_instance = CLI::instance();
    cli_instance.configure(config);

    if (cli_instance.start(*const_cast<Application*>(this))) {
        cli_enabled_ = true;
        Logger::info("CLI enabled successfully");
        return true;
    } else {
        Logger::error("Failed to start CLI");
        return false;
    }
}

bool Application::enable_cli() {
    CLIConfig default_config{};
    return enable_cli(default_config);
}

void Application::disable_cli() {
    if (!cli_enabled_) {
        return;
    }

    Logger::info("Disabling CLI");
    CLI::instance().stop();
    cli_enabled_ = false;
    Logger::info("CLI disabled");
}

bool Application::is_cli_enabled() const {
    return cli_enabled_ && CLI::instance().is_running();
}

// ============================================================================
// Daemonization Implementation
// ============================================================================

bool Application::daemonize() {
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    if (!config_.daemonize) {
        Logger::debug("Daemonization not requested");
        return true;
    }

    Logger::info("Starting daemonization process");
    return daemonize_unix();
#else
    if (config_.daemonize) {
        Logger::error("Daemonization not supported on this platform");
        return false;
    }
    return true;
#endif
}

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
bool Application::daemonize_unix() {
    // Step 1: Fork first child and exit parent
    pid_t pid = fork();
    if (pid < 0) {
        Logger::error("First fork failed: {}", std::strerror(errno));
        return false;
    }

    if (pid > 0) {
        // Parent process - exit
        Logger::info("Parent process exiting (child PID: {})", pid);
        std::exit(EXIT_SUCCESS);
    }

    // Step 2: Become session leader
    if (setsid() < 0) {
        Logger::error("setsid failed: {}", std::strerror(errno));
        return false;
    }

    // Step 3: Fork second child and exit first child
    // This ensures the daemon cannot acquire a controlling terminal
    pid = fork();
    if (pid < 0) {
        Logger::error("Second fork failed: {}", std::strerror(errno));
        return false;
    }

    if (pid > 0) {
        // First child - exit
        std::exit(EXIT_SUCCESS);
    }

    // Step 3.5: Configure logger for daemon mode before redirecting FDs
    if (!config_.daemon_log_file.empty()) {
        LoggerConfig daemon_logger_config{
            .app_name = config_.name,
            .log_file = std::filesystem::path{config_.daemon_log_file},
            .level = LogLevel::Debug,
            .enable_console = false,  // Disable console output in daemon mode
            .enable_file = true
        };
        Logger::init(daemon_logger_config);
        Logger::info("Logger reconfigured for daemon mode to: {}", config_.daemon_log_file);
        Logger::flush();  // Ensure the configuration message is written
    }

    // Step 4: Change working directory
    if (!config_.daemon_work_dir.empty()) {
        if (chdir(config_.daemon_work_dir.c_str()) < 0) {
            Logger::error("Failed to change working directory to '{}': {}",
                         config_.daemon_work_dir, std::strerror(errno));
            return false;
        }
        Logger::debug("Changed working directory to '{}'", config_.daemon_work_dir);
    }

    // Step 5: Set file creation mask
    umask(config_.daemon_umask);
    Logger::debug("Set umask to {:o}", config_.daemon_umask);

    // Step 6: Close all file descriptors
    if (config_.daemon_close_fds) {
        close_all_fds();
    }

    // Step 7: Redirect standard file descriptors
    if (!redirect_standard_fds()) {
        Logger::error("Failed to redirect standard file descriptors");
        return false;
    }

    // Step 8: Drop privileges if requested
    if (!drop_privileges()) {
        Logger::error("Failed to drop privileges");
        return false;
    }

    // Step 9: Create PID file
    if (!create_pid_file()) {
        Logger::error("Failed to create PID file");
        return false;
    }

    // Step 10: Set up UNIX signal handlers for daemon mode
    // ASIO signal handling doesn't work reliably after fork, so use traditional UNIX signals
    Logger::debug("Setting up UNIX signal handlers for daemon process");
    setup_unix_signal_handlers();

    Logger::info("Daemonization completed successfully (PID: {})", getpid());
    return true;
}

bool Application::drop_privileges() {
    // Drop group privileges first
    if (!config_.daemon_group.empty()) {
        struct group* grp = getgrnam(config_.daemon_group.c_str());
        if (!grp) {
            Logger::error("Group '{}' not found", config_.daemon_group);
            return false;
        }

        if (setgid(grp->gr_gid) < 0) {
            Logger::error("Failed to set group ID to {}: {}",
                         grp->gr_gid, std::strerror(errno));
            return false;
        }
        Logger::debug("Set group to '{}' (GID: {})", config_.daemon_group, grp->gr_gid);
    }

    // Drop user privileges
    if (!config_.daemon_user.empty()) {
        struct passwd* pwd = getpwnam(config_.daemon_user.c_str());
        if (!pwd) {
            Logger::error("User '{}' not found", config_.daemon_user);
            return false;
        }

        if (setuid(pwd->pw_uid) < 0) {
            Logger::error("Failed to set user ID to {}: {}",
                         pwd->pw_uid, std::strerror(errno));
            return false;
        }
        Logger::debug("Set user to '{}' (UID: {})", config_.daemon_user, pwd->pw_uid);
    }

    return true;
}

bool Application::create_pid_file() {
    if (config_.daemon_pid_file.empty()) {
        return true;  // No PID file requested
    }

    std::ofstream pid_file(config_.daemon_pid_file);
    if (!pid_file) {
        Logger::error("Failed to create PID file '{}'", config_.daemon_pid_file);
        return false;
    }

    pid_file << getpid() << std::endl;
    if (!pid_file) {
        Logger::error("Failed to write to PID file '{}'", config_.daemon_pid_file);
        return false;
    }

    Logger::debug("Created PID file '{}' with PID {}", config_.daemon_pid_file, getpid());
    return true;
}

void Application::remove_pid_file() {
    if (!config_.daemon_pid_file.empty()) {
        if (unlink(config_.daemon_pid_file.c_str()) < 0) {
            Logger::warn("Failed to remove PID file '{}': {}",
                        config_.daemon_pid_file, std::strerror(errno));
        } else {
            Logger::debug("Removed PID file '{}'", config_.daemon_pid_file);
        }
    }
}

bool Application::redirect_standard_fds() {
    // Redirect stdin to /dev/null
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
        Logger::error("Failed to open /dev/null for reading: {}", std::strerror(errno));
        return false;
    }

    if (dup2(fd, STDIN_FILENO) < 0) {
        Logger::error("Failed to redirect stdin: {}", std::strerror(errno));
        close(fd);
        return false;
    }
    close(fd);

    // Redirect stdout and stderr
    const char* output_target = "/dev/null";
    if (!config_.daemon_log_file.empty()) {
        output_target = config_.daemon_log_file.c_str();
    }

    fd = open(output_target, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        Logger::error("Failed to open '{}' for writing: {}", output_target, std::strerror(errno));
        return false;
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        Logger::error("Failed to redirect stdout: {}", std::strerror(errno));
        close(fd);
        return false;
    }

    if (dup2(fd, STDERR_FILENO) < 0) {
        Logger::error("Failed to redirect stderr: {}", std::strerror(errno));
        close(fd);
        return false;
    }

    close(fd);

    if (!config_.daemon_log_file.empty()) {
        Logger::debug("Redirected stdout/stderr to '{}'", config_.daemon_log_file);
    } else {
        Logger::debug("Redirected stdout/stderr to /dev/null");
    }

    return true;
}

void Application::close_all_fds() {
    // Get maximum number of file descriptors
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        // If we can't get the limit, use a reasonable default
        rl.rlim_max = 1024;
    }

    // Close all file descriptors except stdin, stdout, stderr
    // We'll redirect these later
    for (int fd = 3; fd < static_cast<int>(rl.rlim_max); ++fd) {
        close(fd);  // Ignore errors - the FD might not be open
    }

    Logger::debug("Closed all file descriptors from 3 to {}", rl.rlim_max - 1);
}

// Static instance for signal handling
Application* Application::signal_instance_ = nullptr;

void Application::signal_handler_wrapper(int signal) {
    if (signal_instance_) {
        signal_instance_->handle_signal(signal);
    }
}

void Application::setup_unix_signal_handlers() {
    Logger::debug("Setting up UNIX signal handlers");

    // Set the static instance for signal handling
    signal_instance_ = this;

    // Set up signal handlers for daemon mode
    for (int sig : config_.handled_signals) {
        if (signal(sig, signal_handler_wrapper) == SIG_ERR) {
            Logger::warn("Failed to set signal handler for signal {}: {}", sig, std::strerror(errno));
        } else {
            Logger::debug("Set UNIX signal handler for signal {}", sig);
        }
    }
}
#endif

// ============================================================================
// ThreadedComponent Implementation
// ============================================================================

ThreadedComponent::ThreadedComponent(std::string name)
    : name_(std::move(name)) {
    Logger::debug("ThreadedComponent '{}' created", name_);
}

ThreadedComponent::~ThreadedComponent() {
    if (running_.load()) {
        Logger::warn("ThreadedComponent '{}' destroyed while running - forcing stop", name_);
        stop();
    }
    Logger::debug("ThreadedComponent '{}' destroyed", name_);
}

bool ThreadedComponent::initialize(ThreadFactory& thread_factory) {
    if (thread_factory_) {
        Logger::warn("ThreadedComponent '{}' already initialized", name_);
        return true;
    }

    // Store the thread factory reference
    thread_factory_ = &thread_factory;

    try {
        if (!on_initialize()) {
            Logger::error("ThreadedComponent '{}' on_initialize() failed", name_);
            return false;
        }

        Logger::info("ThreadedComponent '{}' initialized successfully", name_);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Exception during ThreadedComponent '{}' initialization: {}", name_, e.what());
        return false;
    }
}

bool ThreadedComponent::start() {
    if (!thread_factory_) {
        Logger::error("ThreadedComponent '{}' not initialized - call initialize() first", name_);
        return false;
    }

    if (running_.load()) {
        Logger::warn("ThreadedComponent '{}' already running", name_);
        return true;
    }

    try {
        Logger::info("Starting ThreadedComponent '{}'", name_);

        // Create managed thread with our thread main function
        managed_thread_ = thread_factory_->create_thread(name_,
            [this](ManagedThreadBase& thread) {
                auto concrete_thread = static_cast<Application::ManagedThread*>(&thread);
                thread_main(*concrete_thread);
            });

        if (!managed_thread_) {
            Logger::error("Failed to create managed thread for ThreadedComponent '{}'", name_);
            return false;
        }

        running_.store(true);
        Logger::info("ThreadedComponent '{}' started successfully", name_);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Exception starting ThreadedComponent '{}': {}", name_, e.what());
        return false;
    }
}

bool ThreadedComponent::stop() {
    if (!running_.load()) {
        return true; // Already stopped
    }

    Logger::info("Stopping ThreadedComponent '{}'", name_);

    try {
        // Cancel all timers
        for (auto& [timer_id, timer] : timers_) {
            timer->cancel();
        }
        timers_.clear();

        // Request thread stop
        if (managed_thread_) {
            managed_thread_->request_stop();
        }

        running_.store(false);

        // Reset managed thread (jthread destructor will join automatically)
        managed_thread_.reset();

        Logger::info("ThreadedComponent '{}' stopped successfully", name_);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Exception stopping ThreadedComponent '{}': {}", name_, e.what());
        running_.store(false);
        return false;
    }
}

void ThreadedComponent::thread_main(Application::ManagedThread& thread) {
    Logger::info("ThreadedComponent '{}' thread starting", name_);

    try {
        // Apply any pending message subscriptions
        apply_pending_subscriptions();

        // Register a stop callback using a shared pointer for proper lifetime management
        auto stop_callback = std::make_shared<std::stop_callback<std::function<void()>>>(
            thread.get_stop_token(), [this]() {
                // This runs when stop is requested
                try {
                    on_stop();
                } catch (const std::exception& e) {
                    Logger::error("Exception in ThreadedComponent '{}' on_stop(): {}", name_, e.what());
                }
                Logger::info("ThreadedComponent '{}' thread stopped", name_);
            }
        );

        // Call user startup logic
        if (!on_start()) {
            Logger::error("ThreadedComponent '{}' on_start() failed", name_);
            return;
        }

        Logger::debug("ThreadedComponent '{}' thread main setup complete", name_);

        // Keep the stop callback alive by capturing it in a task
        // This ensures the callback stays registered for the lifetime of the thread
        thread.post_task([stop_callback]() {
            // Just hold onto the callback - it will be called when stop is requested
            // The callback will be destroyed when the thread stops
        });

    } catch (const std::exception& e) {
        Logger::error("Exception in ThreadedComponent '{}' thread: {}", name_, e.what());
    }
}

void ThreadedComponent::apply_pending_subscriptions() {
    for (auto& subscription : pending_subscriptions_) {
        try {
            subscription();
        } catch (const std::exception& e) {
            Logger::error("Exception applying pending subscription for '{}': {}", name_, e.what());
        }
    }
    pending_subscriptions_.clear();
}

} // namespace base
