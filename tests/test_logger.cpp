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


class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Shutdown any existing logger before initializing a new one
        base::Logger::shutdown();
        base::Logger::init();
    }

    void TearDown() override {
        // Clean up after each test
        base::Logger::shutdown();
    }
};

TEST_F(LoggerTest, InfoLog) {
    base::Logger::info("This is an info message");
}

TEST_F(LoggerTest, WarnLog) {
    base::Logger::warn("This is a warning message");
}

TEST_F(LoggerTest, ErrorLog) {
    base::Logger::error("This is an error message");
}

// Test modern C++20 logger features
TEST_F(LoggerTest, ModernLoggerFeatures) {
    // Shutdown the default logger and init with custom config
    base::Logger::shutdown();

    // Test with LoggerConfig
    base::LoggerConfig config{
        .app_name = "test_app",
        .level = base::LogLevel::Debug,
        .enable_console = true,
        .enable_file = false
    };

    base::Logger::init(config);

    // Test type-safe log levels
    base::Logger::set_level(base::LogLevel::Info);
    EXPECT_EQ(base::Logger::get_level(), base::LogLevel::Info);

    // Test modern logging with source location
    base::Logger::info("Modern logging test");
    base::Logger::warn("Warning with value: {}", 42);
    base::Logger::error("Error occurred");

    EXPECT_TRUE(base::Logger::is_initialized());

    // Cleanup
    base::Logger::shutdown();
    EXPECT_FALSE(base::Logger::is_initialized());
}

TEST_F(LoggerTest, StructuredLogging) {
    // Test with various types (logger already initialized in SetUp)
    base::Logger::info("String: {}, Int: {}, Float: {:.2f}", "test", 123, 3.14159);
    base::Logger::debug("Debug message with boolean: {}", true);
}