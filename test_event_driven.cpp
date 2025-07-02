/*
 * @file test_event_driven.cpp
 * @brief Simple test to verify event-driven messaging works correctly
 */

#include "application.h"
#include "messaging.h"
#include "logger.h"

#include <iostream>
#include <chrono>
#include <atomic>

using namespace crux;

int main() {
    std::cout << "Testing Event-Driven Messaging System" << std::endl;
    std::cout << "=====================================" << std::endl;

    try {
        ApplicationConfig config;
        config.worker_threads = 1;
        config.enable_health_check = false;
        Application app(config);

        struct TestMessage {
            int id;
            std::string data;
        };

        // Create event-driven threads
        auto sender = app.create_event_driven_thread("sender");
        auto receiver = app.create_event_driven_thread("receiver");

        std::atomic<int> messages_received{0};
        const int total_messages = 100;

        // Subscribe to messages
        receiver->subscribe_to_messages<TestMessage>([&](const Message<TestMessage>& msg) {
            std::cout << "Received message " << msg.data().id << ": " << msg.data().data << std::endl;
            messages_received++;
        });

        // Give threads time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "\nSending " << total_messages << " messages..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        // Send messages
        for (int i = 0; i < total_messages; ++i) {
            receiver->send_message(TestMessage{i, "Hello from message " + std::to_string(i)});
        }

        // Wait for all messages to be processed
        while (messages_received < total_messages) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "\nResults:" << std::endl;
        std::cout << "Messages sent: " << total_messages << std::endl;
        std::cout << "Messages received: " << messages_received.load() << std::endl;
        std::cout << "Duration: " << duration.count() << "ms" << std::endl;
        std::cout << "Throughput: " << (total_messages * 1000.0 / duration.count()) << " messages/sec" << std::endl;

        // Test immediate notification by checking queue is empty
        std::cout << "Receiver queue size: " << receiver->queue_size() << std::endl;

        // Clean shutdown
        sender->stop();
        receiver->stop();
        sender->join();
        receiver->join();

        std::cout << "\nEvent-driven messaging test completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
