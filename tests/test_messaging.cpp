/*
 * @file test_messaging.cpp
 * @brief Unit tests for the messaging system
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include "thread_messaging.h"
#include "logger.h"
#include <thread>
#include <chrono>
#include <asio.hpp>

using namespace base;

// Test message types
struct SimpleMessage {
    int value;
    std::string text;

    SimpleMessage(int v, std::string t) : value(v), text(std::move(t)) {}
};

struct ComplexMessage {
    std::vector<int> data;
    std::chrono::steady_clock::time_point timestamp;

    ComplexMessage(std::vector<int> d)
        : data(std::move(d)), timestamp(std::chrono::steady_clock::now()) {}
};

class MessagingTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::init();
    }

    void TearDown() override {
        Logger::shutdown();
    }
};

TEST_F(MessagingTest, ThreadMessagingContextBasic) {
    asio::io_context io_context;
    auto context = std::make_shared<ThreadMessagingContext>("test_thread", io_context);

    std::atomic<int> message_count{0};
    std::atomic<int> last_value{0};

    // Subscribe to messages
    context->subscribe<SimpleMessage>([&message_count, &last_value](const SimpleMessage& msg) {
        message_count++;
        last_value.store(msg.value);
    });

    // Start the context
    context->start();

    // Send messages
    context->send_message(SimpleMessage{42, "test1"});
    context->send_message(SimpleMessage{84, "test2"});

    // Run IO context to process messages
    io_context.run();

    // Check results
    EXPECT_EQ(message_count.load(), 2);
    EXPECT_EQ(last_value.load(), 84);

    // Stop context
    context->stop();
}

TEST_F(MessagingTest, MessagingBusThreadCommunication) {
    asio::io_context io_context1, io_context2;
    auto context1 = std::make_shared<ThreadMessagingContext>("thread1", io_context1);
    auto context2 = std::make_shared<ThreadMessagingContext>("thread2", io_context2);

    std::atomic<int> thread1_messages{0};
    std::atomic<int> thread2_messages{0};
    std::atomic<int> broadcast_count{0};

    // Set up message handlers
    context1->subscribe<SimpleMessage>([&thread1_messages, &broadcast_count](const SimpleMessage& msg) {
        thread1_messages++;
        if (msg.text == "broadcast") {
            broadcast_count++;
        }
    });

    context2->subscribe<SimpleMessage>([&thread2_messages, &broadcast_count](const SimpleMessage& msg) {
        thread2_messages++;
        if (msg.text == "broadcast") {
            broadcast_count++;
        }
    });

    // Start contexts
    context1->start();
    context2->start();

    // Test direct thread messaging
    InterThreadMessagingBus::instance().send_to_thread("thread1", SimpleMessage{1, "direct"});
    InterThreadMessagingBus::instance().send_to_thread("thread2", SimpleMessage{2, "direct"});

    // Test broadcast
    InterThreadMessagingBus::instance().broadcast(SimpleMessage{3, "broadcast"});

    // Run IO contexts to process messages
    std::jthread t1([&io_context1]() { io_context1.run(); });
    std::jthread t2([&io_context2]() { io_context2.run(); });

    // Allow messages to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop contexts and threads
    context1->stop();
    context2->stop();
    io_context1.stop();
    io_context2.stop();

    if (t1.joinable()) t1.join();
    if (t2.joinable()) t2.join();

    // Check results
    EXPECT_EQ(thread1_messages.load(), 2); // 1 direct + 1 broadcast
    EXPECT_EQ(thread2_messages.load(), 2); // 1 direct + 1 broadcast
    EXPECT_EQ(broadcast_count.load(), 2);  // Both threads received broadcast
}

TEST_F(MessagingTest, MessagePriority) {
    asio::io_context io_context;
    auto context = std::make_shared<ThreadMessagingContext>("test_thread", io_context);

    std::vector<int> received_order;
    std::mutex order_mutex;

    // Subscribe to messages
    context->subscribe<SimpleMessage>([&received_order, &order_mutex](const SimpleMessage& msg) {
        std::lock_guard<std::mutex> lock(order_mutex);
        received_order.push_back(msg.value);
    });

    // Start the context
    context->start();

    // Send messages with different priorities
    context->send_message(SimpleMessage{1, "low"}, MessagePriority::Low);
    context->send_message(SimpleMessage{2, "normal"}, MessagePriority::Normal);
    context->send_message(SimpleMessage{3, "high"}, MessagePriority::High);
    context->send_message(SimpleMessage{4, "critical"}, MessagePriority::Critical);

    // Run IO context to process messages
    io_context.run();

    // Check that all messages were received (order may vary due to ASIO scheduling)
    EXPECT_EQ(received_order.size(), 4);
    EXPECT_TRUE(std::find(received_order.begin(), received_order.end(), 1) != received_order.end());
    EXPECT_TRUE(std::find(received_order.begin(), received_order.end(), 2) != received_order.end());
    EXPECT_TRUE(std::find(received_order.begin(), received_order.end(), 3) != received_order.end());
    EXPECT_TRUE(std::find(received_order.begin(), received_order.end(), 4) != received_order.end());

    // Stop context
    context->stop();
}

TEST_F(MessagingTest, MultipleMessageTypes) {
    asio::io_context io_context;
    auto context = std::make_shared<ThreadMessagingContext>("test_thread", io_context);

    std::atomic<int> simple_count{0};
    std::atomic<int> complex_count{0};

    // Subscribe to different message types
    context->subscribe<SimpleMessage>([&simple_count](const SimpleMessage& msg) {
        simple_count++;
    });

    context->subscribe<ComplexMessage>([&complex_count](const ComplexMessage& msg) {
        complex_count++;
    });

    // Start the context
    context->start();

    // Send different message types
    context->send_message(SimpleMessage{1, "test"});
    context->send_message(ComplexMessage{std::vector<int>{1, 2, 3}});
    context->send_message(SimpleMessage{2, "test2"});

    // Run IO context to process messages
    io_context.run();

    // Check results
    EXPECT_EQ(simple_count.load(), 2);
    EXPECT_EQ(complex_count.load(), 1);

    // Stop context
    context->stop();
}

TEST_F(MessagingTest, UnsubscribeFromMessages) {
    asio::io_context io_context;
    auto context = std::make_shared<ThreadMessagingContext>("test_thread", io_context);

    std::atomic<int> message_count{0};

    // Subscribe to messages
    context->subscribe<SimpleMessage>([&message_count](const SimpleMessage& msg) {
        message_count++;
    });

    // Start the context
    context->start();

    // Send a message
    context->send_message(SimpleMessage{1, "test"});

    // Run IO context to process first message
    io_context.restart();
    io_context.run();

    EXPECT_EQ(message_count.load(), 1);

    // Unsubscribe from messages
    context->unsubscribe<SimpleMessage>();

    // Send another message
    context->send_message(SimpleMessage{2, "test2"});

    // Run IO context to process second message
    io_context.restart();
    io_context.run();

    // Count should still be 1 (message was ignored)
    EXPECT_EQ(message_count.load(), 1);

    // Stop context
    context->stop();
}

TEST_F(MessagingTest, MessagingBusPerformance) {
    constexpr int NUM_MESSAGES = 1000;

    asio::io_context io_context;
    auto context = std::make_shared<ThreadMessagingContext>("perf_thread", io_context);

    std::atomic<int> messages_processed{0};
    auto start_time = std::chrono::steady_clock::now();

    // Subscribe to messages
    context->subscribe<SimpleMessage>([&messages_processed](const SimpleMessage& msg) {
        messages_processed++;
    });

    // Start the context
    context->start();

    // Send many messages
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        context->send_message(SimpleMessage{i, "perf_test"});
    }

    // Run IO context to process messages
    io_context.run();

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Check results
    EXPECT_EQ(messages_processed.load(), NUM_MESSAGES);

    // Performance should be reasonable (less than 1 second for 1000 messages)
    EXPECT_LT(duration.count(), 1000);

    Logger::info("Processed {} messages in {}ms", NUM_MESSAGES, duration.count());

    // Stop context
    context->stop();
}
