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
#include "singleton.h"
#include "messaging.h"

#include <asio.hpp>
#include <asio/signal_set.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <future>
#include <exception>

namespace crux {

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
    std::string name = "crux_app";
    std::string version = "1.0.0";
    std::string description = "Crux Application";

    // Threading configuration
    std::size_t worker_threads = std::thread::hardware_concurrency();
    bool use_dedicated_io_thread = true;

    // Signal handling
    std::vector<int> handled_signals = {SIGINT, SIGTERM, SIGUSR1, SIGUSR2};

    // Lifecycle timeouts
    std::chrono::milliseconds startup_timeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds shutdown_timeout = std::chrono::milliseconds(10000);

    // Health check
    bool enable_health_check = true;
    std::chrono::milliseconds health_check_interval = std::chrono::milliseconds(5000);

    // Configuration file
    std::string config_file = "app.toml";
    std::string config_app_name = "default";

    // CLI configuration
    bool enable_cli = false;
    bool cli_enable_stdin = true;
    bool cli_enable_tcp = false;
    std::string cli_bind_address = "127.0.0.1";
    std::uint16_t cli_port = 8080;
};

/**
 * @brief Task priority levels for the application
 */
enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Forward declarations
 */
class Application;
class CLI;

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
class Application : public SingletonBase<Application> {
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
     * @brief Post a task to the event loop
     * @param task Task function to execute
     * @param priority Task priority (default: Normal)
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

    // Thread Management API

    /**
     * @brief Managed thread handle for custom event loops
     */
    class ManagedThread {
    public:
        using ThreadFunction = std::function<void(asio::io_context&)>;

        ManagedThread(std::string name, ThreadFunction func);
        ~ManagedThread();

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
         * @brief Check if thread is running
         */
        bool is_running() const noexcept { return running_.load(); }

        /**
         * @brief Post a task to this thread's event loop
         */
        void post_task(std::function<void()> task);

        /**
         * @brief Send a typed message to this thread
         */
        template<MessageType T>
        bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
            if (!messaging_context_) {
                return false;
            }
            return messaging_context_->send_message(std::move(data), priority);
        }

        /**
         * @brief Subscribe to messages of a specific type
         */
        template<MessageType T>
        void subscribe_to_messages(MessageHandler<T> handler) {
            if (messaging_context_) {
                messaging_context_->subscribe<T>(std::move(handler));
            }
        }

        /**
         * @brief Unsubscribe from messages of a specific type
         */
        template<MessageType T>
        void unsubscribe_from_messages() {
            if (messaging_context_) {
                messaging_context_->unsubscribe<T>();
            }
        }

        /**
         * @brief Get pending message count
         */
        std::size_t pending_message_count() const {
            return messaging_context_ ? messaging_context_->pending_message_count() : 0;
        }

        /**
         * @brief Stop the thread gracefully
         */
        void stop();

        /**
         * @brief Wait for thread to finish
         */
        void join();

    private:
        void run();
        void schedule_message_processing();

        std::string name_;
        asio::io_context io_context_;
        std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
        std::thread thread_;
        std::atomic<bool> running_{false};
        ThreadFunction user_function_;
        std::shared_ptr<ThreadMessagingContext> messaging_context_;
        std::shared_ptr<asio::steady_timer> message_timer_;
    };

    /**
     * @brief Event-driven managed thread with immediate message processing
     */
    class EventDrivenManagedThread {
    public:
        using ThreadFunction = std::function<void(EventDrivenManagedThread&)>;

        EventDrivenManagedThread(std::string name, ThreadFunction func = nullptr);
        ~EventDrivenManagedThread();

        // Non-copyable, non-movable
        EventDrivenManagedThread(const EventDrivenManagedThread&) = delete;
        EventDrivenManagedThread& operator=(const EventDrivenManagedThread&) = delete;
        EventDrivenManagedThread(EventDrivenManagedThread&&) = delete;
        EventDrivenManagedThread& operator=(EventDrivenManagedThread&&) = delete;

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
         * @brief Check if thread is running
         */
        bool is_running() const noexcept { return running_.load(); }

        /**
         * @brief Post a task to this thread's event loop
         */
        void post_task(std::function<void()> task);

        /**
         * @brief Send a typed message to this thread (immediate notification)
         */
        template<MessageType T>
        bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
            if (!messaging_context_) {
                return false;
            }
            return messaging_context_->send_message(std::move(data), priority);
        }

        /**
         * @brief Subscribe to messages of a specific type
         */
        template<MessageType T>
        void subscribe_to_messages(MessageHandler<T> handler) {
            if (messaging_context_) {
                messaging_context_->subscribe<T>(std::move(handler));
            }
        }

        /**
         * @brief Unsubscribe from messages of a specific type
         */
        template<MessageType T>
        void unsubscribe_from_messages() {
            if (messaging_context_) {
                messaging_context_->unsubscribe<T>();
            }
        }

        /**
         * @brief Stop the thread gracefully
         */
        void stop();

        /**
         * @brief Join the thread (wait for completion)
         */
        void join();

        /**
         * @brief Get current queue size
         */
        std::size_t queue_size() const {
            return messaging_context_ ? messaging_context_->queue_size() : 0;
        }

    private:
        void thread_main();

        std::string name_;
        ThreadFunction func_;
        std::atomic<bool> running_{false};
        std::thread thread_;
        asio::io_context io_context_;
        std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
        std::unique_ptr<EventDrivenThreadMessagingContext> messaging_context_;
    };

    /**
     * @brief Create and start a managed thread with its own event loop
     * @param name Thread name for logging/debugging
     * @param thread_func Function to run in the thread (receives io_context reference)
     * @return Shared pointer to managed thread
     */
    std::shared_ptr<ManagedThread> create_thread(std::string name,
                                                ManagedThread::ThreadFunction thread_func);

    /**
     * @brief Create a simple worker thread with automatic task handling
     * @param name Thread name for logging/debugging
     * @return Shared pointer to managed thread
     */
    std::shared_ptr<ManagedThread> create_worker_thread(std::string name);

    /**
     * @brief Create an event-driven worker thread with immediate message processing
     * @param name Thread name for logging/debugging
     * @return Shared pointer to event-driven managed thread
     */
    std::shared_ptr<EventDrivenManagedThread> create_event_driven_thread(std::string name);

    /**
     * @brief Get the number of managed threads
     */
    std::size_t managed_thread_count() const;

    /**
     * @brief Stop all managed threads gracefully
     */
    void stop_all_managed_threads();

    /**
     * @brief Wait for all managed threads to finish
     */
    void join_all_managed_threads();

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
        return MessagingBus::instance().send_to_thread(target_thread, std::move(data), priority);
    }

    /**
     * @brief Broadcast message to all managed threads
     * @param data Message data
     * @param priority Message priority
     */
    template<MessageType T>
    void broadcast_message(T data, MessagePriority priority = MessagePriority::Normal) {
        MessagingBus::instance().broadcast(std::move(data), priority);
    }

    /**
     * @brief Get messaging bus instance
     */
    MessagingBus& messaging_bus() { return MessagingBus::instance(); }

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
    std::vector<std::thread> worker_threads_;

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
    void start_worker_threads();
    void stop_worker_threads();
    void start_health_monitoring();
    void stop_health_monitoring();
    void perform_health_check();
    void execute_recurring_task(std::shared_ptr<RecurringTask> task);
    void change_state(ApplicationState new_state);
    void handle_exception(const std::exception& e);
};

/**
 * @brief Helper macro to create main function for an application
 */
#define CRUX_APPLICATION_MAIN(AppClass) \
int main(int argc, char* argv[]) { \
    try { \
        AppClass app; \
        return app.run(); \
    } catch (const std::exception& e) { \
        std::cerr << "Fatal error: " << e.what() << std::endl; \
        return EXIT_FAILURE; \
    } catch (...) { \
        std::cerr << "Unknown fatal error occurred" << std::endl; \
        return EXIT_FAILURE; \
    } \
}

} // namespace crux
