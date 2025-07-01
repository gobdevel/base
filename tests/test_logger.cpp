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
        // Initialize the logger before each test
        crux::Logger::init();
    }
};

TEST(LoggerTest, InfoLog) {
    crux::Logger::info("This is an info message");
}

TEST(LoggerTest, WarnLog) {
    crux::Logger::warn("This is a warning message");
}

TEST(LoggerTest, ErrorLog) {
    crux::Logger::error("This is an error message");
}