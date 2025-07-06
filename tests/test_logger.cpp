/*
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * SPDX-License-Identifier: MIT
 */
#include <logger.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <chrono>

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Shutdown any existing logger before initializing a new one
        base::Logger::shutdown();
        base::Logger::init();
        test_log_file_ = "test_log.log";
    }

    void TearDown() override {
        // Clean up after each test
        base::Logger::shutdown();
        // Remove test log file if it exists
        if (std::filesystem::exists(test_log_file_)) {
            std::filesystem::remove(test_log_file_);
        }
    }

    std::string test_log_file_;
};

// ======================================
// BASIC LOGGING TESTS
// ======================================

TEST_F(LoggerTest, BasicLogging) {
    base::Logger::info("This is an info message");
    base::Logger::warn("This is a warning message");
    base::Logger::error("This is an error message");
    base::Logger::debug("This is a debug message");
    base::Logger::trace("This is a trace message");
    base::Logger::critical("This is a critical message");
}

TEST_F(LoggerTest, StructuredLogging) {
    base::Logger::info("String: {}, Int: {}, Float: {:.2f}", "test", 123, 3.14159);
    base::Logger::debug("Debug message with boolean: {}", true);
    base::Logger::warn("Multiple args: {} {} {} {}", 1, 2.5, "string", true);
    base::Logger::error("Complex format: {:#x} {:.3f} {:<10}", 255, 3.14159, "left");
}

// ======================================
// INITIALIZATION AND LIFECYCLE TESTS
// ======================================

TEST_F(LoggerTest, InitializationStates) {
    // Test default initialization
    EXPECT_TRUE(base::Logger::is_initialized());
    EXPECT_TRUE(base::Logger::ready());

    // Test shutdown
    base::Logger::shutdown();
    EXPECT_FALSE(base::Logger::is_initialized());
    EXPECT_FALSE(base::Logger::ready());

    // Test re-initialization
    base::Logger::init();
    EXPECT_TRUE(base::Logger::is_initialized());
}

TEST_F(LoggerTest, ConfigurationInitialization) {
    base::Logger::shutdown();

    // Test with LoggerConfig
    base::LoggerConfig config{
        .app_name = "test_app",
        .level = base::LogLevel::Debug,
        .enable_console = true,
        .enable_file = false,
        .enable_colors = true,
        .pattern = "[%Y-%m-%d %H:%M:%S] [%l] %v"
    };

    base::Logger::init(config);
    EXPECT_TRUE(base::Logger::is_initialized());
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Debug);
}

TEST_F(LoggerTest, FileLoggingConfiguration) {
    base::Logger::shutdown();

    base::LoggerConfig config{
        .app_name = "file_test",
        .log_file = test_log_file_,
        .max_file_size = 1024,
        .max_files = 2,
        .level = base::LogLevel::Info,
        .enable_console = false,
        .enable_file = true
    };

    base::Logger::init(config);
    EXPECT_TRUE(base::Logger::is_initialized());

    // Write some logs
    base::Logger::info("File log test message");
    base::Logger::warn("Warning to file");
    base::Logger::flush();

    // Check if file was created and contains logs
    EXPECT_TRUE(std::filesystem::exists(test_log_file_));

    std::ifstream log_file(test_log_file_);
    std::string content((std::istreambuf_iterator<char>(log_file)),
                        std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("File log test message"), std::string::npos);
}

// ======================================
// LOG LEVEL MANAGEMENT TESTS
// ======================================

TEST_F(LoggerTest, LogLevelManagement) {
    // Test all log levels
    base::Logger::set_level(base::LogLevel::Trace);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Trace);

    base::Logger::set_level(base::LogLevel::Debug);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Debug);

    base::Logger::set_level(base::LogLevel::Info);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Info);

    base::Logger::set_level(base::LogLevel::Warn);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Warn);

    base::Logger::set_level(base::LogLevel::Error);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Error);

    base::Logger::set_level(base::LogLevel::Critical);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Critical);

    base::Logger::set_level(base::LogLevel::Off);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Off);
}

TEST_F(LoggerTest, LogLevelFiltering) {
    // Set level to Warn - should filter out trace, debug, info
    base::Logger::set_level(base::LogLevel::Warn);

    // These should not be logged (but we can't easily test console output)
    base::Logger::trace("This trace should be filtered");
    base::Logger::debug("This debug should be filtered");
    base::Logger::info("This info should be filtered");

    // These should be logged
    base::Logger::warn("This warning should appear");
    base::Logger::error("This error should appear");
    base::Logger::critical("This critical should appear");
}

// ======================================
// COMPONENT LOGGING TESTS
// ======================================

TEST_F(LoggerTest, ComponentLogging) {
    // Test Component wrapper
    auto network = base::Logger::component("Network");
    auto database = base::Logger::component("Database");

    base::Logger::info(network, "Connected to server {}", "192.168.1.1");
    base::Logger::error(database, "Connection failed: {}", "timeout");
    base::Logger::debug(network, "Received {} bytes", 1024);
}

TEST_F(LoggerTest, ComponentFiltering) {
    // Enable only specific components
    base::Logger::enable_components({"Network", "Security"});

    auto enabled_comps = base::Logger::get_enabled_components();
    EXPECT_EQ(enabled_comps.size(), 2);
    EXPECT_NE(std::find(enabled_comps.begin(), enabled_comps.end(), "Network"), enabled_comps.end());
    EXPECT_NE(std::find(enabled_comps.begin(), enabled_comps.end(), "Security"), enabled_comps.end());

    // Test component filtering
    EXPECT_TRUE(base::Logger::is_component_enabled("Network"));
    EXPECT_TRUE(base::Logger::is_component_enabled("Security"));
    EXPECT_FALSE(base::Logger::is_component_enabled("Database"));

    // Disable specific components
    base::Logger::disable_components({"Debug", "Verbose"});
    auto disabled_comps = base::Logger::get_disabled_components();
    EXPECT_EQ(disabled_comps.size(), 2);
    EXPECT_NE(std::find(disabled_comps.begin(), disabled_comps.end(), "Debug"), disabled_comps.end());
    EXPECT_NE(std::find(disabled_comps.begin(), disabled_comps.end(), "Verbose"), disabled_comps.end());

    EXPECT_FALSE(base::Logger::is_component_enabled("Debug"));
    EXPECT_FALSE(base::Logger::is_component_enabled("Verbose"));

    // Clear filters
    base::Logger::clear_component_filters();
    EXPECT_TRUE(base::Logger::get_enabled_components().empty());
    EXPECT_TRUE(base::Logger::get_disabled_components().empty());
    EXPECT_TRUE(base::Logger::is_component_enabled("AnyComponent"));
}

TEST_F(LoggerTest, ComponentLoggerClass) {
    // Test ComponentLogger class
    auto db_logger = base::Logger::get_component_logger("Database");
    auto net_logger = base::Logger::get_component_logger("Network");

    EXPECT_EQ(db_logger.get_component_name(), "Database");
    EXPECT_EQ(net_logger.get_component_name(), "Network");

    // Test all logging levels with ComponentLogger
    db_logger.trace("Database trace message");
    db_logger.debug("Database debug message");
    db_logger.info("Database connected successfully");
    db_logger.warn("Database connection slow: {}ms", 500);
    db_logger.error("Database query failed: {}", "syntax error");
    db_logger.critical("Database corruption detected");

    net_logger.info("Network connection established");
    net_logger.warn("High latency detected: {}ms", 200);
}

TEST_F(LoggerTest, ComponentMacros) {
    // Test convenience macros
    COMPONENT_LOGGER(network);
    COMPONENT_LOGGER_NAMED(db, "Database");

    EXPECT_EQ(network.get_component_name(), "network");
    EXPECT_EQ(db.get_component_name(), "Database");

    network.info("Macro test message");
    db.warn("Database warning via macro");
}

// ======================================
// THREAD SAFETY TESTS
// ======================================

TEST_F(LoggerTest, ThreadSafety) {
    const int num_threads = 4;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;

    // Launch multiple threads that log concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, messages_per_thread]() {
            auto logger = base::Logger::get_component_logger("Thread" + std::to_string(i));
            for (int j = 0; j < messages_per_thread; ++j) {
                logger.info("Message {} from thread {}", j, i);
                base::Logger::debug("Global message {} from thread {}", j, i);
            }
        });
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Logger should still be operational
    EXPECT_TRUE(base::Logger::is_initialized());
    base::Logger::info("Thread safety test completed");
}

// ======================================
// PERFORMANCE AND STRESS TESTS
// ======================================

TEST_F(LoggerTest, HighVolumeLogging) {
    const int num_messages = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_messages; ++i) {
        base::Logger::info("High volume message {}", i);
    }

    base::Logger::flush();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Logging 1000 messages should complete reasonably quickly (< 1 second)
    EXPECT_LT(duration.count(), 1000);
}

TEST_F(LoggerTest, ComponentFilteringPerformance) {
    // Enable only one component
    base::Logger::enable_components({"Enabled"});

    const int num_messages = 1000;
    auto enabled_logger = base::Logger::get_component_logger("Enabled");
    auto disabled_logger = base::Logger::get_component_logger("Disabled");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_messages; ++i) {
        enabled_logger.info("Enabled message {}", i);
        disabled_logger.info("Disabled message {}", i);  // Should be filtered out
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Component filtering should not significantly impact performance
    EXPECT_LT(duration.count(), 1000);
}

// ======================================
// ERROR HANDLING AND EDGE CASES
// ======================================

TEST_F(LoggerTest, UninitializedLogger) {
    base::Logger::shutdown();

    // Logging should not crash when logger is not initialized
    base::Logger::info("This should not crash");
    base::Logger::error("Error when uninitialized");

    EXPECT_FALSE(base::Logger::is_initialized());
}

TEST_F(LoggerTest, EmptyMessages) {
    // Test logging empty messages
    base::Logger::info("");
    base::Logger::warn("");
    base::Logger::error("");

    // Test logging with empty format args
    base::Logger::info("{}", "");
    base::Logger::debug("{} {}", "", "");
}

TEST_F(LoggerTest, SpecialCharacters) {
    // Test logging messages with special characters
    base::Logger::info("Message with unicode: αβγδε");
    base::Logger::warn("Message with symbols: !@#$%^&*()");
    base::Logger::error("Message with newlines:\nLine 1\nLine 2");
    base::Logger::debug("Message with tabs:\tTabbed\tcontent");
}

TEST_F(LoggerTest, LargeMessages) {
    // Test logging very large messages
    std::string large_message(10000, 'X');
    base::Logger::info("Large message: {}", large_message);

    // Test with many format arguments
    base::Logger::debug("Many args: {} {} {} {} {} {} {} {} {} {}",
                        1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}

// ======================================
// FLUSH AND SHUTDOWN TESTS
// ======================================

TEST_F(LoggerTest, FlushOperation) {
    base::Logger::info("Message before flush");
    base::Logger::flush();  // Should not crash
    base::Logger::warn("Message after flush");
}

TEST_F(LoggerTest, MultipleShutdowns) {
    EXPECT_TRUE(base::Logger::is_initialized());

    base::Logger::shutdown();
    EXPECT_FALSE(base::Logger::is_initialized());

    // Multiple shutdowns should be safe
    base::Logger::shutdown();
    base::Logger::shutdown();
    EXPECT_FALSE(base::Logger::is_initialized());
}

TEST_F(LoggerTest, InitAfterShutdown) {
    base::Logger::shutdown();
    EXPECT_FALSE(base::Logger::is_initialized());

    base::Logger::init();
    EXPECT_TRUE(base::Logger::is_initialized());

    base::Logger::info("Message after reinit");
}