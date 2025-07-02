/*
 * @file cli_unit_tests.cpp
 * @brief Comprehensive unit tests for CLI functionality
 *
 * This file contains detailed unit tests for all CLI components:
 * 1. CLI class instantiation and lifecycle
 * 2. Command registration and management
 * 3. Command execution and result handling
 * 4. Context parsing and argument handling
 * 5. Error handling and edge cases
 * 6. Thread safety and concurrent access
 * 7. Built-in command functionality
 * 8. Integration with Application framework
 * 9. Memory management and resource cleanup
 * 10. Performance characteristics
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "cli.h"
#include "application.h"
#include "logger.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <future>
#include <sstream>

using namespace crux;

class CLIUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up clean state for each test
        Logger::set_level(LogLevel::Error); // Reduce noise in tests

        // Reset CLI singleton state by creating a fresh instance
        cli_ = &CLI::instance();

        // Clear any existing commands (except built-ins)
        // Note: This assumes CLI has a way to reset state
        test_counter_ = 0;
    }

    void TearDown() override {
        // Clean up after each test
        test_counter_ = 0;
    }

    CLI* cli_;
    std::atomic<int> test_counter_{0};

    // Helper method to register a test command
    void register_test_command(const std::string& name,
                              const std::string& description,
                              const std::string& usage,
                              std::function<CLIResult(const CLIContext&)> handler,
                              bool requires_app_context = true) {
        cli_->register_command(name, description, usage, handler, requires_app_context);
    }
};

// ============================================================================
// CLI Instance Tests
// ============================================================================

TEST_F(CLIUnitTest, SingletonInstance) {
    // Test that CLI follows singleton pattern
    CLI& cli1 = CLI::instance();
    CLI& cli2 = CLI::instance();

    EXPECT_EQ(&cli1, &cli2) << "CLI should be a singleton";
    EXPECT_EQ(cli_, &cli1) << "CLI instance should be consistent";
}

TEST_F(CLIUnitTest, InstanceThreadSafety) {
    // Test that singleton instance is thread-safe
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<CLI*> instances(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&instances, i]() {
            instances[i] = &CLI::instance();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All instances should be the same
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(instances[0], instances[i])
            << "All CLI instances should be identical";
    }
}

// ============================================================================
// Command Registration Tests
// ============================================================================

TEST_F(CLIUnitTest, BasicCommandRegistration) {
    // Test basic command registration
    bool handler_called = false;

    register_test_command("test-basic", "Test command", "test-basic",
        [&handler_called](const CLIContext& context) -> CLIResult {
            handler_called = true;
            return CLIResult::ok("Command executed");
        }, false);

    auto result = cli_->execute_command("test-basic");

    EXPECT_TRUE(result.success) << "Command should execute successfully";
    EXPECT_EQ(result.output, "Command executed") << "Command output should match";
    EXPECT_TRUE(handler_called) << "Command handler should be called";
}

TEST_F(CLIUnitTest, CommandRegistrationWithAppContext) {
    // Test command registration that requires application context
    register_test_command("test-app-context", "Test app context", "test-app-context",
        [this](const CLIContext& context) -> CLIResult {
            test_counter_++;
            return CLIResult::ok("Counter: " + std::to_string(test_counter_.load()));
        });

    auto result = cli_->execute_command("test-app-context");

    EXPECT_TRUE(result.success) << "App context command should execute";
    EXPECT_EQ(test_counter_.load(), 1) << "Counter should be incremented";
    EXPECT_NE(result.output.find("Counter: 1"), std::string::npos)
        << "Output should contain counter value";
}

TEST_F(CLIUnitTest, DuplicateCommandRegistration) {
    // Test registering the same command twice
    register_test_command("duplicate", "First registration", "duplicate",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("First");
        }, false);

    // Second registration should either overwrite or fail gracefully
    register_test_command("duplicate", "Second registration", "duplicate",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("Second");
        }, false);

    auto result = cli_->execute_command("duplicate");
    EXPECT_TRUE(result.success) << "Duplicate registration should be handled gracefully";
    // The behavior of which command is executed is implementation-dependent
}

TEST_F(CLIUnitTest, EmptyCommandName) {
    // Test registering command with empty name
    EXPECT_NO_THROW({
        register_test_command("", "Empty name command", "",
            [](const CLIContext& context) -> CLIResult {
                return CLIResult::ok("Empty name");
            }, false);
    }) << "Empty command name should be handled gracefully";
}

// ============================================================================
// Command Execution Tests
// ============================================================================

TEST_F(CLIUnitTest, ValidCommandExecution) {
    register_test_command("valid-cmd", "Valid command", "valid-cmd",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("Valid execution");
        }, false);

    auto result = cli_->execute_command("valid-cmd");

    EXPECT_TRUE(result.success) << "Valid command should succeed";
    EXPECT_EQ(result.output, "Valid execution") << "Output should match handler return";
    EXPECT_TRUE(result.error_message.empty()) << "Error message should be empty on success";
}

TEST_F(CLIUnitTest, InvalidCommandExecution) {
    auto result = cli_->execute_command("non-existent-command");

    EXPECT_FALSE(result.success) << "Invalid command should fail";
    EXPECT_TRUE(result.output.empty()) << "Output should be empty on failure";
    EXPECT_FALSE(result.error_message.empty()) << "Error message should be provided";
    EXPECT_NE(result.error_message.find("Unknown command"), std::string::npos)
        << "Error should mention unknown command";
}

TEST_F(CLIUnitTest, EmptyCommandExecution) {
    auto result = cli_->execute_command("");

    // Empty command should be handled gracefully (implementation-dependent behavior)
    EXPECT_NO_THROW(cli_->execute_command("")) << "Empty command should not throw";
}

TEST_F(CLIUnitTest, WhitespaceCommandExecution) {
    auto result = cli_->execute_command("   ");

    // Whitespace-only command should be handled gracefully
    EXPECT_NO_THROW(cli_->execute_command("   ")) << "Whitespace command should not throw";
}

// ============================================================================
// Command Context and Arguments Tests
// ============================================================================

TEST_F(CLIUnitTest, ArgumentParsing) {
    register_test_command("args-test", "Test arguments", "args-test <arg1> <arg2>",
        [](const CLIContext& context) -> CLIResult {
            std::ostringstream oss;
            oss << "Args: ";
            for (size_t i = 0; i < context.args.size(); ++i) {
                if (i > 0) oss << ",";
                oss << context.args[i];
            }
            return CLIResult::ok(oss.str());
        }, false);

    auto result = cli_->execute_command("args-test hello world 123");

    EXPECT_TRUE(result.success) << "Command with arguments should succeed";
    EXPECT_NE(result.output.find("args-test"), std::string::npos)
        << "Output should contain command name";
    EXPECT_NE(result.output.find("hello"), std::string::npos)
        << "Output should contain first argument";
    EXPECT_NE(result.output.find("world"), std::string::npos)
        << "Output should contain second argument";
    EXPECT_NE(result.output.find("123"), std::string::npos)
        << "Output should contain third argument";
}

TEST_F(CLIUnitTest, QuotedArgumentParsing) {
    register_test_command("quoted-test", "Test quoted args", "quoted-test <arg>",
        [](const CLIContext& context) -> CLIResult {
            if (context.args.size() >= 2) {
                return CLIResult::ok("Quoted arg: " + context.args[1]);
            }
            return CLIResult::error("No argument provided");
        }, false);

    auto result = cli_->execute_command("quoted-test \"hello world\"");

    EXPECT_TRUE(result.success) << "Quoted arguments should be parsed correctly";
    EXPECT_NE(result.output.find("hello world"), std::string::npos)
        << "Quoted argument should preserve spaces";
}

TEST_F(CLIUnitTest, SpecialCharacterArguments) {
    register_test_command("special-test", "Test special chars", "special-test <arg>",
        [](const CLIContext& context) -> CLIResult {
            if (context.args.size() >= 2) {
                return CLIResult::ok("Special: " + context.args[1]);
            }
            return CLIResult::error("No argument");
        }, false);

    // Test various special characters
    std::vector<std::string> special_chars = {
        "test@example.com",
        "path/to/file",
        "value=123",
        "host:port",
        "file.ext"
    };

    for (const auto& special_char : special_chars) {
        auto result = cli_->execute_command("special-test " + special_char);
        EXPECT_TRUE(result.success) << "Special character argument should work: " << special_char;
        EXPECT_NE(result.output.find(special_char), std::string::npos)
            << "Output should contain special character: " << special_char;
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(CLIUnitTest, CommandHandlerException) {
    register_test_command("exception-test", "Test exception handling", "exception-test",
        [](const CLIContext& context) -> CLIResult {
            throw std::runtime_error("Test exception");
        }, false);

    auto result = cli_->execute_command("exception-test");

    EXPECT_FALSE(result.success) << "Command throwing exception should fail";
    EXPECT_FALSE(result.error_message.empty()) << "Error message should be provided";
    EXPECT_NE(result.error_message.find("exception"), std::string::npos)
        << "Error should mention exception";
}

TEST_F(CLIUnitTest, ErrorResultHandling) {
    register_test_command("error-test", "Test error result", "error-test",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::error("Test error message");
        }, false);

    auto result = cli_->execute_command("error-test");

    EXPECT_FALSE(result.success) << "Error result should indicate failure";
    EXPECT_EQ(result.error_message, "Test error message") << "Error message should match";
    EXPECT_TRUE(result.output.empty()) << "Output should be empty on error";
}

TEST_F(CLIUnitTest, SuccessResultHandling) {
    register_test_command("success-test", "Test success result", "success-test",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("Success message");
        }, false);

    auto result = cli_->execute_command("success-test");

    EXPECT_TRUE(result.success) << "Success result should indicate success";
    EXPECT_EQ(result.output, "Success message") << "Output should match";
    EXPECT_TRUE(result.error_message.empty()) << "Error message should be empty";
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(CLIUnitTest, ConcurrentCommandRegistration) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> registration_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &registration_count]() {
            std::string cmd_name = "concurrent-cmd-" + std::to_string(i);
            register_test_command(cmd_name, "Concurrent command", cmd_name,
                [i](const CLIContext& context) -> CLIResult {
                    return CLIResult::ok("Thread " + std::to_string(i));
                }, false);
            registration_count++;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(registration_count.load(), num_threads)
        << "All concurrent registrations should complete";

    // Test that all registered commands work
    for (int i = 0; i < num_threads; ++i) {
        std::string cmd_name = "concurrent-cmd-" + std::to_string(i);
        auto result = cli_->execute_command(cmd_name);
        EXPECT_TRUE(result.success) << "Concurrently registered command should work: " << cmd_name;
    }
}

TEST_F(CLIUnitTest, ConcurrentCommandExecution) {
    register_test_command("concurrent-exec", "Concurrent execution test", "concurrent-exec",
        [this](const CLIContext& context) -> CLIResult {
            test_counter_++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Simulate work
            return CLIResult::ok("Executed: " + std::to_string(test_counter_.load()));
        }, false);

    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<CLIResult> results(num_threads);
    std::atomic<int> completed{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i, &results, &completed]() {
            results[i] = cli_->execute_command("concurrent-exec");
            completed++;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(completed.load(), num_threads) << "All concurrent executions should complete";

    // All executions should succeed
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_TRUE(results[i].success) << "Concurrent execution " << i << " should succeed";
    }

    EXPECT_EQ(test_counter_.load(), num_threads)
        << "Counter should reflect all executions";
}

// ============================================================================
// Built-in Commands Tests
// ============================================================================

TEST_F(CLIUnitTest, HelpCommand) {
    auto result = cli_->execute_command("help");

    EXPECT_TRUE(result.success) << "Help command should succeed";
    EXPECT_FALSE(result.output.empty()) << "Help should provide output";
    EXPECT_NE(result.output.find("commands"), std::string::npos)
        << "Help should mention commands";
}

TEST_F(CLIUnitTest, HelpWithSpecificCommand) {
    register_test_command("help-target", "Command for help test", "help-target <arg>",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("Help target executed");
        }, false);

    auto result = cli_->execute_command("help help-target");

    EXPECT_TRUE(result.success) << "Help with specific command should succeed";
    EXPECT_NE(result.output.find("help-target"), std::string::npos)
        << "Help should contain target command name";
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(CLIUnitTest, CommandExecutionPerformance) {
    register_test_command("perf-test", "Performance test command", "perf-test",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("Fast execution");
        }, false);

    const int num_executions = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_executions; ++i) {
        auto result = cli_->execute_command("perf-test");
        ASSERT_TRUE(result.success) << "Performance test execution should succeed";
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Should be able to execute at least 1000 commands per second
    EXPECT_LT(duration.count(), 1000000) << "1000 executions should complete within 1 second";

    double executions_per_second = (num_executions * 1000000.0) / duration.count();
    std::cout << "Command execution performance: " << executions_per_second
              << " executions/second" << std::endl;
}

TEST_F(CLIUnitTest, MemoryUsageStability) {
    register_test_command("memory-test", "Memory usage test", "memory-test",
        [](const CLIContext& context) -> CLIResult {
            // Create some temporary data
            std::string large_output(1000, 'x');
            return CLIResult::ok(large_output);
        }, false);

    // Execute many commands to test for memory leaks
    const int num_executions = 10000;

    for (int i = 0; i < num_executions; ++i) {
        auto result = cli_->execute_command("memory-test");
        ASSERT_TRUE(result.success) << "Memory test execution should succeed";

        // Periodically check that we're not accumulating massive amounts of memory
        if (i % 1000 == 0) {
            // This is a basic check - in a real scenario you'd use more sophisticated memory monitoring
            EXPECT_NO_THROW(cli_->execute_command("help"))
                << "CLI should remain functional after many executions";
        }
    }
}

// ============================================================================
// Integration Tests with Mock Application
// ============================================================================

class MockCLIApp : public Application {
public:
    MockCLIApp() : Application({
        .name = "Mock CLI App",
        .version = "1.0.0",
        .description = "Mock app for CLI testing",
        .worker_threads = 1,
        .enable_cli = true,
        .cli_enable_stdin = false,
        .cli_enable_tcp = false
    }) {}

protected:
    bool on_initialize() override {
        auto& cli = this->cli();
        cli.register_command("mock-cmd", "Mock command", "mock-cmd",
            [this](const CLIContext& context) -> CLIResult {
                return CLIResult::ok("Mock app command executed");
            });
        return true;
    }

    bool on_start() override {
        return true;
    }

    bool on_stop() override {
        return true;
    }
};

TEST_F(CLIUnitTest, ApplicationIntegration) {
    MockCLIApp app;

    // Test that CLI is accessible through application
    EXPECT_TRUE(app.is_cli_enabled()) << "CLI should be enabled in mock app";

    auto& app_cli = app.cli();
    EXPECT_EQ(&app_cli, cli_) << "Application CLI should be same singleton instance";

    // Test application-registered command
    auto result = app_cli.execute_command("mock-cmd");
    EXPECT_TRUE(result.success) << "Application-registered command should work";
    EXPECT_EQ(result.output, "Mock app command executed")
        << "Command output should match";
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST_F(CLIUnitTest, VeryLongCommandName) {
    std::string long_name(1000, 'a'); // 1000 character command name

    EXPECT_NO_THROW({
        register_test_command(long_name, "Very long command name", long_name,
            [](const CLIContext& context) -> CLIResult {
                return CLIResult::ok("Long name command");
            }, false);
    }) << "Very long command name should be handled gracefully";

    auto result = cli_->execute_command(long_name);
    EXPECT_TRUE(result.success) << "Command with very long name should execute";
}

TEST_F(CLIUnitTest, VeryLongCommandOutput) {
    std::string long_output(100000, 'x'); // 100KB output

    register_test_command("long-output", "Command with long output", "long-output",
        [long_output](const CLIContext& context) -> CLIResult {
            return CLIResult::ok(long_output);
        }, false);

    auto result = cli_->execute_command("long-output");
    EXPECT_TRUE(result.success) << "Command with very long output should succeed";
    EXPECT_EQ(result.output.length(), long_output.length())
        << "Output length should be preserved";
}

TEST_F(CLIUnitTest, ManyArguments) {
    register_test_command("many-args", "Command with many arguments", "many-args <args...>",
        [](const CLIContext& context) -> CLIResult {
            return CLIResult::ok("Args count: " + std::to_string(context.args.size() - 1));
        }, false);

    // Create command with 100 arguments
    std::string command = "many-args";
    for (int i = 0; i < 100; ++i) {
        command += " arg" + std::to_string(i);
    }

    auto result = cli_->execute_command(command);
    EXPECT_TRUE(result.success) << "Command with many arguments should succeed";
    EXPECT_NE(result.output.find("100"), std::string::npos)
        << "Should handle 100 arguments correctly";
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Set up logging for tests
    Logger::set_level(LogLevel::Error);

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "CRUX CLI UNIT TEST SUITE\n";
    std::cout << std::string(60, '=') << "\n\n";

    int result = RUN_ALL_TESTS();

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "CLI UNIT TESTS COMPLETED\n";
    std::cout << std::string(60, '=') << "\n\n";

    return result;
}
