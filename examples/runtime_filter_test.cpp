/*
 * @file runtime_filter_test.cpp
 * @brief Quick test to verify runtime component filtering
 */

#include <application.h>
#include <logger.h>
#include <config.h>
#include <chrono>
#include <thread>
#include <fmt/ranges.h>

using namespace base;

/**
 * @brief Simple test application for runtime filter verification.
 */
class RuntimeFilterTest : public Application {
public:
    RuntimeFilterTest() : Application(create_config()) {}

protected:
    bool on_initialize() override {
        // Load configuration
        auto& config_mgr = ConfigManager::instance();
        if (!config_mgr.load_config("examples/component_demo_selective.toml", "ComponentLoggingDemo")) {
            Logger::warn("Failed to load selective config");
            return false;
        }

        // Configure logger from config
        auto logging_config = config_mgr.get_logging_config("ComponentLoggingDemo");
        Logger::init(LoggerConfig{
            .app_name = "FilterTest",
            .level = logging_config.level,
            .enable_console = logging_config.enable_console,
            .enable_file = false,
            .enable_colors = true,
            .pattern = logging_config.pattern,
            .enable_component_logging = logging_config.enable_component_logging,
            .enabled_components = logging_config.enabled_components,
            .disabled_components = logging_config.disabled_components,
            .component_pattern = logging_config.component_pattern
        });

        Logger::info("Runtime filter test starting...");
        show_current_filters();

        return true;
    }

    bool on_start() override {
        Logger::info("Testing component filtering - only 'auth' and 'cache' should appear:");

        // Test various components - only auth and cache should show
        Logger::info(Logger::Component("database"), "This DATABASE message should NOT appear");
        Logger::info(Logger::Component("network"), "This NETWORK message should NOT appear");
        Logger::info(Logger::Component("auth"), "This AUTH message SHOULD appear");
        Logger::info(Logger::Component("cache"), "This CACHE message SHOULD appear");
        Logger::info(Logger::Component("storage"), "This STORAGE message should NOT appear");

        // Test with automatic component loggers
        auto database = Logger::get_component_logger("database");
        auto auth = Logger::get_component_logger("auth");
        auto cache = Logger::get_component_logger("cache");

        database.info("Automatic database logger - should NOT appear");
        auth.info("Automatic auth logger - SHOULD appear");
        cache.info("Automatic cache logger - SHOULD appear");

        // Schedule shutdown
        schedule_recurring_task([this]() {
            Logger::info("Filter test complete - shutting down");
            shutdown();
        }, std::chrono::seconds(2));

        return true;
    }

private:
    static ApplicationConfig create_config() {
        return ApplicationConfig{
            .name = "RuntimeFilterTest",
            .version = "1.0.0",
            .description = "Tests runtime component filtering"
        };
    }

    void show_current_filters() {
        auto enabled = Logger::get_enabled_components();
        auto disabled = Logger::get_disabled_components();

        if (enabled.empty()) {
            Logger::info("Enabled components: ALL");
        } else {
            Logger::info("Enabled components: [{}]", fmt::join(enabled.begin(), enabled.end(), ", "));
        }

        if (disabled.empty()) {
            Logger::info("Disabled components: NONE");
        } else {
            Logger::info("Disabled components: [{}]", fmt::join(disabled.begin(), disabled.end(), ", "));
        }
    }
};

BASE_APPLICATION_MAIN(RuntimeFilterTest)
