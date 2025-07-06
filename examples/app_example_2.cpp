#include <application.h>
#include <cli.h>
#include <logger.h>

using namespace base;

static ApplicationConfig create_config() {
    ApplicationConfig config;
    config.name = "AppExample";
    config.version = "1.0.0";
    config.description = "Example application demonstrating the Base framework";
    config.worker_threads = 1;
    config.enable_health_check = true;
    config.health_check_interval = std::chrono::milliseconds(5000);
    config.config_file = "app_example.toml";
    config.config_app_name = "app_example";
    config.enable_cli = true;           // Enable CLI automatically
    config.cli_enable_stdin = true;     // Enable stdin interface
    config.cli_enable_tcp = true;       // Enable TCP interface
    config.cli_bind_address = "127.0.0.1";
    config.cli_port = 8080;
    return config;
}

// Example CLI component for the application
// class AppCliComponent : public ApplicationComponent {
// public:
//     bool initialize(Application& app) override {
//         Logger::info(__PRETTY_FUNCTION__ << " initialization");
//         // Register CLI commands and options here
//         return true;
//     }
// };

class AppExample : public Application {
public:
    AppExample() : Application(create_config()) {
        Logger::init();
        // add_component(std::make_unique<AppCliComponent>());
    }
protected:
    bool on_initialize() override {
        Logger::info(__PRETTY_FUNCTION__, " initialization");
        // Register custom CLI commands
        register_custom_commands();
        return true;
    }

    bool on_start() override {
        Logger::info(__PRETTY_FUNCTION__, " startup");
        return true;
    }

    bool on_stop() override {
        Logger::info(__PRETTY_FUNCTION__, " shutdown");
        return true;
    }

    void on_cleanup() override {
        Logger::info(__PRETTY_FUNCTION__, " cleanup");
    }

    void on_signal(int signal) override {
        Logger::info(__PRETTY_FUNCTION__, " signal handler for signal {}", signal);
    }


private:
    void register_custom_commands() {
        auto& cli = this->cli();

        // Custom command to show task counter
        cli.register_command("show-table", "Show Table", "show-table",
            [this](const CLIContext& context) -> CLIResult {
                std::ostringstream oss;
                oss << "Table Dump: ";
                return CLIResult::ok(oss.str());
            });
    }
};

// Use the convenience macro to create main function
BASE_APPLICATION_MAIN(AppExample)