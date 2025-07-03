/*
 * @file cli.h
 * @brief Runtime inspection and debugging CLI for Base applications
 *
 * Provides a command-line interface for inspecting and debugging application
 * internals including thread status, message queues, configuration, health,
 * and custom user-defined commands.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "logger.h"
#include "singleton.h"
#include <asio.hpp>

#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <sstream>
#include <iostream>
#include <future>

namespace base {

// Forward declarations
class Application;
enum class ApplicationState;

/**
 * @brief CLI command result
 */
struct CLIResult {
    bool success{true};
    std::string output;
    std::string error_message;

    CLIResult() = default;
    CLIResult(bool s, std::string out = "", std::string err = "")
        : success(s), output(std::move(out)), error_message(std::move(err)) {}

    static CLIResult ok(std::string output = "") {
        return CLIResult{true, std::move(output), ""};
    }

    static CLIResult error(std::string error_msg) {
        return CLIResult{false, "", std::move(error_msg)};
    }
};

/**
 * @brief CLI command context with parsed arguments
 */
struct CLIContext {
    std::vector<std::string> args;
    std::unordered_map<std::string, std::string> options;
    Application* app{nullptr};

    bool has_option(const std::string& name) const {
        return options.find(name) != options.end();
    }

    std::string get_option(const std::string& name, const std::string& default_value = "") const {
        auto it = options.find(name);
        return it != options.end() ? it->second : default_value;
    }
};

/**
 * @brief CLI command handler function
 */
using CLICommandHandler = std::function<CLIResult(const CLIContext&)>;

/**
 * @brief CLI command definition
 */
struct CLICommand {
    std::string name;
    std::string description;
    std::string usage;
    CLICommandHandler handler;
    bool requires_app{true};  // Whether command requires application context

    CLICommand(std::string n, std::string desc, std::string use, CLICommandHandler h, bool req_app = true)
        : name(std::move(n)), description(std::move(desc)), usage(std::move(use)), handler(std::move(h)), requires_app(req_app) {}
};

/**
 * @brief Configuration for CLI server
 */
struct CLIConfig {
    bool enable{true};
    std::string bind_address{"127.0.0.1"};
    std::uint16_t port{8080};
    bool enable_stdin{true};          // Enable stdin commands
    bool enable_tcp_server{false};    // Enable TCP server for remote access
    std::string prompt{"> "};
    std::chrono::milliseconds command_timeout{std::chrono::milliseconds(5000)};
};

/**
 * @brief Runtime inspection and debugging CLI
 *
 * The CLI provides both stdin-based and optional TCP-based interfaces for
 * inspecting and debugging application internals in real-time.
 *
 * Built-in commands:
 * - help: Show available commands
 * - status: Show application status
 * - threads: List and inspect managed threads
 * - config: Show configuration
 * - health: Run health checks
 * - messaging: Show messaging statistics
 * - log-level: Change log level
 * - shutdown: Graceful shutdown
 * - force-shutdown: Immediate shutdown
 *
 * Usage:
 * @code
 * // In your application
 * auto& cli = CLI::instance();
 * cli.configure({.enable = true, .enable_tcp_server = true, .port = 8080});
 * cli.register_command("mycmd", "My command", "mycmd [args]", my_handler);
 * cli.start(*this);
 *
 * // Then use via stdin or telnet localhost 8080
 * > status
 * > threads
 * > mycmd arg1 arg2
 * @endcode
 */
class CLI : public SingletonBase<CLI> {
public:
    /**
     * @brief Default constructor
     */
    CLI();

    /**
     * @brief Destructor
     */
    ~CLI();

    // Non-copyable, non-movable
    CLI(const CLI&) = delete;
    CLI& operator=(const CLI&) = delete;
    CLI(CLI&&) = delete;
    CLI& operator=(CLI&&) = delete;

    /**
     * @brief Configure the CLI
     * @param config CLI configuration
     */
    void configure(CLIConfig config);

    /**
     * @brief Start the CLI with application context
     * @param app Application instance
     * @return true if started successfully
     */
    bool start(Application& app);

    /**
     * @brief Stop the CLI
     */
    void stop();

    /**
     * @brief Register a custom command
     * @param name Command name
     * @param description Command description
     * @param usage Usage string
     * @param handler Command handler function
     * @param requires_app Whether command requires application context
     */
    void register_command(const std::string& name,
                         const std::string& description,
                         const std::string& usage,
                         CLICommandHandler handler,
                         bool requires_app = true);

    /**
     * @brief Execute a command
     * @param command_line Command line to execute
     * @return Command result
     */
    CLIResult execute_command(const std::string& command_line);

    /**
     * @brief Check if CLI is running
     */
    bool is_running() const noexcept { return running_.load(); }

    /**
     * @brief Get current configuration
     */
    const CLIConfig& config() const noexcept { return config_; }

private:
    CLIConfig config_;
    Application* app_{nullptr};
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};

    // Command registry
    std::unordered_map<std::string, std::unique_ptr<CLICommand>> commands_;
    mutable std::mutex commands_mutex_;

    // Threading
    std::unique_ptr<std::thread> stdin_thread_;
    std::unique_ptr<std::thread> tcp_server_thread_;

    // ASIO for TCP server
    std::unique_ptr<asio::io_context> tcp_io_context_;
    std::unique_ptr<asio::ip::tcp::acceptor> tcp_acceptor_;

    /**
     * @brief Initialize built-in commands
     */
    void initialize_builtin_commands();

    /**
     * @brief Parse command line into context
     * @param command_line Raw command line
     * @return Parsed context
     */
    CLIContext parse_command_line(const std::string& command_line);

    /**
     * @brief Execute parsed command
     * @param context Parsed command context
     * @return Command result
     */
    CLIResult execute_parsed_command(const CLIContext& context);

    /**
     * @brief Start stdin reader thread
     */
    void start_stdin_reader();

    /**
     * @brief Start TCP server thread
     */
    void start_tcp_server();

    /**
     * @brief Handle TCP client connection
     * @param socket Client socket
     */
    void handle_tcp_client(std::shared_ptr<asio::ip::tcp::socket> socket);

    /**
     * @brief Process command from input stream
     * @param input Input stream
     * @param output Output stream
     * @param prompt_enabled Whether to show prompt
     */
    void process_commands(std::istream& input, std::ostream& output, bool prompt_enabled = true);

    // Built-in command handlers
    CLIResult cmd_help(const CLIContext& context);
    CLIResult cmd_status(const CLIContext& context);
    CLIResult cmd_threads(const CLIContext& context);
    CLIResult cmd_config(const CLIContext& context);
    CLIResult cmd_health(const CLIContext& context);
    CLIResult cmd_messaging(const CLIContext& context);
    CLIResult cmd_log_level(const CLIContext& context);
    CLIResult cmd_shutdown(const CLIContext& context);
    CLIResult cmd_force_shutdown(const CLIContext& context);
    CLIResult cmd_exit(const CLIContext& context);

    /**
     * @brief Format application state as string
     */
    std::string format_app_state(ApplicationState state) const;

    /**
     * @brief Format duration in human-readable format
     */
    std::string format_duration(std::chrono::nanoseconds duration) const;

    /**
     * @brief Format bytes in human-readable format
     */
    std::string format_bytes(std::size_t bytes) const;

    /**
     * @brief Safe command execution with timeout
     */
    CLIResult execute_with_timeout(std::function<CLIResult()> func, std::chrono::milliseconds timeout);
};

} // namespace base
