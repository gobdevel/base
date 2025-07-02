/*
 * @file test_config.cpp
 * @brief Comprehensive tests for the TOML configuration system
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include "config.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace crux;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing configuration
        ConfigManager::instance().clear();

        // Clean up any test files
        cleanup_test_files();
    }

    void TearDown() override {
        ConfigManager::instance().clear();
        cleanup_test_files();
    }

    void cleanup_test_files() {
        std::filesystem::remove("test_config.toml");
        std::filesystem::remove("test_template.toml");
        std::filesystem::remove_all("test_logs");
    }

    std::string sample_config_content() {
        return R"(
[myapp]

[myapp.app]
name = "test_application"
version = "2.0.0"
description = "Test application for config system"
debug_mode = true
worker_threads = 8
working_directory = "/tmp/test"

[myapp.logging]
level = "debug"
pattern = "[%Y-%m-%d %H:%M:%S] [%l] %v"
enable_console = true
enable_file = true
file_path = "test_logs/app.log"
max_file_size = 5242880
max_files = 3
flush_immediately = true

[myapp.network]
host = "0.0.0.0"
port = 9090
timeout_seconds = 60
max_connections = 200
enable_ssl = true
ssl_cert_path = "/etc/ssl/cert.pem"
ssl_key_path = "/etc/ssl/key.pem"

[myapp.database]
host = "db.example.com"
port = 5432
name = "testdb"
user = "testuser"
password = "testpass"
max_connections = 20

[myapp.cache]
redis_host = "cache.example.com"
redis_port = 6379
ttl_seconds = 7200

# Another app configuration
[otherapp]

[otherapp.app]
name = "other_app"
version = "1.5.0"
debug_mode = false

[otherapp.logging]
level = "info"
enable_console = false
enable_file = true
file_path = "other_app.log"
)";
    }
};

// Basic configuration loading tests
TEST_F(ConfigTest, LoadConfigFromString) {
    auto& config = ConfigManager::instance();

    EXPECT_TRUE(config.load_from_string(sample_config_content(), "myapp"));
    EXPECT_TRUE(config.has_app_config("myapp"));
    EXPECT_FALSE(config.has_app_config("nonexistent"));
}

TEST_F(ConfigTest, LoadConfigFromFile) {
    // Create test config file
    std::ofstream file("test_config.toml");
    file << sample_config_content();
    file.close();

    auto& config = ConfigManager::instance();
    EXPECT_TRUE(config.load_config("test_config.toml", "myapp"));
    EXPECT_TRUE(config.has_app_config("myapp"));
}

TEST_F(ConfigTest, LoadNonExistentFile) {
    auto& config = ConfigManager::instance();
    EXPECT_FALSE(config.load_config("nonexistent.toml", "myapp"));
}

TEST_F(ConfigTest, LoadInvalidTomlContent) {
    auto& config = ConfigManager::instance();
    std::string invalid_toml = "[invalid toml content with missing quotes and brackets";
    EXPECT_FALSE(config.load_from_string(invalid_toml, "myapp"));
}

// Configuration parsing tests
TEST_F(ConfigTest, ParseAppConfig) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    auto app_config = config.get_app_config("myapp");

    EXPECT_EQ(app_config.name, "test_application");
    EXPECT_EQ(app_config.version, "2.0.0");
    EXPECT_EQ(app_config.description, "Test application for config system");
    EXPECT_TRUE(app_config.debug_mode);
    EXPECT_EQ(app_config.worker_threads, 8);
    EXPECT_EQ(app_config.working_directory, "/tmp/test");
}

TEST_F(ConfigTest, ParseLoggingConfig) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    auto logging_config = config.get_logging_config("myapp");

    EXPECT_EQ(logging_config.level, LogLevel::Debug);
    EXPECT_EQ(logging_config.pattern, "[%Y-%m-%d %H:%M:%S] [%l] %v");
    EXPECT_TRUE(logging_config.enable_console);
    EXPECT_TRUE(logging_config.enable_file);
    EXPECT_EQ(logging_config.file_path, "test_logs/app.log");
    EXPECT_EQ(logging_config.max_file_size, 5242880UL);
    EXPECT_EQ(logging_config.max_files, 3UL);
    EXPECT_TRUE(logging_config.flush_immediately);
}

TEST_F(ConfigTest, ParseNetworkConfig) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    auto network_config = config.get_network_config("myapp");

    EXPECT_EQ(network_config.host, "0.0.0.0");
    EXPECT_EQ(network_config.port, 9090);
    EXPECT_EQ(network_config.timeout_seconds, 60);
    EXPECT_EQ(network_config.max_connections, 200);
    EXPECT_TRUE(network_config.enable_ssl);
    EXPECT_EQ(network_config.ssl_cert_path, "/etc/ssl/cert.pem");
    EXPECT_EQ(network_config.ssl_key_path, "/etc/ssl/key.pem");
}

// Custom value retrieval tests
TEST_F(ConfigTest, GetCustomValues) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    // Test string values
    auto db_host = config.get_value<std::string>("database.host", "myapp");
    ASSERT_TRUE(db_host.has_value());
    EXPECT_EQ(*db_host, "db.example.com");

    // Test integer values
    auto db_port = config.get_value<int>("database.port", "myapp");
    ASSERT_TRUE(db_port.has_value());
    EXPECT_EQ(*db_port, 5432);

    // Test int64_t values
    auto cache_ttl = config.get_value<int64_t>("cache.ttl_seconds", "myapp");
    ASSERT_TRUE(cache_ttl.has_value());
    EXPECT_EQ(*cache_ttl, 7200);

    // Test non-existent value
    auto missing = config.get_value<std::string>("nonexistent.key", "myapp");
    EXPECT_FALSE(missing.has_value());
}

TEST_F(ConfigTest, GetValueWithDefault) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    // Existing value
    auto existing = config.get_value_or<std::string>("database.host", "default_host", "myapp");
    EXPECT_EQ(existing, "db.example.com");

    // Non-existent value with default
    auto with_default = config.get_value_or<std::string>("missing.key", "default_value", "myapp");
    EXPECT_EQ(with_default, "default_value");

    // Integer with default
    auto int_default = config.get_value_or<int>("missing.int", 42, "myapp");
    EXPECT_EQ(int_default, 42);
}

// Multi-app configuration tests
TEST_F(ConfigTest, MultipleAppConfigs) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");
    config.load_from_string(sample_config_content(), "otherapp");

    EXPECT_TRUE(config.has_app_config("myapp"));
    EXPECT_TRUE(config.has_app_config("otherapp"));

    auto app_names = config.get_app_names();
    EXPECT_EQ(app_names.size(), 2);
    EXPECT_TRUE(std::find(app_names.begin(), app_names.end(), "myapp") != app_names.end());
    EXPECT_TRUE(std::find(app_names.begin(), app_names.end(), "otherapp") != app_names.end());
}

TEST_F(ConfigTest, DifferentAppConfigs) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");
    config.load_from_string(sample_config_content(), "otherapp");

    auto myapp_config = config.get_app_config("myapp");
    auto otherapp_config = config.get_app_config("otherapp");

    EXPECT_EQ(myapp_config.name, "test_application");
    EXPECT_EQ(otherapp_config.name, "other_app");

    auto myapp_logging = config.get_logging_config("myapp");
    auto otherapp_logging = config.get_logging_config("otherapp");

    EXPECT_EQ(myapp_logging.level, LogLevel::Debug);
    EXPECT_EQ(otherapp_logging.level, LogLevel::Info);
    EXPECT_TRUE(myapp_logging.enable_console);
    EXPECT_FALSE(otherapp_logging.enable_console);
}

// Default configuration tests
TEST_F(ConfigTest, DefaultConfigurations) {
    auto& config = ConfigManager::instance();

    // Get configs for non-existent app should return defaults
    auto app_config = config.get_app_config("nonexistent");
    EXPECT_EQ(app_config.name, "crux_app");
    EXPECT_EQ(app_config.version, "1.0.0");
    EXPECT_FALSE(app_config.debug_mode);

    auto logging_config = config.get_logging_config("nonexistent");
    EXPECT_EQ(logging_config.level, LogLevel::Info);
    EXPECT_TRUE(logging_config.enable_console);
    EXPECT_FALSE(logging_config.enable_file);

    auto network_config = config.get_network_config("nonexistent");
    EXPECT_EQ(network_config.host, "localhost");
    EXPECT_EQ(network_config.port, 8080);
    EXPECT_FALSE(network_config.enable_ssl);
}

// Template creation test
TEST_F(ConfigTest, CreateConfigTemplate) {
    EXPECT_TRUE(ConfigManager::create_config_template("test_template.toml", "myapp"));
    EXPECT_TRUE(std::filesystem::exists("test_template.toml"));

    // Verify template is valid TOML and can be loaded
    auto& config = ConfigManager::instance();
    EXPECT_TRUE(config.load_config("test_template.toml", "myapp"));

    auto app_config = config.get_app_config("myapp");
    EXPECT_EQ(app_config.name, "myapp");
}

// Configuration reload test
TEST_F(ConfigTest, ReloadConfiguration) {
    // Create initial config
    std::ofstream file("test_config.toml");
    file << sample_config_content();
    file.close();

    auto& config = ConfigManager::instance();
    EXPECT_TRUE(config.load_config("test_config.toml", "myapp"));

    auto initial_config = config.get_app_config("myapp");
    EXPECT_EQ(initial_config.name, "test_application");

    // Modify the file
    std::ofstream modified_file("test_config.toml");
    modified_file << R"(
[myapp]
[myapp.app]
name = "modified_application"
version = "3.0.0"
)";
    modified_file.close();

    // Reload and verify changes
    EXPECT_TRUE(config.reload_config());
    auto reloaded_config = config.get_app_config("myapp");
    EXPECT_EQ(reloaded_config.name, "modified_application");
    EXPECT_EQ(reloaded_config.version, "3.0.0");
}

// Clear configuration test
TEST_F(ConfigTest, ClearConfiguration) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    EXPECT_TRUE(config.has_app_config("myapp"));

    config.clear();

    EXPECT_FALSE(config.has_app_config("myapp"));
    EXPECT_TRUE(config.get_app_names().empty());
}

// Thread safety test
TEST_F(ConfigTest, ThreadSafetyTest) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    // Launch multiple threads that read configuration
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&config, &success_count]() {
            for (int j = 0; j < 100; ++j) {
                auto app_config = config.get_app_config("myapp");
                auto logging_config = config.get_logging_config("myapp");
                auto network_config = config.get_network_config("myapp");

                if (app_config.name == "test_application" &&
                    logging_config.level == LogLevel::Debug &&
                    network_config.port == 9090) {
                    success_count++;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count.load(), 1000); // 10 threads * 100 iterations
}

// Edge cases and error handling
TEST_F(ConfigTest, EdgeCases) {
    auto& config = ConfigManager::instance();

    // Empty TOML content
    EXPECT_TRUE(config.load_from_string("", "empty"));
    auto empty_config = config.get_app_config("empty");
    EXPECT_EQ(empty_config.name, "crux_app"); // Should return defaults

    // TOML with only comments
    EXPECT_TRUE(config.load_from_string("# This is just a comment", "comment_only"));

    // Complex nested key retrieval
    std::string complex_toml = R"(
[testapp]
[testapp.level1]
[testapp.level1.level2]
[testapp.level1.level2.level3]
deep_value = "found it"
)";

    EXPECT_TRUE(config.load_from_string(complex_toml, "testapp"));
    auto deep_value = config.get_value<std::string>("level1.level2.level3.deep_value", "testapp");
    ASSERT_TRUE(deep_value.has_value());
    EXPECT_EQ(*deep_value, "found it");
}

// Logger integration test
TEST_F(ConfigTest, LoggerIntegration) {
    auto& config = ConfigManager::instance();
    config.load_from_string(sample_config_content(), "myapp");

    // This test verifies that configure_logger doesn't crash
    // We can't easily test the actual logger configuration without
    // mocking the logger system, but we can ensure the method works
    EXPECT_TRUE(config.configure_logger("myapp", "test_logger"));

    // Test with non-existent app (should use defaults)
    EXPECT_TRUE(config.configure_logger("nonexistent", "default_logger"));
}
