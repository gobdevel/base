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
        crux::Logger::shutdown();
        crux::Logger::init();
    }

    void TearDown() override {
        // Clean up after each test
        crux::Logger::shutdown();
    }
};

TEST_F(LoggerTest, InfoLog) {
    crux::Logger::info("This is an info message");
}

TEST_F(LoggerTest, WarnLog) {
    crux::Logger::warn("This is a warning message");
}

TEST_F(LoggerTest, ErrorLog) {
    crux::Logger::error("This is an error message");
}

// Test modern C++20 logger features
TEST_F(LoggerTest, ModernLoggerFeatures) {
    // Shutdown the default logger and init with custom config
    crux::Logger::shutdown();

    // Test with LoggerConfig
    crux::LoggerConfig config{
        .app_name = "test_app",
        .level = crux::LogLevel::Debug,
        .enable_console = true,
        .enable_file = false
    };

    crux::Logger::init(config);

    // Test type-safe log levels
    crux::Logger::set_level(crux::LogLevel::Info);
    EXPECT_EQ(crux::Logger::get_level(), crux::LogLevel::Info);

    // Test modern logging with source location
    crux::Logger::info("Modern logging test");
    crux::Logger::warn("Warning with value: {}", 42);
    crux::Logger::error("Error occurred");

    EXPECT_TRUE(crux::Logger::is_initialized());

    // Cleanup
    crux::Logger::shutdown();
    EXPECT_FALSE(crux::Logger::is_initialized());
}

TEST_F(LoggerTest, StructuredLogging) {
    // Test with various types (logger already initialized in SetUp)
    crux::Logger::info("String: {}, Int: {}, Float: {:.2f}", "test", 123, 3.14159);
    crux::Logger::debug("Debug message with boolean: {}", true);
}