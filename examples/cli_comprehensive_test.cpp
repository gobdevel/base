/*
 * @file cli_comprehensive_test.cpp
 * @brief Comprehensive CLI testing suite combining all CLI test scenarios
 *
 * This test suite includes:
 * 1. Unit tests for CLI functionality
 * 2. Automated CLI command testing
 * 3. Integration tests with Application framework
 * 4. Diagnostic tests for troubleshooting
 * 5. Performance and stress testing
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include "cli.h"
#include "logger.h"
#include "config.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <cassert>
#include <vector>
#include <sstream>

using namespace base;

class CLITestApp : public Application {
private:
    std::atomic<int> task_counter_{0};
    std::atomic<bool> worker_running_{false};

public:
    CLITestApp() : Application({
        .name = "CLI Comprehensive Test",
        .version = "1.0.0",
        .description = "Comprehensive CLI testing suite",
        .worker_threads = 1,
        .enable_health_check = true,
        .enable_cli = true,
        .cli_enable_stdin = false,  // Disable stdin to avoid blocking
        .cli_enable_tcp = false     // Disable TCP for simpler testing
    }) {}

protected:
    bool on_initialize() override {
        Logger::info("Initializing CLI comprehensive test");

        // Register test commands
        register_test_commands();

        return true;
    }

    bool on_start() override {
        Logger::info("Starting CLI comprehensive test");

        // Run all tests sequentially
        post_task([this]() {
            run_comprehensive_tests();
        });

        return true;
    }

private:
    void register_test_commands() {
        auto& cli = this->cli();

        // Test command for basic functionality
        cli.register_command("test-basic", "Basic test command", "test-basic",
            [](const CLIContext& context) -> CLIResult {
                return CLIResult::ok("Basic test command executed successfully");
            }, false);  // doesn't require app context

        // Test command with app context
        cli.register_command("test-counter", "Show test counter", "test-counter",
            [this](const CLIContext& context) -> CLIResult {
                return CLIResult::ok("Test counter: " + std::to_string(task_counter_.load()));
            });

        // Test command with arguments
        cli.register_command("test-args", "Test command with arguments", "test-args <value>",
            [this](const CLIContext& context) -> CLIResult {
                if (context.args.size() < 2) {
                    return CLIResult::error("Usage: test-args <value>");
                }

                try {
                    int value = std::stoi(context.args[1]);
                    task_counter_ += value;
                    return CLIResult::ok("Added " + std::to_string(value) + " to counter");
                } catch (const std::exception& e) {
                    return CLIResult::error("Invalid number: " + std::string(e.what()));
                }
            });

        // Test command for worker control
        cli.register_command("test-worker", "Control test worker", "test-worker [start|stop|status]",
            [this](const CLIContext& context) -> CLIResult {
                if (context.args.size() < 2) {
                    return CLIResult::ok("Worker status: " +
                        std::string(worker_running_.load() ? "Running" : "Stopped"));
                }

                const std::string& action = context.args[1];
                if (action == "start") {
                    worker_running_.store(true);
                    return CLIResult::ok("Worker started");
                } else if (action == "stop") {
                    worker_running_.store(false);
                    return CLIResult::ok("Worker stopped");
                } else if (action == "status") {
                    return CLIResult::ok("Worker status: " +
                        std::string(worker_running_.load() ? "Running" : "Stopped"));
                } else {
                    return CLIResult::error("Invalid action. Use: start, stop, or status");
                }
            });
    }

    void run_comprehensive_tests() {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "BASE CLI COMPREHENSIVE TEST SUITE\n";
        std::cout << std::string(60, '=') << "\n\n";

        bool all_tests_passed = true;

        // Test 1: Basic CLI Instance Access
        all_tests_passed &= test_cli_instance_access();

        // Test 2: Command Registration and Execution
        all_tests_passed &= test_command_registration();

        // Test 3: Built-in Commands
        all_tests_passed &= test_builtin_commands();

        // Test 4: Error Handling
        all_tests_passed &= test_error_handling();

        // Test 5: Application Integration
        all_tests_passed &= test_application_integration();

        // Test 6: Command Arguments and Options
        all_tests_passed &= test_command_arguments();

        // Test 7: Automated Command Sequence
        all_tests_passed &= test_automated_command_sequence();

        // Test 8: Diagnostic Tests
        all_tests_passed &= test_diagnostic_functionality();

        // Test 9: Stress Testing
        all_tests_passed &= test_stress_scenarios();

        // Print final results
        print_test_summary(all_tests_passed);

        // Shutdown the application
        shutdown();
    }

    bool test_cli_instance_access() {
        std::cout << "Test 1: CLI Instance Access\n";
        std::cout << std::string(30, '-') << "\n";

        try {
            auto& cli = this->cli();
            std::cout << "✅ CLI instance obtained successfully\n";
            return true;
        } catch (const std::exception& e) {
            std::cout << "❌ Failed to access CLI instance: " << e.what() << "\n";
            return false;
        }
    }

    bool test_command_registration() {
        std::cout << "\nTest 2: Command Registration and Execution\n";
        std::cout << std::string(40, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test basic command execution
        auto result = cli.execute_command("test-basic");
        if (result.success && result.output.find("Basic test command executed successfully") != std::string::npos) {
            std::cout << "✅ Basic command registration and execution working\n";
        } else {
            std::cout << "❌ Basic command registration failed\n";
            success = false;
        }

        // Test command with app context
        result = cli.execute_command("test-counter");
        if (result.success && result.output.find("Test counter:") != std::string::npos) {
            std::cout << "✅ Command with application context working\n";
        } else {
            std::cout << "❌ Command with application context failed\n";
            success = false;
        }

        return success;
    }

    bool test_builtin_commands() {
        std::cout << "\nTest 3: Built-in Commands\n";
        std::cout << std::string(25, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test help command
        auto result = cli.execute_command("help");
        if (result.success && result.output.find("Available commands") != std::string::npos) {
            std::cout << "✅ Built-in help command working\n";
        } else {
            std::cout << "❌ Built-in help command failed\n";
            success = false;
        }

        // Test status command
        result = cli.execute_command("status");
        if (result.success && result.output.find("Application Status") != std::string::npos) {
            std::cout << "✅ Built-in status command working\n";
        } else {
            std::cout << "❌ Built-in status command failed\n";
            success = false;
        }

        // Test health command
        result = cli.execute_command("health");
        if (result.success && result.output.find("Health Check") != std::string::npos) {
            std::cout << "✅ Built-in health command working\n";
        } else {
            std::cout << "❌ Built-in health command failed\n";
            success = false;
        }

        // Test config command
        result = cli.execute_command("config");
        if (result.success && result.output.find("Configuration") != std::string::npos) {
            std::cout << "✅ Built-in config command working\n";
        } else {
            std::cout << "❌ Built-in config command failed\n";
            success = false;
        }

        return success;
    }

    bool test_error_handling() {
        std::cout << "\nTest 4: Error Handling\n";
        std::cout << std::string(23, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test invalid command
        auto result = cli.execute_command("invalid-command-xyz");
        if (!result.success && result.error_message.find("Unknown command") != std::string::npos) {
            std::cout << "✅ Error handling for invalid commands working\n";
        } else {
            std::cout << "❌ Error handling for invalid commands failed\n";
            success = false;
        }

        // Test empty command
        result = cli.execute_command("");
        if (result.success) {
            std::cout << "✅ Empty command handling working\n";
        } else {
            std::cout << "❌ Empty command handling failed\n";
            success = false;
        }

        // Test command with invalid arguments
        result = cli.execute_command("test-args invalid");
        if (!result.success && result.error_message.find("Invalid number") != std::string::npos) {
            std::cout << "✅ Invalid argument error handling working\n";
        } else {
            std::cout << "❌ Invalid argument error handling failed\n";
            success = false;
        }

        return success;
    }

    bool test_application_integration() {
        std::cout << "\nTest 5: Application Integration\n";
        std::cout << std::string(32, '-') << "\n";

        bool success = true;

        // Test CLI enabled check
        if (is_cli_enabled()) {
            std::cout << "✅ Application CLI integration working\n";
        } else {
            std::cout << "❌ Application CLI integration failed\n";
            success = false;
        }

        // Test CLI singleton access through application
        try {
            auto& app_cli = this->cli();
            auto result = app_cli.execute_command("help");
            if (result.success) {
                std::cout << "✅ CLI singleton access through application working\n";
            } else {
                std::cout << "❌ CLI singleton access through application failed\n";
                success = false;
            }
        } catch (const std::exception& e) {
            std::cout << "❌ Exception in application CLI integration: " << e.what() << "\n";
            success = false;
        }

        return success;
    }

    bool test_command_arguments() {
        std::cout << "\nTest 6: Command Arguments and Options\n";
        std::cout << std::string(37, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test command with arguments
        auto result = cli.execute_command("test-args 42");
        if (result.success && result.output.find("Added 42 to counter") != std::string::npos) {
            std::cout << "✅ Command with arguments working\n";
        } else {
            std::cout << "❌ Command with arguments failed\n";
            success = false;
        }

        // Test worker control commands
        result = cli.execute_command("test-worker start");
        if (result.success && result.output.find("Worker started") != std::string::npos) {
            std::cout << "✅ Worker control command working\n";
        } else {
            std::cout << "❌ Worker control command failed\n";
            success = false;
        }

        result = cli.execute_command("test-worker status");
        if (result.success && result.output.find("Worker status: Running") != std::string::npos) {
            std::cout << "✅ Worker status command working\n";
        } else {
            std::cout << "❌ Worker status command failed\n";
            success = false;
        }

        return success;
    }

    bool test_automated_command_sequence() {
        std::cout << "\nTest 7: Automated Command Sequence\n";
        std::cout << std::string(35, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test sequence of commands
        std::vector<std::string> test_commands = {
            "help",
            "status",
            "test-counter",
            "test-args 10",
            "test-worker stop",
            "config",
            "health"
        };

        int passed = 0;
        for (const auto& cmd : test_commands) {
            auto result = cli.execute_command(cmd);
            if (result.success) {
                passed++;
            }
        }

        if (passed == test_commands.size()) {
            std::cout << "✅ All " << test_commands.size() << " commands in sequence executed successfully\n";
        } else {
            std::cout << "❌ Only " << passed << "/" << test_commands.size() << " commands succeeded\n";
            success = false;
        }

        return success;
    }

    bool test_diagnostic_functionality() {
        std::cout << "\nTest 8: Diagnostic Functionality\n";
        std::cout << std::string(33, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test help for specific command
        auto result = cli.execute_command("help test-args");
        if (result.success && result.output.find("test-args") != std::string::npos) {
            std::cout << "✅ Specific command help working\n";
        } else {
            std::cout << "❌ Specific command help failed\n";
            success = false;
        }

        // Test threads command
        result = cli.execute_command("threads");
        if (result.success && result.output.find("Thread") != std::string::npos) {
            std::cout << "✅ Threads diagnostic command working\n";
        } else {
            std::cout << "❌ Threads diagnostic command failed\n";
            success = false;
        }

        return success;
    }

    bool test_stress_scenarios() {
        std::cout << "\nTest 9: Stress Testing\n";
        std::cout << std::string(23, '-') << "\n";

        auto& cli = this->cli();
        bool success = true;

        // Test rapid command execution
        auto start_time = std::chrono::steady_clock::now();
        int stress_commands = 50;
        int successful = 0;

        for (int i = 0; i < stress_commands; ++i) {
            auto result = cli.execute_command("test-counter");
            if (result.success) {
                successful++;
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        if (successful == stress_commands) {
            std::cout << "✅ Stress test passed: " << stress_commands << " commands in "
                      << duration.count() << "ms\n";
        } else {
            std::cout << "❌ Stress test failed: " << successful << "/" << stress_commands
                      << " commands succeeded\n";
            success = false;
        }

        return success;
    }

    void print_test_summary(bool all_passed) {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << std::string(60, '=') << "\n\n";

        if (all_passed) {
            std::cout << "🎉 ALL TESTS PASSED! 🎉\n\n";
            std::cout << "CLI feature is fully functional with:\n";
        } else {
            std::cout << "⚠️  SOME TESTS FAILED ⚠️\n\n";
            std::cout << "CLI feature has issues. Check the test output above.\n\n";
            std::cout << "Expected features:\n";
        }

        std::cout << "• Command registration and execution\n";
        std::cout << "• Built-in commands (help, status, health, config, threads)\n";
        std::cout << "• Custom commands with arguments\n";
        std::cout << "• Error handling and validation\n";
        std::cout << "• Application integration\n";
        std::cout << "• Thread-safe operation\n";
        std::cout << "• Performance under load\n\n";

        std::cout << "For interactive testing, run: ./build/Release/examples/cli_example\n";
        std::cout << "For remote testing: telnet localhost 8080 (when cli_example is running)\n\n";
    }
};

int main() {
    try {
        Logger::set_level(LogLevel::Info);

        std::cout << "Starting CLI Comprehensive Test Suite...\n";

        CLITestApp app;
        int result = app.run();

        std::cout << "Test suite completed with exit code: " << result << std::endl;
        return result;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error in CLI test suite: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
