/*
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * SPDX-License-Identifier: MIT
 */

#include <logger.h>

int main() {
    // Initialize the logger with console output
    crux::Logger::init();

    // Log messages at different levels
    crux::Logger::info("Application started");
    crux::Logger::warn("Low memory warning: {}MB remaining", 128);
    crux::Logger::error("Failed to connect to database: {}", "Connection timeout");

    return 0;
}
