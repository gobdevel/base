/*
 * @file example_threaded_component.cpp
 * @brief Example usage of ThreadedComponent base class
 */

#include "application.h"
#include <iostream>
#include <queue>

using namespace base;

// Define message types for inter-thread communication
struct WorkRequest {
    std::string task_id;
    std::string payload;
    int priority = 0;
};

struct ConfigUpdate {
    std::string key;
    std::string value;
};

struct StatusRequest {
    std::string requester;
};

struct StatusResponse {
    std::string component_name;
    bool healthy;
    std::size_t processed_count;
    std::string status_message;
};

/**
 * @brief Example data processing service using ThreadedComponent
 */
class DataProcessor : public ThreadedComponent {
public:
    DataProcessor() : ThreadedComponent("DataProcessor") {}

protected:
    bool on_initialize() override {
        Logger::info("DataProcessor initializing...");

        // Subscribe to message types we want to handle
        subscribe_to_messages<WorkRequest>([this](const WorkRequest& request) {
            handle_work_request(request);
        });

        subscribe_to_messages<ConfigUpdate>([this](const ConfigUpdate& update) {
            handle_config_update(update);
        });

        subscribe_to_messages<StatusRequest>([this](const StatusRequest& request) {
            handle_status_request(request);
        });

        // Initialize component state
        config_["batch_size"] = "10";
        config_["timeout_ms"] = "5000";
        processed_count_ = 0;

        Logger::info("DataProcessor initialization complete");
        return true;
    }

    bool on_start() override {
        Logger::info("DataProcessor starting business logic...");

        // Schedule periodic health check every 5 seconds
        health_timer_id_ = schedule_timer(std::chrono::seconds(5), [this]() {
            perform_health_check();
        });

        // Schedule periodic statistics reporting every 10 seconds
        stats_timer_id_ = schedule_timer(std::chrono::seconds(10), [this]() {
            report_statistics();
        });

        // Schedule periodic batch processing every 2 seconds
        batch_timer_id_ = schedule_timer(std::chrono::seconds(2), [this]() {
            process_pending_work();
        });

        Logger::info("DataProcessor started successfully");
        return true;
    }

    void on_stop() override {
        Logger::info("DataProcessor stopping...");

        // Cancel timers (automatically handled by base class, but good practice)
        cancel_timer(health_timer_id_);
        cancel_timer(stats_timer_id_);
        cancel_timer(batch_timer_id_);

        // Process any remaining work
        process_pending_work();

        Logger::info("DataProcessor stopped. Total processed: {}", processed_count_.load());
    }

    bool on_health_check() override {
        // Custom health check logic
        bool healthy = work_queue_.size() < 100; // Not too backlogged

        if (!healthy) {
            Logger::warn("DataProcessor unhealthy: work queue size = {}", work_queue_.size());
        }

        return healthy;
    }

private:
    void handle_work_request(const WorkRequest& request) {
        Logger::debug("Received work request: {} (priority: {})", request.task_id, request.priority);

        // Add to work queue (in a real implementation, you'd use a priority queue)
        work_queue_.push(request);

        Logger::debug("Work queue size: {}", work_queue_.size());
    }

    void handle_config_update(const ConfigUpdate& update) {
        Logger::info("Configuration update: {} = {}", update.key, update.value);
        config_[update.key] = update.value;

        // Apply configuration changes
        if (update.key == "batch_size") {
            try {
                batch_size_ = std::stoi(update.value);
                Logger::info("Batch size updated to: {}", batch_size_);
            } catch (const std::exception& e) {
                Logger::error("Invalid batch_size value '{}': {}", update.value, e.what());
            }
        }
    }

    void handle_status_request(const StatusRequest& request) {
        Logger::debug("Status request from: {}", request.requester);

        StatusResponse response{
            .component_name = name(),
            .healthy = on_health_check(),
            .processed_count = processed_count_.load(),
            .status_message = "Queue size: " + std::to_string(work_queue_.size())
        };

        // In a real application, you'd send this back to the requester
        Logger::info("Status: healthy={}, processed={}, queue={}",
                    response.healthy, response.processed_count, work_queue_.size());
    }

    void process_pending_work() {
        if (work_queue_.empty()) {
            return;
        }

        Logger::debug("Processing batch of work items...");

        int processed_this_batch = 0;
        while (!work_queue_.empty() && processed_this_batch < batch_size_) {
            auto work_item = work_queue_.front();
            work_queue_.pop();

            // Simulate work processing
            process_work_item(work_item);
            processed_this_batch++;
            processed_count_++;
        }

        if (processed_this_batch > 0) {
            Logger::info("Processed {} work items, total: {}", processed_this_batch, processed_count_.load());
        }
    }

    void process_work_item(const WorkRequest& item) {
        // Simulate actual work
        Logger::trace("Processing work item: {}", item.task_id);

        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    void perform_health_check() {
        bool healthy = on_health_check();
        Logger::debug("Health check: {}", healthy ? "HEALTHY" : "UNHEALTHY");
    }

    void report_statistics() {
        Logger::info("DataProcessor Stats - Processed: {}, Queue: {}, Healthy: {}",
                    processed_count_.load(), work_queue_.size(), on_health_check());
    }

private:
    // Configuration
    std::unordered_map<std::string, std::string> config_;
    int batch_size_ = 10;

    // Work management
    std::queue<WorkRequest> work_queue_;
    std::atomic<std::size_t> processed_count_{0};

    // Timer IDs
    std::size_t health_timer_id_ = 0;
    std::size_t stats_timer_id_ = 0;
    std::size_t batch_timer_id_ = 0;
};

/**
 * @brief Example network service using ThreadedComponent
 */
class NetworkService : public ThreadedComponent {
public:
    NetworkService() : ThreadedComponent("NetworkService") {}

protected:
    bool on_initialize() override {
        Logger::info("NetworkService initializing...");

        // Subscribe to status requests
        subscribe_to_messages<StatusRequest>([this](const StatusRequest& request) {
            handle_status_request(request);
        });

        return true;
    }

    bool on_start() override {
        Logger::info("NetworkService starting...");

        // Schedule periodic network polling
        network_timer_id_ = schedule_timer(std::chrono::seconds(3), [this]() {
            simulate_network_activity();
        });

        return true;
    }

    void on_stop() override {
        Logger::info("NetworkService stopping...");
        cancel_timer(network_timer_id_);
    }

private:
    void handle_status_request(const StatusRequest& request) {
        Logger::info("NetworkService status requested by: {}", request.requester);
        // Handle status request
    }

    void simulate_network_activity() {
        static int counter = 0;
        counter++;
        Logger::debug("NetworkService simulated network activity #{}", counter);
    }

private:
    std::size_t network_timer_id_ = 0;
};

/**
 * @brief Example application using ThreadedComponents
 */
class ExampleApp : public Application {
public:
    ExampleApp() : Application(create_config()) {
        Logger::init();
    }

private:
    static ApplicationConfig create_config() {
        return ApplicationConfig{
            .name = "ThreadedComponentExample",
            .version = "1.0.0",
            .description = "Example application using ThreadedComponent base class",
            .worker_threads = 1
        };
    }

protected:
    bool on_initialize() override {
        Logger::info("ExampleApp initializing...");

        // Create threaded components
        data_processor_ = std::make_unique<DataProcessor>();
        network_service_ = std::make_unique<NetworkService>();

        // Initialize components
        if (!data_processor_->initialize(*this)) {
            Logger::error("Failed to initialize DataProcessor");
            return false;
        }

        if (!network_service_->initialize(*this)) {
            Logger::error("Failed to initialize NetworkService");
            return false;
        }

        return true;
    }

    bool on_start() override {
        Logger::info("ExampleApp starting threaded components...");

        // Start threaded components
        if (!data_processor_->start()) {
            Logger::error("Failed to start DataProcessor");
            return false;
        }

        if (!network_service_->start()) {
            Logger::error("Failed to start NetworkService");
            return false;
        }

        // Create a coordinator thread to send messages
        coordinator_ = create_thread("Coordinator", [this](ManagedThreadBase& thread_base) {
            // Cast to Application::ManagedThread to access specific methods
            auto& thread = static_cast<Application::ManagedThread&>(thread_base);
            run_coordinator(thread);
        });

        return true;
    }

    bool on_stop() override {
        Logger::info("ExampleApp stopping threaded components...");

        // Stop threaded components
        data_processor_->stop();
        network_service_->stop();

        return true;
    }

private:
    void run_coordinator(ManagedThread& thread) {
        Logger::info("Coordinator starting...");

        // Send some initial work requests
        for (int i = 0; i < 5; ++i) {
            WorkRequest work{
                .task_id = "initial-" + std::to_string(i),
                .payload = "Initial work payload " + std::to_string(i),
                .priority = i % 3
            };

            data_processor_->send_message(work);
        }

        // Schedule periodic work generation
        auto timer = std::make_shared<asio::steady_timer>(thread.io_context());

        std::function<void()> generate_work = [this, timer, &thread, &generate_work]() {
            if (thread.stop_requested()) return;

            timer->expires_after(std::chrono::seconds(7));
            timer->async_wait([this, &generate_work](const asio::error_code& ec) {
                if (!ec) {
                    static int work_counter = 100;

                    // Generate new work
                    WorkRequest work{
                        .task_id = "generated-" + std::to_string(work_counter++),
                        .payload = "Generated at runtime",
                        .priority = 1
                    };

                    data_processor_->send_message(work);

                    // Send config update occasionally
                    if (work_counter % 3 == 0) {
                        ConfigUpdate config{
                            .key = "batch_size",
                            .value = std::to_string(5 + (work_counter % 10))
                        };
                        data_processor_->send_message(config);
                    }

                    // Request status occasionally
                    if (work_counter % 5 == 0) {
                        StatusRequest status_req{.requester = "Coordinator"};
                        data_processor_->send_message(status_req);
                        network_service_->send_message(status_req);
                    }

                    generate_work();
                }
            });
        };

        // Start work generation
        generate_work();
    }

private:
    std::unique_ptr<DataProcessor> data_processor_;
    std::unique_ptr<NetworkService> network_service_;
    std::shared_ptr<ManagedThreadBase> coordinator_;
};

// Main function
BASE_APPLICATION_MAIN(ExampleApp)
