/*
 * @file application.h
 * @brief Modern C++20 application framework with standalone ASIO event loop
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "logger.h"
#include "config.h"
#include "cli.h"
#include "thread_messaging.h"

#include <asio.hpp>
#include <asio/signal_set.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <future>
#include <exception>
#include <stop_token>
#include <unordered_map>

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#endif

namespace base {

// Forward declarations
struct CLIConfig;
enum class ApplicationState;

/**
 * @brief Application lifecycle states
 */
enum class ApplicationState {
    Created,     ///< Application created but not initialized
    Initialized, ///< Application initialized but not started
    Starting,    ///< Application is starting up
    Running,     ///< Application is running normally
    Stopping,    ///< Application is shutting down
    Stopped,     ///< Application has stopped
    Failed       ///< Application failed to start or encountered fatal error
};

/**
 * @brief Application configuration structure
 */
struct ApplicationConfig {
    std::string name = "base_app";
    std::string version = "1.0.0";
    std::string description = "Base Application";

    // Threading configuration
    std::size_t worker_threads = std::jthread::hardware_concurrency();

    // Signal handling
    std::vector<int> handled_signals = {SIGINT, SIGTERM, SIGHUP, SIGUSR1, SIGUSR2};

    // Health check
    bool enable_health_check = true;
    std::chrono::milliseconds health_check_interval = std::chrono::milliseconds(5000);

    // Configuration file
    std::string config_file = "";  // No default config file
    std::string config_app_name = "default";

    // Daemonization options
    bool daemonize = false;                    ///< Run as daemon process
    std::string daemon_work_dir = "/";         ///< Working directory for daemon
    std::string daemon_user = "";             ///< User to run daemon as (empty = current user)
    std::string daemon_group = "";            ///< Group to run daemon as (empty = current group)
    std::string daemon_pid_file = "";         ///< PID file path (empty = no PID file)
    std::string daemon_log_file = "";         ///< Log file path for daemon (empty = use logger config)
    mode_t daemon_umask = 022;                ///< File creation mask for daemon
    bool daemon_close_fds = true;             ///< Close all file descriptors when daemonizing

    // CLI configuration
    bool enable_cli = false;
    bool cli_enable_stdin = true;
    bool cli_enable_tcp = false;
    std::string cli_bind_address = "127.0.0.1";
    std::uint16_t cli_port = 8080;

    // Command line argument handling
    bool parse_command_line = true;          ///< Enable command line argument parsing
    bool show_help_and_exit = false;        ///< Show help and exit (set by --help)
    bool show_version_and_exit = false;     ///< Show version and exit (set by --version)
    std::string custom_config_file = "";    ///< Config file from command line (--config)
    std::string custom_log_level = "";      ///< Log level from command line (--log-level)
    std::string custom_log_file = "";       ///< Log file from command line (--log-file)
    bool force_foreground = false;          ///< Force foreground mode (--no-daemon)
};

/**
 * @brief Task execution priority levels for scheduling optimization
 *
 * Priority determines WHEN a task executes, not exception handling strategy.
 * All priorities maintain full exception safety for application reliability.
 */
enum class TaskPriority {
    Low = 0,        ///< Low priority: Queued execution, explicit low importance
    Normal = 1,     ///< Normal priority: Queued execution, default for most tasks
    High = 2,       ///< High priority: Immediate dispatch, time-sensitive operations
    Critical = 3    ///< Critical priority: Immediate dispatch, highest urgency
};

/**
 * @brief Forward declarations
 */
class Application;
class CLI;

/**
 * @brief Forward declarations
 */
class Application;
class CLI;
class ManagedThreadBase; // Forward declare the base class

/**
 * @brief Base interface for managed threads
 */
class ManagedThreadBase {
public:
    virtual ~ManagedThreadBase() = default;

    // Essential interface for thread factory
    virtual const std::string& name() const noexcept = 0;
    virtual bool stop_requested() const noexcept = 0;
    virtual void request_stop() noexcept = 0;
    virtual void post_task(std::function<void()> task) = 0;
};

/**
 * @brief Interface for creating managed threads - enables dependency injection
 */
class ThreadFactory {
public:
    virtual ~ThreadFactory() = default;

    /**
     * @brief Create a managed thread
     * @param name Thread name for logging/debugging
     * @param thread_func Function to run in the thread
     * @return Shared pointer to created thread (actual type will be Application::ManagedThread)
     */
    virtual std::shared_ptr<ManagedThreadBase> create_thread(
        std::string name,
        std::function<void(ManagedThreadBase&)> thread_func = nullptr) = 0;
};

/**
 * @brief Application component interface for modular components
 */
class ApplicationComponent {
public:
    virtual ~ApplicationComponent() = default;

    /**
     * @brief Initialize the component
     * @param app Reference to the application
     * @return true if initialization successful
     */
    virtual bool initialize(Application& app) = 0;

    /**
     * @brief Start the component
     * @return true if start successful
     */
    virtual bool start() = 0;

    /**
     * @brief Stop the component
     * @return true if stop successful
     */
    virtual bool stop() = 0;

    /**
     * @brief Get component name for logging
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Health check for the component
     * @return true if component is healthy
     */
    virtual bool health_check() const { return true; }
};

/**
 * @brief Modern C++20 application framework with event-driven architecture
 *
 * Features:
 * - Standalone ASIO event loop with configurable thread pool
 * - Signal handling for graceful shutdown
 * - Component-based architecture
 * - Health monitoring
 * - Configuration integration
 * - Comprehensive logging
 * - Exception handling and recovery
 *
 * Usage:
 * @code
 * class MyApp : public Application {
 * protected:
 *     bool on_initialize() override {
 *         // Custom initialization
 *         return true;
 *     }
 *
 *     bool on_start() override {
 *         // Start application services
 *         return true;
 *     }
 * };
 *
 * int main() {
 *     MyApp app;
 *     return app.run();
 * }
 * @endcode
 */
class Application : public SingletonBase<Application>, public ThreadFactory {
public:
    using SignalHandler = std::function<void(int signal)>;
    using TaskFunction = std::function<void()>;
    using ErrorHandler = std::function<void(const std::exception&)>;

    /**
     * @brief Construct application with configuration
     */
    explicit Application(ApplicationConfig config = {});

    /**
     * @brief Virtual destructor
     */
    virtual ~Application();

    // Non-copyable, non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * @brief Initialize and run the application
     * @return Exit code (0 for success)
     */
    int run();

    /**
     * @brief Initialize and run the application with command line arguments
     * @param argc Argument count
     * @param argv Argument vector
     * @return Exit code (0 for success)
     */
    int run(int argc, char* argv[]);

    /**
     * @brief Parse command line arguments
     * @param argc Argument count
     * @param argv Argument vector
     * @return true if parsing successful and application should continue
     */
    bool parse_command_line_args(int argc, char* argv[]);

    /**
     * @brief Show application help and usage information
     */
    void show_help(const char* program_name) const;

    /**
     * @brief Show application version information
     */
    void show_version() const;

    /**
     * @brief Daemonize the application (UNIX only)
     * @return true if daemonization successful, false otherwise
     * @note This must be called before run() and only works on UNIX-like systems
     */
    bool daemonize();

    /**
     * @brief Request graceful shutdown
     */
    void shutdown();

    /**
     * @brief Force immediate shutdown
     */
    void force_shutdown();

    /**
     * @brief Get current application state
     */
    ApplicationState state() const noexcept { return state_.load(); }

    /**
     * @brief Get application configuration
     */
    const ApplicationConfig& config() const noexcept { return config_; }

    /**
     * @brief Get the main IO context
     */
    asio::io_context& io_context() noexcept { return io_context_; }

    /**
     * @brief Get the work executor for the main IO context
     */
    asio::any_io_executor executor() noexcept { return io_context_.get_executor(); }

    /**
     * @brief Post a task to the event loop with priority-based scheduling optimization
     * @param task Task function to execute
     * @param priority Task execution priority:
     *                 - Critical: Immediate execution using dispatch (highest priority)
     *                 - High: Immediate execution using dispatch (high priority)
     *                 - Normal: Queued execution using post (default, balanced)
     *                 - Low: Queued execution using post (explicitly lower priority)
     *
     * @note All priorities maintain full exception safety for application stability.
     *       Priority affects WHEN a task executes, not HOW safely it executes.
     *       - dispatch: Executes immediately if called from event loop, otherwise queues
     *       - post: Always queues for execution in event loop order
     *
     * @example
     * // Safe execution with default priority
     * app.post_task([]{ do_work(); });
     *
     * // High-priority execution (immediate dispatch)
     * app.post_task([]{ urgent_work(); }, TaskPriority::High);
     *
     * // Critical priority for time-sensitive operations
     * app.post_task([]{ real_time_processing(); }, TaskPriority::Critical);
     */
    void post_task(TaskFunction task, TaskPriority priority = TaskPriority::Normal);

    /**
     * @brief Post a delayed task to the event loop
     * @param task Task function to execute
     * @param delay Delay before execution
     * @param priority Task priority (default: Normal)
     */
    void post_delayed_task(TaskFunction task,
                          std::chrono::milliseconds delay,
                          TaskPriority priority = TaskPriority::Normal);

    /**
     * @brief Schedule a recurring task
     * @param task Task function to execute
     * @param interval Interval between executions
     * @param priority Task priority (default: Normal)
     * @return Task ID for cancellation
     */
    std::size_t schedule_recurring_task(TaskFunction task,
                                       std::chrono::milliseconds interval,
                                       TaskPriority priority = TaskPriority::Normal);

    /**
     * @brief Cancel a recurring task
     * @param task_id Task ID returned by schedule_recurring_task
     */
    void cancel_recurring_task(std::size_t task_id);

    /**
     * @brief Add an application component
     * @param component Unique pointer to component
     */
    void add_component(std::unique_ptr<ApplicationComponent> component);

    /**
     * @brief Get component by name
     * @param name Component name
     * @return Pointer to component or nullptr if not found
     */
    ApplicationComponent* get_component(std::string_view name) const;

    /**
     * @brief Set custom signal handler for specific signal
     * @param signal Signal number
     * @param handler Handler function
     */
    void set_signal_handler(int signal, SignalHandler handler);

    /**
     * @brief Set global error handler
     * @param handler Error handler function
     */
    void set_error_handler(ErrorHandler handler);

    /**
     * @brief Check if application is running
     */
    bool is_running() const noexcept {
        return state_.load() == ApplicationState::Running;
    }

    /**
     * @brief Wait for application to stop
     * @param timeout Maximum time to wait
     * @return true if stopped within timeout
     */
    bool wait_for_stop(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    /**
     * @brief Reload application configuration
     * @return true if configuration was reloaded successfully
     */
    bool reload_config();

    // Thread Management API

    /**
     * @brief Simplified event-driven managed thread using std::jthread's built-in capabilities
     */
    class ManagedThread : public ManagedThreadBase {
    public:
        using ThreadFunction = std::function<void(ManagedThread&)>;

        explicit ManagedThread(std::string name, ThreadFunction func = nullptr);

        // std::jthread automatically joins in destructor - no manual join() needed
        ~ManagedThread() = default;

        // Non-copyable, non-movable
        ManagedThread(const ManagedThread&) = delete;
        ManagedThread& operator=(const ManagedThread&) = delete;
        ManagedThread(ManagedThread&&) = delete;
        ManagedThread& operator=(ManagedThread&&) = delete;

        /**
         * @brief Get the thread's IO context
         */
        asio::io_context& io_context() noexcept { return io_context_; }

        /**
         * @brief Get the thread's executor
         */
        asio::any_io_executor executor() noexcept { return io_context_.get_executor(); }

        /**
         * @brief Get thread name
         */
        const std::string& name() const noexcept { return name_; }

        /**
         * @brief Get the thread's stop token
         */
        std::stop_token get_stop_token() const noexcept { return thread_.get_stop_token(); }

        /**
         * @brief Request thread to stop (uses jthread's cooperative cancellation)
         */
        void request_stop() noexcept { thread_.request_stop(); }

        /**
         * @brief Check if stop was requested
         */
        bool stop_requested() const noexcept { return thread_.get_stop_token().stop_requested(); }

        /**
         * @brief Post a task to this thread's event loop (wakes up thread immediately)
         */
        void post_task(std::function<void()> task);

        /**
         * @brief Send a typed message to this thread (wakes up thread immediately)
         */
        template<MessageType T>
        bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
            return messaging_context_->send_message(std::move(data), priority);
        }

        /**
         * @brief Subscribe to messages of a specific type
         */
        template<MessageType T>
        void subscribe_to_messages(MessageHandler<T> handler) {
            messaging_context_->subscribe(std::move(handler));
        }

        /**
         * @brief Unsubscribe from messages of a specific type
         */
        template<MessageType T>
        void unsubscribe_from_messages() {
            messaging_context_->unsubscribe<T>();
        }

        /**
         * @brief Get current message queue size
         */
        std::size_t queue_size() const {
            return messaging_context_->pending_message_count();
        }

    private:
        void thread_main(std::stop_token stop_token);

        const std::string name_;
        const ThreadFunction func_;
        asio::io_context io_context_;
        std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
        std::shared_ptr<ThreadMessagingContext> messaging_context_;
        std::jthread thread_;  // Last member - destructor auto-joins
    };

    /**
     * @brief Create an event-driven managed thread (starts automatically)
     * @param name Thread name for logging/debugging
     * @param thread_func Optional function to run in the thread (receives ManagedThread reference)
     * @return Shared pointer to managed thread
     */
    std::shared_ptr<ManagedThreadBase> create_thread(std::string name,
                                                std::function<void(ManagedThreadBase&)> thread_func = nullptr) override;

    /**
     * @brief Get the number of managed threads
     */
    std::size_t managed_thread_count() const;

    /**
     * @brief Stop all managed threads (uses jthread's cooperative cancellation)
     */
    void stop_all_managed_threads();

    /**
     * @brief Check if any managed thread has been requested to stop
     */
    bool any_managed_thread_stop_requested() const;

    /**
     * @brief Get managed thread by name
     * @param name Thread name
     * @return Pointer to thread or nullptr if not found
     */
    ManagedThread* get_managed_thread(const std::string& name) const;

    // ============================================================================
    // Messaging API
    // ============================================================================

    /**
     * @brief Send message to a specific thread
     * @param target_thread Target thread name
     * @param data Message data
     * @param priority Message priority
     * @return true if message was sent successfully
     */
    template<MessageType T>
    bool send_message_to_thread(const std::string& target_thread, T data,
                               MessagePriority priority = MessagePriority::Normal) {
        return InterThreadMessagingBus::instance().send_to_thread(target_thread, std::move(data), priority);
    }

    /**
     * @brief Broadcast message to all managed threads
     * @param data Message data
     * @param priority Message priority
     */
    template<MessageType T>
    void broadcast_message(T data, MessagePriority priority = MessagePriority::Normal) {
        InterThreadMessagingBus::instance().broadcast(std::move(data), priority);
    }

    /**
     * @brief Get messaging bus instance
     */
    InterThreadMessagingBus& messaging_bus() { return InterThreadMessagingBus::instance(); }

    // CLI Management

    /**
     * @brief Get CLI instance
     */
    CLI& cli() const;

    /**
     * @brief Enable CLI with configuration
     * @param config CLI configuration
     * @return true if CLI was started successfully
     */
    bool enable_cli(const CLIConfig& config);

    /**
     * @brief Enable CLI with default configuration
     * @return true if CLI was started successfully
     */
    bool enable_cli();

    /**
     * @brief Disable CLI
     */
    void disable_cli();

    /**
     * @brief Check if CLI is enabled and running
     */
    bool is_cli_enabled() const;

protected:
    /**
     * @brief Override this method for custom initialization
     * Called after basic initialization but before components
     * @return true if initialization successful
     */
    virtual bool on_initialize() { return true; }

    /**
     * @brief Override this method for custom startup logic
     * Called after all components are started
     * @return true if startup successful
     */
    virtual bool on_start() { return true; }

    /**
     * @brief Override this method for custom shutdown logic
     * Called before components are stopped
     * @return true if shutdown successful
     */
    virtual bool on_stop() { return true; }

    /**
     * @brief Override this method for custom cleanup
     * Called after all components are stopped
     */
    virtual void on_cleanup() {}

    /**
     * @brief Override this method for custom signal handling
     * @param signal Signal number received
     */
    virtual void on_signal(int signal);

    /**
     * @brief Override this method for custom config reload handling
     * Called when SIGHUP is received or reload_config() is called
     * @return true if config was reloaded successfully
     */
    virtual bool on_config_reload();

    /**
     * @brief Override this method for custom error handling
     * @param error Exception that occurred
     */
    virtual void on_error(const std::exception& error);

private:
    // Core application state and configuration
    ApplicationConfig config_;
    std::atomic<ApplicationState> state_{ApplicationState::Created};

    // Boost.Asio event loop infrastructure
    asio::io_context io_context_;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
    asio::signal_set signals_;
    std::vector<std::jthread> worker_threads_;

    // Component management
    std::vector<std::unique_ptr<ApplicationComponent>> components_;

    // Task scheduling
    struct RecurringTask {
        std::size_t id;
        std::shared_ptr<asio::steady_timer> timer;
        TaskFunction function;
        std::chrono::milliseconds interval;
        TaskPriority priority;
    };
    std::vector<RecurringTask> recurring_tasks_;
    std::atomic<std::size_t> next_task_id_{1};

    // Signal and error handling
    std::map<int, SignalHandler> signal_handlers_;
    ErrorHandler error_handler_;

    // Health monitoring
    std::unique_ptr<asio::steady_timer> health_timer_;

    // Managed threads
    std::vector<std::shared_ptr<ManagedThread>> managed_threads_;
    mutable std::mutex managed_threads_mutex_;

    // CLI management
    bool cli_enabled_{false};

    // Synchronization
    mutable std::mutex components_mutex_;
    mutable std::mutex tasks_mutex_;
    std::condition_variable stop_condition_;
    std::mutex stop_mutex_;

    // Private methods
    bool initialize_internal();
    bool start_internal();
    void stop_internal();
    void setup_signal_handling();
    void handle_signal(int signal);
    void reset_signal_handling_after_fork();
    void start_worker_threads();
    void stop_worker_threads();
    void start_health_monitoring();
    void stop_health_monitoring();
    void perform_health_check();
    void execute_recurring_task(std::shared_ptr<RecurringTask> task);
    void change_state(ApplicationState new_state);
    void handle_exception(const std::exception& e);

    // Command line argument parsing helpers
    void apply_command_line_overrides();

    // Daemonization helper methods
#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    bool daemonize_unix();
    bool drop_privileges();
    bool create_pid_file();
    void remove_pid_file();
    bool redirect_standard_fds();
    void close_all_fds();
    static void signal_handler_wrapper(int signal);
    void setup_unix_signal_handlers();
    static Application* signal_instance_;
#endif
};

/**
 * @brief Base class for threaded components that run on ManagedThread
 *
 * Provides a high-level interface for creating thread-based components with:
 * - Automatic thread lifecycle management
 * - Built-in message handling infrastructure
 * - Event-driven architecture with ASIO integration
 * - Clean separation between infrastructure and business logic
 *
 * Usage:
 * @code
 * class MyService : public ThreadedComponent {
 * public:
 *     MyService() : ThreadedComponent("MyService") {}
 *
 * protected:
 *     bool on_initialize() override {
 *         // Setup message handlers, initialize state
 *         subscribe_to_messages<MyMessage>([this](const MyMessage& msg) {
 *             handle_my_message(msg);
 *         });
 *         return true;
 *     }
 *
 *     bool on_start() override {
 *         // Start business logic, timers, etc.
 *         setup_periodic_work();
 *         return true;
 *     }
 *
 *     void on_stop() override {
 *         // Cleanup business logic
 *         cleanup_resources();
 *     }
 * };
 * @endcode
 */
class ThreadedComponent {
public:
    /**
     * @brief Construct threaded component with name
     * @param name Component name for logging and identification
     */
    explicit ThreadedComponent(std::string name);

    /**
     * @brief Virtual destructor - automatically stops the component
     */
    virtual ~ThreadedComponent();

    // Non-copyable, non-movable
    ThreadedComponent(const ThreadedComponent&) = delete;
    ThreadedComponent& operator=(const ThreadedComponent&) = delete;
    ThreadedComponent(ThreadedComponent&&) = delete;
    ThreadedComponent& operator=(ThreadedComponent&&) = delete;

    /**
     * @brief Initialize the threaded component
     * @param thread_factory Factory for creating managed threads
     * @return true if initialization successful
     */
    bool initialize(ThreadFactory& thread_factory);

    /**
     * @brief Start the threaded component (creates and starts the managed thread)
     * @return true if start successful
     */
    bool start();

    /**
     * @brief Stop the threaded component (gracefully stops the managed thread)
     * @return true if stop successful
     */
    bool stop();

    /**
     * @brief Get component name
     */
    const std::string& name() const noexcept { return name_; }

    /**
     * @brief Check if component is running
     */
    bool is_running() const noexcept { return running_.load(); }

    /**
     * @brief Get the managed thread (may be nullptr if not started)
     */
    ManagedThreadBase* managed_thread() const noexcept { return managed_thread_.get(); }

    /**
     * @brief Get the concrete managed thread (may be nullptr if not started)
     */
    Application::ManagedThread* concrete_managed_thread() const noexcept {
        return static_cast<Application::ManagedThread*>(managed_thread_.get());
    }

    /**
     * @brief Send a typed message to this component's thread
     * @param data Message data
     * @param priority Message priority
     * @return true if message was sent successfully
     */
    template<MessageType T>
    bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
        auto concrete_thread = concrete_managed_thread();
        if (!concrete_thread) {
            return false;
        }
        return concrete_thread->send_message(std::move(data), priority);
    }

    /**
     * @brief Post a task to this component's thread
     * @param task Task function to execute
     */
    void post_task(std::function<void()> task) {
        if (managed_thread_) {
            managed_thread_->post_task(std::move(task));
        }
    }

    /**
     * @brief Get the component's IO context (only valid when running)
     */
    asio::io_context* io_context() const noexcept {
        auto concrete_thread = concrete_managed_thread();
        return concrete_thread ? &concrete_thread->io_context() : nullptr;
    }

    /**
     * @brief Get the component's executor (only valid when running)
     */
    asio::any_io_executor executor() const {
        auto concrete_thread = concrete_managed_thread();
        if (!concrete_thread) {
            throw std::runtime_error("ThreadedComponent not running - no executor available");
        }
        return concrete_thread->executor();
    }

protected:
    /**
     * @brief Override for component-specific initialization
     * Called in the main thread before the managed thread is created.
     * Use this to set up message handlers, initialize non-thread-specific state, etc.
     * @return true if initialization successful
     */
    virtual bool on_initialize() { return true; }

    /**
     * @brief Override for component-specific startup logic
     * Called in the component's managed thread after it starts.
     * Use this to start timers, setup periodic tasks, begin business logic, etc.
     * @return true if start successful
     */
    virtual bool on_start() { return true; }

    /**
     * @brief Override for component-specific shutdown logic
     * Called in the component's managed thread during shutdown.
     * Use this to cleanup resources, cancel timers, finish ongoing work, etc.
     */
    virtual void on_stop() {}

    /**
     * @brief Override for component health checking
     * Called periodically if health monitoring is enabled.
     * @return true if component is healthy
     */
    virtual bool on_health_check() { return true; }

    /**
     * @brief Subscribe to messages of a specific type
     * Can be called from on_initialize() or later during runtime.
     * @param handler Function to handle messages of type T
     */
    template<MessageType T>
    void subscribe_to_messages(MessageHandler<T> handler) {
        auto concrete_thread = concrete_managed_thread();
        if (concrete_thread) {
            concrete_thread->subscribe_to_messages<T>(std::move(handler));
        } else {
            // Store handler for later registration when thread starts
            pending_subscriptions_.emplace_back([this, handler = std::move(handler)]() mutable {
                auto concrete_thread = concrete_managed_thread();
                if (concrete_thread) {
                    concrete_thread->subscribe_to_messages<T>(std::move(handler));
                }
            });
        }
    }

    /**
     * @brief Unsubscribe from messages of a specific type
     */
    template<MessageType T>
    void unsubscribe_from_messages() {
        auto concrete_thread = concrete_managed_thread();
        if (concrete_thread) {
            concrete_thread->unsubscribe_from_messages<T>();
        }
    }

    /**
     * @brief Schedule a timer that executes repeatedly
     * @param interval Time between executions
     * @param callback Function to execute
     * @return Timer ID for cancellation, or 0 if component not running
     */
    std::size_t schedule_timer(std::chrono::milliseconds interval, std::function<void()> callback) {
        auto concrete_thread = concrete_managed_thread();
        if (!concrete_thread) {
            return 0;
        }

        auto timer_id = next_timer_id_++;
        auto timer = std::make_shared<asio::steady_timer>(concrete_thread->io_context());

        timers_[timer_id] = timer;

        // Use a shared_ptr to the recursive function to avoid capture issues
        auto schedule_func = std::make_shared<std::function<void()>>();
        *schedule_func = [this, timer, interval, callback, timer_id, schedule_func]() {
            if (!running_.load() || timers_.find(timer_id) == timers_.end()) {
                return; // Timer was cancelled or component stopped
            }

            timer->expires_after(interval);
            timer->async_wait([this, callback, schedule_func](const asio::error_code& ec) {
                if (!ec && running_.load()) {
                    try {
                        callback();
                    } catch (const std::exception& e) {
                        Logger::error("Exception in timer callback for '{}': {}", name_, e.what());
                    }
                    (*schedule_func)();
                }
            });
        };

        // Start the timer
        post_task(*schedule_func);
        return timer_id;
    }

    /**
     * @brief Cancel a scheduled timer
     * @param timer_id Timer ID returned by schedule_timer
     */
    void cancel_timer(std::size_t timer_id) {
        auto it = timers_.find(timer_id);
        if (it != timers_.end()) {
            it->second->cancel();
            timers_.erase(it);
        }
    }

    /**
     * @brief Check if stop was requested
     */
    bool stop_requested() const noexcept {
        return managed_thread_ ? managed_thread_->stop_requested() : true;
    }

private:
    void thread_main(Application::ManagedThread& thread);
    void apply_pending_subscriptions();

    const std::string name_;
    ThreadFactory* thread_factory_{nullptr};
    std::shared_ptr<ManagedThreadBase> managed_thread_;
    std::atomic<bool> running_{false};

    // Timer management
    std::unordered_map<std::size_t, std::shared_ptr<asio::steady_timer>> timers_;
    std::atomic<std::size_t> next_timer_id_{1};

    // Deferred message subscription setup
    std::vector<std::function<void()>> pending_subscriptions_;
};

/**
 * @brief Helper macro to create main function for an application
 */
#define BASE_APPLICATION_MAIN(AppClass) \
int main(int argc, char* argv[]) { \
    try { \
        AppClass app; \
        return app.run(argc, argv); \
    } catch (const std::exception& e) { \
        std::cerr << "Fatal error: " << e.what() << std::endl; \
        return EXIT_FAILURE; \
    } catch (...) { \
        std::cerr << "Unknown fatal error occurred" << std::endl; \
        return EXIT_FAILURE; \
    } \
}

} // namespace base
