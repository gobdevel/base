/*
 * @file application.cpp
 * @brief Modern C++20 application framework implementation
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"

#include <algorithm>
#include <csignal>
#include <future>
#include <iomanip>
#include <iostream>

namespace crux {

Application::Application(ApplicationConfig config)
    : config_(std::move(config))
    , signals_(io_context_)
{
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
    Logger::info("Application '{}' destroyed", config_.name);
}

int Application::run() {
    Logger::info("Starting application '{}' v{}", config_.name, config_.version);
    Logger::info("Description: {}", config_.description);

    try {
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
        setup_signal_handling();

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

    for (int signal : config_.handled_signals) {
        signals_.add(signal);
        Logger::trace("Added signal handler for signal {}", signal);
    }

    signals_.async_wait([this](const asio::error_code& ec, int signal) {
        if (!ec) {
            handle_signal(signal);
        }
    });
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
    asio::post(io_context_, [task = std::move(task), priority]() {
        try {
            task();
        } catch (const std::exception& e) {
            Logger::error("Exception in posted task (priority {}): {}",
                         static_cast<int>(priority), e.what());
        }
    });
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
{
    Logger::debug("Creating managed thread: {}", name_);

    // Create work guard to keep IO context alive
    work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
        asio::make_work_guard(io_context_));

    // Start the thread
    running_.store(true);
    thread_ = std::thread([this]() { run(); });

    Logger::debug("Managed thread '{}' started", name_);
}

Application::ManagedThread::~ManagedThread() {
    if (running_.load()) {
        Logger::debug("Stopping managed thread '{}' in destructor", name_);
        stop();
        join();
    }
    Logger::debug("Managed thread '{}' destroyed", name_);
}

void Application::ManagedThread::run() {
    Logger::trace("Managed thread '{}' event loop starting", name_);

    try {
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

void Application::ManagedThread::post_task(std::function<void()> task) {
    asio::post(io_context_, [task = std::move(task)]() {
        try {
            task();
        } catch (const std::exception& e) {
            Logger::error("Exception in managed thread task: {}", e.what());
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
    return create_thread(std::move(name), [](asio::io_context& io_ctx) {
        // Default worker thread just runs the event loop
        // Tasks can be posted to it using post_task()
    });
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

} // namespace crux
