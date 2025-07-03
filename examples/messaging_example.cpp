/*
 * @file messaging_example.cpp
 * @brief Example demonstrating the messaging system
 *
 * This example shows how to use the type-safe messaging system for
 * inter-thread communication in a microservices-style architecture.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "application.h"
#include "messaging.h"
#include <thread>
#include <random>
#include <iostream>

using namespace base;

// Define message types for our example microservice
struct OrderMessage {
    int order_id;
    std::string product;
    int quantity;
    double price;

    OrderMessage(int id, std::string prod, int qty, double p)
        : order_id(id), product(std::move(prod)), quantity(qty), price(p) {}
};

struct PaymentMessage {
    int order_id;
    double amount;
    std::string payment_method;
    bool success;

    PaymentMessage(int id, double amt, std::string method, bool succ)
        : order_id(id), amount(amt), payment_method(std::move(method)), success(succ) {}
};

struct InventoryMessage {
    int order_id;
    std::string product;
    int quantity;
    bool available;

    InventoryMessage(int id, std::string prod, int qty, bool avail)
        : order_id(id), product(std::move(prod)), quantity(qty), available(avail) {}
};

struct NotificationMessage {
    int order_id;
    std::string message;
    std::string notification_type;

    NotificationMessage(int id, std::string msg, std::string type)
        : order_id(id), message(std::move(msg)), notification_type(std::move(type)) {}
};

// Global statistics
std::atomic<int> notification_count{0};

void setup_message_handlers(Application& app) {
    // Order processor handles incoming orders
    auto* order_processor = app.get_managed_thread("order-processor");
    if (order_processor) {
        order_processor->subscribe_to_messages<OrderMessage>([&app](const Message<OrderMessage>& msg) {
            const auto& order = msg.data();
            Logger::info("Processing order #{}: {} x {} @ ${:.2f}",
                        order.order_id, order.quantity, order.product, order.price);

            // Check inventory first
            app.send_message_to_thread("inventory-service",
                InventoryMessage{order.order_id, order.product, order.quantity, false});

            // Process payment
            app.send_message_to_thread("payment-service",
                PaymentMessage{order.order_id, order.price * order.quantity, "credit_card", false});
        });
    }

    // Payment service handles payment requests
    auto* payment_service = app.get_managed_thread("payment-service");
    if (payment_service) {
        payment_service->subscribe_to_messages<PaymentMessage>([&app](const Message<PaymentMessage>& msg) {
            const auto& payment = msg.data();

            // Simulate payment processing
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);

            bool success = dis(gen) > 0.1; // 90% success rate

            if (success) {
                Logger::info("Payment successful for order #{}: ${:.2f}",
                            payment.order_id, payment.amount);

                app.send_message_to_thread("notification-service",
                    NotificationMessage{payment.order_id,
                                      "Payment processed successfully", "payment_success"});
            } else {
                Logger::warn("Payment failed for order #{}: ${:.2f}",
                            payment.order_id, payment.amount);

                app.send_message_to_thread("notification-service",
                    NotificationMessage{payment.order_id,
                                      "Payment processing failed", "payment_failure"});
            }
        });
    }

    // Inventory service handles inventory checks
    auto* inventory_service = app.get_managed_thread("inventory-service");
    if (inventory_service) {
        inventory_service->subscribe_to_messages<InventoryMessage>([&app](const Message<InventoryMessage>& msg) {
            const auto& inventory = msg.data();

            // Simulate inventory check
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);

            bool available = dis(gen) > 0.05; // 95% availability

            if (available) {
                Logger::info("Inventory available for order #{}: {} x {}",
                            inventory.order_id, inventory.quantity, inventory.product);

                app.send_message_to_thread("notification-service",
                    NotificationMessage{inventory.order_id,
                                      "Items reserved from inventory", "inventory_reserved"});
            } else {
                Logger::warn("Insufficient inventory for order #{}: {} x {}",
                            inventory.order_id, inventory.quantity, inventory.product);

                app.send_message_to_thread("notification-service",
                    NotificationMessage{inventory.order_id,
                                      "Insufficient inventory", "inventory_shortage"});
            }
        });
    }

    // Notification service handles all notifications
    auto* notification_service = app.get_managed_thread("notification-service");
    if (notification_service) {
        notification_service->subscribe_to_messages<NotificationMessage>([](const Message<NotificationMessage>& msg) {
            const auto& notification = msg.data();
            Logger::info("📧 Notification for order #{} [{}]: {}",
                        notification.order_id, notification.notification_type, notification.message);

            // Simulate sending email/SMS/push notification
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            notification_count++;
        });
    }
}

int main() {
    try {
        Logger::init();

        Logger::info("=== Starting Messaging System Demo ===");
        Logger::info("This demo will:");
        Logger::info("1. Create 4 specialized service threads");
        Logger::info("2. Generate 5 sample orders");
        Logger::info("3. Show inter-thread messaging");
        Logger::info("4. Automatically shutdown after completion");
        Logger::info("=====================================");

        ApplicationConfig config;
        config.name = "MessagingExample";
        config.version = "1.0.0";
        config.description = "Messaging system demonstration";
        config.worker_threads = 2;
        config.enable_health_check = false; // Disable for simplicity

        Application app(config);

        // Create specialized threads for different services
        auto order_processor = app.create_thread("order-processor", [](asio::io_context& io_ctx) {
            Logger::info("Order processor thread started");
        });

        auto payment_service = app.create_thread("payment-service", [](asio::io_context& io_ctx) {
            Logger::info("Payment service thread started");
        });

        auto inventory_service = app.create_thread("inventory-service", [](asio::io_context& io_ctx) {
            Logger::info("Inventory service thread started");
        });

        auto notification_service = app.create_worker_thread("notification-service");

        Logger::info("Created {} specialized service threads", app.managed_thread_count());

        // Set up message handlers
        setup_message_handlers(app);
        Logger::info("Message handlers configured for all services");

        // Generate sample orders
        std::vector<std::string> products = {"Laptop", "Phone", "Tablet", "Headphones", "Speaker"};

        for (int i = 1; i <= 5; ++i) {
            std::string product = products[(i - 1) % products.size()];
            int quantity = (i % 3) + 1;
            double price = 100.0 * i + 50.0;

            Logger::info("Generating demo order #{}/5", i);
            app.send_message_to_thread("order-processor",
                OrderMessage{i, product, quantity, price}, MessagePriority::High);

            // Small delay between orders
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        // Give time for all messages to be processed
        Logger::info("Waiting for message processing to complete...");
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Print final statistics
        Logger::info("=== Final Statistics ===");
        Logger::info("Active threads: {}", app.managed_thread_count());
        Logger::info("Notifications sent: {}", notification_count.load());

        // Check pending messages in each thread
        if (auto* thread = app.get_managed_thread("order-processor")) {
            Logger::info("Order processor pending: {}", thread->pending_message_count());
        }
        if (auto* thread = app.get_managed_thread("payment-service")) {
            Logger::info("Payment service pending: {}", thread->pending_message_count());
        }
        if (auto* thread = app.get_managed_thread("inventory-service")) {
            Logger::info("Inventory service pending: {}", thread->pending_message_count());
        }
        if (auto* thread = app.get_managed_thread("notification-service")) {
            Logger::info("Notification service pending: {}", thread->pending_message_count());
        }

        Logger::info("======================");
        Logger::info("Demo completed successfully!");

        // Clean shutdown
        app.stop_all_managed_threads();
        app.join_all_managed_threads();

        Logger::shutdown();
        return 0;

    } catch (const std::exception& e) {
        Logger::error("Example failed: {}", e.what());
        Logger::shutdown();
        return 1;
    }
}
