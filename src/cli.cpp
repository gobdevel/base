/*
 * @file cli.cpp
 * @brief Runtime inspection and debugging CLI implementation
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "cli.h"
#include "application.h"
#include "config.h"
#include "messaging.h"

#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <regex>

namespace base {

CLI::CLI() {
    Logger::debug("CLI created");
    initialize_builtin_commands();
}

CLI::~CLI() {
    if (running_.load()) {
        Logger::warn("CLI destroyed while running, forcing stop");
        stop();
    }
    Logger::debug("CLI destroyed");
}

void CLI::configure(CLIConfig config) {
    if (running_.load()) {
        Logger::warn("Cannot reconfigure CLI while running");
        return;
    }

    config_ = std::move(config);
    Logger::info("CLI configured - stdin: {}, tcp: {} (port: {})",
                 config_.enable_stdin,
                 config_.enable_tcp_server,
                 config_.port);
}

bool CLI::start(Application& app) {
    if (running_.load()) {
        Logger::warn("CLI already running");
        return false;
    }

    if (!config_.enable) {
        Logger::debug("CLI disabled in configuration");
        return true;
    }

    app_ = &app;
    shutdown_requested_.store(false);
    running_.store(true);

    Logger::info("Starting CLI with stdin: {}, tcp: {}",
                 config_.enable_stdin, config_.enable_tcp_server);

    try {
        if (config_.enable_stdin) {
            start_stdin_reader();
        }

        if (config_.enable_tcp_server) {
            start_tcp_server();
        }

        Logger::info("CLI started successfully");
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to start CLI: {}", e.what());
        running_.store(false);
        return false;
    }
}

void CLI::stop() {
    if (!running_.load()) {
        return;
    }

    Logger::info("Stopping CLI");
    shutdown_requested_.store(true);
    running_.store(false);

    // Stop TCP server
    if (tcp_acceptor_) {
        tcp_acceptor_->close();
    }
    if (tcp_io_context_) {
        tcp_io_context_->stop();
    }

    // Wait for threads to finish
    if (tcp_server_thread_ && tcp_server_thread_->joinable()) {
        tcp_server_thread_->join();
    }

    if (stdin_thread_ && stdin_thread_->joinable()) {
        stdin_thread_->join();
    }

    tcp_acceptor_.reset();
    tcp_io_context_.reset();
    tcp_server_thread_.reset();
    stdin_thread_.reset();

    Logger::info("CLI stopped");
}

void CLI::register_command(const std::string& name,
                          const std::string& description,
                          const std::string& usage,
                          CLICommandHandler handler,
                          bool requires_app) {
    std::lock_guard<std::mutex> lock(commands_mutex_);

    auto command = std::make_unique<CLICommand>(name, description, usage, std::move(handler), requires_app);
    commands_[name] = std::move(command);

    Logger::debug("Registered CLI command: {}", name);
}

CLIResult CLI::execute_command(const std::string& command_line) {
    if (command_line.empty()) {
        return CLIResult::ok();
    }

    try {
        auto context = parse_command_line(command_line);
        return execute_with_timeout([this, context]() {
            return execute_parsed_command(context);
        }, config_.command_timeout);
    } catch (const std::exception& e) {
        return CLIResult::error("Command execution failed: " + std::string(e.what()));
    }
}

void CLI::initialize_builtin_commands() {
    register_command("help", "Show available commands", "help [command]",
                    [this](const CLIContext& ctx) { return cmd_help(ctx); }, false);

    register_command("status", "Show application status", "status",
                    [this](const CLIContext& ctx) { return cmd_status(ctx); });

    register_command("threads", "List and inspect managed threads", "threads [--detail]",
                    [this](const CLIContext& ctx) { return cmd_threads(ctx); });

    register_command("config", "Show configuration", "config [--section <name>]",
                    [this](const CLIContext& ctx) { return cmd_config(ctx); });

    register_command("health", "Run health checks", "health",
                    [this](const CLIContext& ctx) { return cmd_health(ctx); });

    register_command("messaging", "Show messaging statistics", "messaging [--detail]",
                    [this](const CLIContext& ctx) { return cmd_messaging(ctx); });

    register_command("log-level", "Change log level", "log-level <level>",
                    [this](const CLIContext& ctx) { return cmd_log_level(ctx); }, false);

    register_command("shutdown", "Graceful shutdown", "shutdown",
                    [this](const CLIContext& ctx) { return cmd_shutdown(ctx); });

    register_command("force-shutdown", "Immediate shutdown", "force-shutdown",
                    [this](const CLIContext& ctx) { return cmd_force_shutdown(ctx); });

    register_command("exit", "Exit CLI (does not shutdown app)", "exit",
                    [this](const CLIContext& ctx) { return cmd_exit(ctx); }, false);
}

CLIContext CLI::parse_command_line(const std::string& command_line) {
    CLIContext context;
    context.app = app_;

    std::istringstream iss(command_line);
    std::string token;
    bool first_token = true;

    while (iss >> token) {
        if (first_token) {
            context.args.push_back(token);
            first_token = false;
        } else if (token.starts_with("--")) {
            // Parse long option
            auto pos = token.find('=');
            if (pos != std::string::npos) {
                // --option=value
                std::string key = token.substr(2, pos - 2);
                std::string value = token.substr(pos + 1);
                context.options[key] = value;
            } else {
                // --option (boolean or next token is value)
                std::string key = token.substr(2);
                std::string next_token;
                if (iss >> next_token && !next_token.starts_with('-')) {
                    context.options[key] = next_token;
                } else {
                    context.options[key] = "true";
                    if (!next_token.empty()) {
                        // Put back the token we just read
                        iss.seekg(-static_cast<int>(next_token.length() + 1), std::ios::cur);
                    }
                }
            }
        } else if (token.starts_with("-")) {
            // Parse short option
            std::string key = token.substr(1);
            std::string next_token;
            if (iss >> next_token && !next_token.starts_with('-')) {
                context.options[key] = next_token;
            } else {
                context.options[key] = "true";
                if (!next_token.empty()) {
                    // Put back the token we just read
                    iss.seekg(-static_cast<int>(next_token.length() + 1), std::ios::cur);
                }
            }
        } else {
            // Regular argument
            context.args.push_back(token);
        }
    }

    return context;
}

CLIResult CLI::execute_parsed_command(const CLIContext& context) {
    if (context.args.empty()) {
        return CLIResult::ok();
    }

    const std::string& command_name = context.args[0];

    // Find the command handler
    CLICommand* command = nullptr;
    {
        std::lock_guard<std::mutex> lock(commands_mutex_);
        auto it = commands_.find(command_name);
        if (it == commands_.end()) {
            return CLIResult::error("Unknown command: " + command_name + ". Type 'help' for available commands.");
        }
        command = it->second.get();
    }

    // Check requirements and execute handler outside of the mutex lock
    if (command->requires_app && !context.app) {
        return CLIResult::error("Command requires application context but none available");
    }

    return command->handler(context);
}

void CLI::start_stdin_reader() {
    stdin_thread_ = std::make_unique<std::thread>([this]() {
        Logger::debug("CLI stdin reader thread started");

        try {
            process_commands(std::cin, std::cout, true);
        } catch (const std::exception& e) {
            Logger::error("CLI stdin reader error: {}", e.what());
        }

        Logger::debug("CLI stdin reader thread stopped");
    });
}

void CLI::start_tcp_server() {
    tcp_io_context_ = std::make_unique<asio::io_context>();
    tcp_acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(*tcp_io_context_);

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(config_.bind_address), config_.port);
    tcp_acceptor_->open(endpoint.protocol());
    tcp_acceptor_->set_option(asio::ip::tcp::acceptor::reuse_address(true));
    tcp_acceptor_->bind(endpoint);
    tcp_acceptor_->listen();

    Logger::info("CLI TCP server listening on {}:{}", config_.bind_address, config_.port);

    tcp_server_thread_ = std::make_unique<std::thread>([this]() {
        Logger::debug("CLI TCP server thread started");

        while (running_.load()) {
            try {
                auto socket = std::make_shared<asio::ip::tcp::socket>(*tcp_io_context_);
                tcp_acceptor_->accept(*socket);

                Logger::debug("CLI TCP client connected from {}",
                             socket->remote_endpoint().address().to_string());

                // Handle client in a separate thread
                std::thread([this, socket]() {
                    handle_tcp_client(socket);
                }).detach();

            } catch (const std::exception& e) {
                if (running_.load()) {
                    Logger::error("CLI TCP server error: {}", e.what());
                }
                break;
            }
        }

        Logger::debug("CLI TCP server thread stopped");
    });
}

void CLI::handle_tcp_client(std::shared_ptr<asio::ip::tcp::socket> socket) {
    try {
        std::string welcome = "Base CLI v1.0 - Type 'help' for commands\n";
        asio::write(*socket, asio::buffer(welcome));

        asio::streambuf buffer;
        std::string line;

        while (running_.load() && socket->is_open()) {
            // Send prompt
            std::string prompt = config_.prompt;
            asio::write(*socket, asio::buffer(prompt));

            // Read command
            asio::read_until(*socket, buffer, '\n');
            std::istream input(&buffer);
            std::getline(input, line);

            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) {
                continue;
            }

            // Execute command
            auto result = execute_command(line);
            std::string response;

            if (!result.output.empty()) {
                response += result.output + "\n";
            }

            if (!result.error_message.empty()) {
                response += "Error: " + result.error_message + "\n";
            }

            if (!response.empty()) {
                asio::write(*socket, asio::buffer(response));
            }

            // Check for exit command
            if (line == "exit" || line == "quit") {
                break;
            }
        }

        socket->close();
        Logger::debug("CLI TCP client disconnected");

    } catch (const std::exception& e) {
        Logger::debug("CLI TCP client error: {}", e.what());
    }
}

void CLI::process_commands(std::istream& input, std::ostream& output, bool prompt_enabled) {
    std::string line;

    while (running_.load() && !shutdown_requested_.load()) {
        if (prompt_enabled) {
            output << config_.prompt;
            output.flush();
        }

        if (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }

            auto result = execute_command(line);

            if (!result.output.empty()) {
                output << result.output << std::endl;
            }

            if (!result.error_message.empty()) {
                output << "Error: " << result.error_message << std::endl;
            }

            // Check for exit command
            if (line == "exit" || line == "quit") {
                shutdown_requested_.store(true);
                break;
            }
        } else {
            // EOF reached
            break;
        }
    }
}

CLIResult CLI::execute_with_timeout(std::function<CLIResult()> func, std::chrono::milliseconds timeout) {
    auto future = std::async(std::launch::async, func);

    if (future.wait_for(timeout) == std::future_status::timeout) {
        return CLIResult::error("Command timed out after " + std::to_string(timeout.count()) + "ms");
    }

    return future.get();
}

// Built-in command implementations

CLIResult CLI::cmd_help(const CLIContext& context) {
    std::ostringstream oss;

    if (context.args.size() > 1) {
        // Show help for specific command
        const std::string& command_name = context.args[1];
        std::lock_guard<std::mutex> lock(commands_mutex_);
        auto it = commands_.find(command_name);
        if (it == commands_.end()) {
            return CLIResult::error("Unknown command: " + command_name);
        }

        const auto& command = it->second;
        oss << "Command: " << command->name << "\n";
        oss << "Description: " << command->description << "\n";
        oss << "Usage: " << command->usage << "\n";
    } else {
        // Show all commands
        oss << "Available commands:\n\n";

        std::lock_guard<std::mutex> lock(commands_mutex_);
        std::vector<std::pair<std::string, const CLICommand*>> sorted_commands;
        for (const auto& [name, command] : commands_) {
            sorted_commands.emplace_back(name, command.get());
        }

        std::sort(sorted_commands.begin(), sorted_commands.end());

        for (const auto& [name, command] : sorted_commands) {
            oss << std::left << std::setw(15) << name << " - " << command->description << "\n";
        }

        oss << "\nType 'help <command>' for detailed usage information.";
    }

    return CLIResult::ok(oss.str());
}

CLIResult CLI::cmd_status(const CLIContext& context) {
    if (!context.app) {
        return CLIResult::error("No application context available");
    }

    std::ostringstream oss;
    const auto& config = context.app->config();

    oss << "Application Status\n";
    oss << "=================\n";
    oss << "Name: " << config.name << "\n";
    oss << "Version: " << config.version << "\n";
    oss << "Description: " << config.description << "\n";
    oss << "State: " << format_app_state(context.app->state()) << "\n";
    oss << "Running: " << (context.app->is_running() ? "Yes" : "No") << "\n";
    oss << "Worker Threads: " << config.worker_threads << "\n";
    oss << "Managed Threads: " << context.app->managed_thread_count() << "\n";
    oss << "Dedicated IO Thread: " << (config.use_dedicated_io_thread ? "Yes" : "No") << "\n";

    return CLIResult::ok(oss.str());
}

CLIResult CLI::cmd_threads(const CLIContext& context) {
    if (!context.app) {
        return CLIResult::error("No application context available");
    }

    std::ostringstream oss;
    bool detailed = context.has_option("detail");

    oss << "Thread Information\n";
    oss << "==================\n";
    oss << "Total Managed Threads: " << context.app->managed_thread_count() << "\n";
    oss << "Hardware Concurrency: " << std::thread::hardware_concurrency() << "\n";

    if (detailed) {
        oss << "\nNote: Detailed thread inspection requires additional instrumentation\n";
        oss << "Consider adding thread-specific metrics to your application.\n";
    }

    return CLIResult::ok(oss.str());
}

CLIResult CLI::cmd_config(const CLIContext& context) {
    std::ostringstream oss;
    std::string section = context.get_option("section");

    try {
        auto& config_manager = ConfigManager::instance();

        oss << "Configuration\n";
        oss << "=============\n";

        if (!section.empty()) {
            oss << "Section: " << section << "\n";
            // Note: This would require extending Config class to support section enumeration
            oss << "Note: Section-specific display requires Config class extension\n";
        } else {
            oss << "Config file loaded: Yes\n";
            oss << "Note: Full config display requires Config class extension\n";
        }

        if (context.app) {
            const auto& app_config = context.app->config();
            oss << "\nApplication Configuration:\n";
            oss << "Name: " << app_config.name << "\n";
            oss << "Version: " << app_config.version << "\n";
            oss << "Config file: " << app_config.config_file << "\n";
            oss << "Worker threads: " << app_config.worker_threads << "\n";
            oss << "Health checks: " << (app_config.enable_health_check ? "Enabled" : "Disabled") << "\n";
            oss << "Health check interval: " << app_config.health_check_interval.count() << "ms\n";
        }

    } catch (const std::exception& e) {
        return CLIResult::error("Failed to read configuration: " + std::string(e.what()));
    }

    return CLIResult::ok(oss.str());
}

CLIResult CLI::cmd_health(const CLIContext& context) {
    if (!context.app) {
        return CLIResult::error("No application context available");
    }

    std::ostringstream oss;

    oss << "Health Check Results\n";
    oss << "===================\n";
    oss << "Application State: " << format_app_state(context.app->state()) << "\n";
    oss << "Running: " << (context.app->is_running() ? "Healthy" : "Not Running") << "\n";

    // Note: This would require extending Application class to support component health checks
    oss << "\nNote: Component health checks require ApplicationComponent extension\n";

    return CLIResult::ok(oss.str());
}

CLIResult CLI::cmd_messaging(const CLIContext& context) {
    std::ostringstream oss;
    bool detailed = context.has_option("detail");

    oss << "Messaging Statistics\n";
    oss << "===================\n";

    try {
        auto& bus = MessagingBus::instance();
        oss << "Message Bus: Available\n";

        if (detailed) {
            oss << "\nNote: Detailed messaging statistics require MessageBus extension\n";
            oss << "Consider adding statistics collection to MessageBus class\n";
        }

    } catch (const std::exception& e) {
        oss << "Message Bus: Error - " << e.what() << "\n";
    }

    return CLIResult::ok(oss.str());
}

CLIResult CLI::cmd_log_level(const CLIContext& context) {
    if (context.args.size() < 2) {
        std::ostringstream oss;

        // Convert current log level to string
        LogLevel current_level = Logger::get_level();
        std::string level_str;
        switch (current_level) {
            case LogLevel::Trace: level_str = "trace"; break;
            case LogLevel::Debug: level_str = "debug"; break;
            case LogLevel::Info: level_str = "info"; break;
            case LogLevel::Warn: level_str = "warn"; break;
            case LogLevel::Error: level_str = "error"; break;
            case LogLevel::Critical: level_str = "critical"; break;
            default: level_str = "unknown"; break;
        }

        oss << "Current log level: " << level_str << "\n";
        oss << "Available levels: trace, debug, info, warn, error, critical";
        return CLIResult::ok(oss.str());
    }

    const std::string& level_str = context.args[1];

    try {
        LogLevel level;
        if (level_str == "trace") level = LogLevel::Trace;
        else if (level_str == "debug") level = LogLevel::Debug;
        else if (level_str == "info") level = LogLevel::Info;
        else if (level_str == "warn") level = LogLevel::Warn;
        else if (level_str == "error") level = LogLevel::Error;
        else if (level_str == "critical") level = LogLevel::Critical;
        else {
            return CLIResult::error("Invalid log level. Available: trace, debug, info, warn, error, critical");
        }

        Logger::set_level(level);
        return CLIResult::ok("Log level changed to: " + level_str);

    } catch (const std::exception& e) {
        return CLIResult::error("Failed to set log level: " + std::string(e.what()));
    }
}

CLIResult CLI::cmd_shutdown(const CLIContext& context) {
    if (!context.app) {
        return CLIResult::error("No application context available");
    }

    Logger::info("Graceful shutdown requested via CLI");
    context.app->shutdown();
    return CLIResult::ok("Graceful shutdown initiated");
}

CLIResult CLI::cmd_force_shutdown(const CLIContext& context) {
    if (!context.app) {
        return CLIResult::error("No application context available");
    }

    Logger::warn("Force shutdown requested via CLI");
    context.app->force_shutdown();
    return CLIResult::ok("Force shutdown initiated");
}

CLIResult CLI::cmd_exit(const CLIContext& context) {
    shutdown_requested_.store(true);
    return CLIResult::ok("Exiting CLI (application continues running)");
}

std::string CLI::format_app_state(ApplicationState state) const {
    switch (state) {
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

std::string CLI::format_duration(std::chrono::nanoseconds duration) const {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    if (ms.count() > 1000) {
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
        return std::to_string(sec.count()) + "s";
    }
    return std::to_string(ms.count()) + "ms";
}

std::string CLI::format_bytes(std::size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = static_cast<double>(bytes);
    int unit = 0;

    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << units[unit];
    return oss.str();
}

} // namespace base
