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
    Logger::debug("Worker threads: {}, IO thread: {}",
                  config_.worker_threads,
                  config_.use_dedicated_io_thread ? "dedicated" : "shared");
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

        Logger::info("Application '{}' started successfully", config_.name);
        change_state(ApplicationState::Running);

        // Run the event loop
        Logger::debug("Starting event loop with {} worker threads", config_.worker_threads);
        start_worker_threads();

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

    // Stop the IO context immediately
    io_context_.stop();

    // Wake up the main thread
    stop_condition_.notify_all();
}

bool Application::initialize_internal() {
    Logger::debug("Initializing application components");
    change_state(ApplicationState::Initialized);

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
        return true;

    } catch (const std::exception& e) {
        Logger::error("Exception during initialization: {}", e.what());
        handle_exception(e);
        return false;
    }
}

bool Application::start_internal() {
    Logger::debug("Starting application components");
    change_state(ApplicationState::Starting);

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

        // Stop and cleanup managed threads
        stop_all_managed_threads();
        join_all_managed_threads();

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
    Logger::debug("Starting {} worker threads", config_.worker_threads);

    for (std::size_t i = 0; i < config_.worker_threads; ++i) {
        worker_threads_.emplace_back([this, i]() {
            Logger::trace("Worker thread {} started", i);

            try {
                io_context_.run();
            } catch (const std::exception& e) {
                Logger::error("Exception in worker thread {}: {}", i, e.what());
                handle_exception(e);
            }

            Logger::trace("Worker thread {} stopped", i);
        });
    }
}

void Application::stop_worker_threads() {
    Logger::debug("Stopping worker threads");

    // Release work guard to allow IO context to stop
    work_guard_.reset();

    // Stop the IO context
    io_context_.stop();

    // Wait for all worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

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
}

void Application::on_error(const std::exception& error) {
    // Default implementation - can be overridden by derived classes
    Logger::error("Default error handler: {}", error.what());
}

// ManagedThread Implementation

Application::ManagedThread::ManagedThread(std::string name, ThreadFunction func)
    : name_(std::move(name))
    , user_function_(std::move(func))
    , messaging_context_(std::make_shared<ThreadMessagingContext>(name_))
{
    Logger::debug("Creating managed thread: {}", name_);

    // Register with messaging bus
    MessagingBus::instance().register_thread(name_, messaging_context_);

    // Create work guard to keep IO context alive
    work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
        asio::make_work_guard(io_context_));

    // Start the thread
    running_.store(true);
    thread_ = std::thread([this]() { run(); });

    Logger::info("Created managed thread: {}", name_);
}

Application::ManagedThread::~ManagedThread() {
    if (running_.load()) {
        Logger::debug("Stopping managed thread '{}' in destructor", name_);
        stop();
        join();
    }

    // Unregister from messaging bus
    MessagingBus::instance().unregister_thread(name_);

    Logger::debug("Managed thread '{}' destroyed", name_);
}

void Application::ManagedThread::run() {
    Logger::trace("Managed thread '{}' event loop starting", name_);

    try {
        // Set up periodic message processing
        if (messaging_context_) {
            message_timer_ = std::make_shared<asio::steady_timer>(io_context_);
            schedule_message_processing();
        }

        // Call user function with the IO context
        if (user_function_) {
            user_function_(io_context_);
        }

        // Run the event loop
        io_context_.run();

    } catch (const std::exception& e) {
        Logger::error("Exception in managed thread '{}': {}", name_, e.what());
    }

    running_.store(false);
    Logger::trace("Managed thread '{}' event loop stopped", name_);
}

void Application::ManagedThread::schedule_message_processing() {
    if (!running_.load() || !messaging_context_ || !message_timer_) {
        return;
    }

    // Process messages in high-performance batch mode
    messaging_context_->process_messages_batch();

    // Use configurable interval for message processing
    // Default: 1ms for high-throughput, low-latency scenarios
    message_timer_->expires_after(std::chrono::microseconds(1000));
    message_timer_->async_wait([this](const asio::error_code& ec) {
        if (!ec && running_.load()) {
            schedule_message_processing();
        }
    });
}

void Application::ManagedThread::post_task(std::function<void()> task) {
    // Consistent with main application post_task behavior
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

void Application::ManagedThread::stop() {
    if (running_.load()) {
        Logger::debug("Stopping managed thread '{}'", name_);

        // Release work guard to allow IO context to stop
        work_guard_.reset();

        // Stop the IO context
        io_context_.stop();

        running_.store(false);
    }
}

void Application::ManagedThread::join() {
    if (thread_.joinable()) {
        Logger::trace("Waiting for managed thread '{}' to finish", name_);
        thread_.join();
        Logger::trace("Managed thread '{}' finished", name_);
    }
}

// Event-Driven Managed Thread Implementation

Application::EventDrivenManagedThread::EventDrivenManagedThread(std::string name, ThreadFunction func)
    : name_(std::move(name)), func_(std::move(func)) {

    Logger::debug("Creating event-driven managed thread '{}'", name_);

    // Create messaging context
    messaging_context_ = std::make_unique<ThreadMessagingContext>(name_);

    // Start the thread
    running_.store(true);
    thread_ = std::thread([this]() { thread_main(); });

    Logger::debug("Event-driven managed thread '{}' created and started", name_);
}

Application::EventDrivenManagedThread::~EventDrivenManagedThread() {
    stop();
    join();
    Logger::debug("Event-driven managed thread '{}' destroyed", name_);
}

void Application::EventDrivenManagedThread::thread_main() {
    Logger::info("Event-driven managed thread '{}' starting", name_);

    try {
        // Set up work guard to keep IO context alive
        work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(io_context_.get_executor());

        // Start background message processing
        messaging_context_->start_background_processing();

        // Run user function if provided
        if (func_) {
            asio::post(io_context_, [this]() {
                try {
                    func_(*this);
                } catch (const std::exception& e) {
                    Logger::error("Exception in event-driven thread function '{}': {}", name_, e.what());
                }
            });
        }

        // Run IO context (event loop)
        io_context_.run();

    } catch (const std::exception& e) {
        Logger::error("Exception in event-driven managed thread '{}': {}", name_, e.what());
    }

    Logger::info("Event-driven managed thread '{}' stopped", name_);
}

void Application::EventDrivenManagedThread::post_task(std::function<void()> task) {
    // Consistent with main application post_task behavior
    asio::post(io_context_, [task = std::move(task)]() {
        try {
            task();
        } catch (const std::exception& e) {
            Logger::error("Exception in event-driven managed thread task: {}", e.what());
        } catch (...) {
            Logger::error("Unknown exception in event-driven managed thread task");
        }
    });
}

void Application::EventDrivenManagedThread::stop() {
    if (running_.load()) {
        Logger::debug("Stopping event-driven managed thread '{}'", name_);

        // Stop messaging context
        if (messaging_context_) {
            messaging_context_->stop();
        }

        // Release work guard to allow IO context to stop
        work_guard_.reset();

        // Stop the IO context
        io_context_.stop();

        running_.store(false);
    }
}

void Application::EventDrivenManagedThread::join() {
    if (thread_.joinable()) {
        Logger::trace("Waiting for event-driven managed thread '{}' to finish", name_);
        thread_.join();
        Logger::trace("Event-driven managed thread '{}' finished", name_);
    }
}

// Application Thread Management Methods

std::shared_ptr<Application::ManagedThread>
Application::create_thread(std::string name, ManagedThread::ThreadFunction thread_func) {
    auto managed_thread = std::make_shared<ManagedThread>(name, std::move(thread_func));

    {
        std::lock_guard<std::mutex> lock(managed_threads_mutex_);
        managed_threads_.push_back(managed_thread);
    }

    Logger::info("Created managed thread: {}", name);
    return managed_thread;
}

std::shared_ptr<Application::ManagedThread>
Application::create_worker_thread(std::string name) {
    // Convenience method - equivalent to create_thread with empty function
    return create_thread(std::move(name), [](asio::io_context& io_ctx) {
        // Default worker thread just runs the event loop
        // Tasks can be posted to it using post_task()
    });
}

std::shared_ptr<Application::EventDrivenManagedThread>
Application::create_event_driven_thread(std::string name) {
    Logger::debug("Creating event-driven thread '{}'", name);

    auto thread = std::make_shared<EventDrivenManagedThread>(std::move(name));

    // Store reference to manage lifecycle (optional - or manage separately)
    // For now, caller manages the thread lifecycle

    Logger::info("Event-driven thread '{}' created successfully", thread->name());
    return thread;
}

std::size_t Application::managed_thread_count() const {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);
    return managed_threads_.size();
}

void Application::stop_all_managed_threads() {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);

    Logger::debug("Stopping {} managed threads", managed_threads_.size());

    for (auto& thread : managed_threads_) {
        if (thread) {
            thread->stop();
        }
    }
}

void Application::join_all_managed_threads() {
    std::lock_guard<std::mutex> lock(managed_threads_mutex_);

    Logger::debug("Waiting for {} managed threads to finish", managed_threads_.size());

    for (auto& thread : managed_threads_) {
        if (thread) {
            thread->join();
        }
    }

    managed_threads_.clear();
    Logger::debug("All managed threads finished and cleaned up");
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

} // namespace base
