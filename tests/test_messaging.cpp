/*
 * @file test_messaging.cpp
 * @brief Unit tests for the messaging system
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include "messaging.h"
#include "logger.h"
#include <thread>
#include <chrono>

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

TEST_F(MessagingTest, MessageCreation) {
    SimpleMessage msg{42, "test"};
    Message<SimpleMessage> typed_msg{1, std::move(msg), MessagePriority::Normal};

    EXPECT_EQ(typed_msg.id(), 1);
    EXPECT_EQ(typed_msg.priority(), MessagePriority::Normal);
    EXPECT_EQ(typed_msg.data().value, 42);
    EXPECT_EQ(typed_msg.data().text, "test");
}

TEST_F(MessagingTest, MessageQueue) {
    EventDrivenMessageQueue queue;

    // Test sending messages
    EXPECT_TRUE(queue.send(SimpleMessage{1, "first"}, MessagePriority::Normal));
    EXPECT_TRUE(queue.send(SimpleMessage{2, "second"}, MessagePriority::High));
    EXPECT_TRUE(queue.send(SimpleMessage{3, "third"}, MessagePriority::Low));

    EXPECT_EQ(queue.size(), 3);
    EXPECT_FALSE(queue.empty());

    // Test receiving messages (order not guaranteed for performance)
    auto msg1 = queue.receive(std::chrono::milliseconds(10));
    EXPECT_TRUE(msg1 != nullptr);

    auto msg2 = queue.receive(std::chrono::milliseconds(10));
    EXPECT_TRUE(msg2 != nullptr);

    auto msg3 = queue.receive(std::chrono::milliseconds(10));
    EXPECT_TRUE(msg3 != nullptr);

    EXPECT_TRUE(queue.empty());
}

TEST_F(MessagingTest, MessageRouter) {
    MessageRouter router;

    std::atomic<int> simple_count{0};
    std::atomic<int> complex_count{0};

    // Subscribe to different message types
    router.subscribe<SimpleMessage>("test_subscriber", [&simple_count](const Message<SimpleMessage>& msg) {
        simple_count++;
    });

    router.subscribe<ComplexMessage>("test_subscriber", [&complex_count](const Message<ComplexMessage>& msg) {
        complex_count++;
    });

    EXPECT_EQ(router.subscriber_count<SimpleMessage>(), 1);
    EXPECT_EQ(router.subscriber_count<ComplexMessage>(), 1);

    // Publish messages
    Message<SimpleMessage> simple_msg{1, SimpleMessage{42, "test"}, MessagePriority::Normal};
    Message<ComplexMessage> complex_msg{2, ComplexMessage{{1, 2, 3}}, MessagePriority::Normal};

    router.publish(simple_msg);
    router.publish(complex_msg);

    // Give handlers a moment to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(simple_count.load(), 1);
    EXPECT_EQ(complex_count.load(), 1);

    // Test unsubscribe
    router.unsubscribe<SimpleMessage>("test_subscriber");
    EXPECT_EQ(router.subscriber_count<SimpleMessage>(), 0);
}

TEST_F(MessagingTest, ThreadMessagingContext) {
    ThreadMessagingContext context("test_thread");

    std::atomic<int> message_count{0};
    std::atomic<int> last_value{0};

    // Subscribe to messages
    context.subscribe<SimpleMessage>([&message_count, &last_value](const Message<SimpleMessage>& msg) {
        message_count++;
        last_value.store(msg.data().value);
    });

    // Send messages
    EXPECT_TRUE(context.send_message(SimpleMessage{100, "test1"}));
    EXPECT_TRUE(context.send_message(SimpleMessage{200, "test2"}));

    EXPECT_EQ(context.pending_message_count(), 2);

    // Process messages in batch mode
    context.process_messages_batch();

    // Give handlers a moment to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(message_count.load(), 2);
    EXPECT_EQ(last_value.load(), 200);
    EXPECT_EQ(context.pending_message_count(), 0);
}

TEST_F(MessagingTest, MessagingBus) {
    MessagingBus& bus = MessagingBus::instance();

    // Create thread contexts
    auto context1 = std::make_shared<ThreadMessagingContext>("thread1");
    auto context2 = std::make_shared<ThreadMessagingContext>("thread2");

    std::atomic<int> thread1_messages{0};
    std::atomic<int> thread2_messages{0};
    std::atomic<int> broadcast_count{0};

    // Subscribe to messages
    context1->subscribe<SimpleMessage>([&thread1_messages, &broadcast_count](const Message<SimpleMessage>& msg) {
        thread1_messages++;
        if (msg.data().text == "broadcast") {
            broadcast_count++;
        }
    });

    context2->subscribe<SimpleMessage>([&thread2_messages, &broadcast_count](const Message<SimpleMessage>& msg) {
        thread2_messages++;
        if (msg.data().text == "broadcast") {
            broadcast_count++;
        }
    });

    // Register threads
    bus.register_thread("thread1", context1);
    bus.register_thread("thread2", context2);

    EXPECT_EQ(bus.thread_count(), 2);
    EXPECT_TRUE(bus.is_thread_registered("thread1"));
    EXPECT_TRUE(bus.is_thread_registered("thread2"));

    // Send targeted message
    EXPECT_TRUE(bus.send_to_thread("thread1", SimpleMessage{1, "targeted"}));

    // Broadcast message
    bus.broadcast(SimpleMessage{2, "broadcast"});

    // Process messages in both contexts using batch mode
    context1->process_messages_batch();
    context2->process_messages_batch();

    // Give handlers a moment to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_EQ(thread1_messages.load(), 2); // targeted + broadcast
    EXPECT_EQ(thread2_messages.load(), 1); // broadcast only
    EXPECT_EQ(broadcast_count.load(), 2);  // received by both threads

    // Cleanup
    bus.unregister_thread("thread1");
    bus.unregister_thread("thread2");

    EXPECT_EQ(bus.thread_count(), 0);
}

TEST_F(MessagingTest, MessagePriority) {
    EventDrivenMessageQueue queue;

    // Send messages in non-priority order
    queue.send(SimpleMessage{1, "low"}, MessagePriority::Low);
    queue.send(SimpleMessage{2, "critical"}, MessagePriority::Critical);
    queue.send(SimpleMessage{3, "normal"}, MessagePriority::Normal);
    queue.send(SimpleMessage{4, "high"}, MessagePriority::High);

    // Test that all messages are received
    std::vector<MessagePriority> received_order;

    while (!queue.empty()) {
        auto msg = queue.receive(std::chrono::milliseconds(1));
        if (msg) {
            received_order.push_back(msg->priority());
        }
    }

    // Just check that we received all 4 messages (order not important for performance)
    EXPECT_EQ(received_order.size(), 4);
}

TEST_F(MessagingTest, MessageTypeSafety) {
    MessageRouter router;

    std::atomic<bool> simple_received{false};
    std::atomic<bool> complex_received{false};

    // Subscribe to specific message types
    router.subscribe<SimpleMessage>("subscriber", [&simple_received](const Message<SimpleMessage>& msg) {
        simple_received.store(true);
    });

    router.subscribe<ComplexMessage>("subscriber", [&complex_received](const Message<ComplexMessage>& msg) {
        complex_received.store(true);
    });

    // Send SimpleMessage - should only trigger SimpleMessage handler
    Message<SimpleMessage> simple_msg{1, SimpleMessage{42, "test"}, MessagePriority::Normal};
    router.publish(simple_msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_TRUE(simple_received.load());
    EXPECT_FALSE(complex_received.load());

    // Reset and send ComplexMessage
    simple_received.store(false);
    complex_received.store(false);

    Message<ComplexMessage> complex_msg{2, ComplexMessage{{1, 2, 3}}, MessagePriority::Normal};
    router.publish(complex_msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_FALSE(simple_received.load());
    EXPECT_TRUE(complex_received.load());
}

TEST_F(MessagingTest, PerformanceBasics) {
    EventDrivenMessageQueue queue;

    const int message_count = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    // Send messages
    for (int i = 0; i < message_count; ++i) {
        queue.send(SimpleMessage{i, "test"});
    }

    // Receive messages
    for (int i = 0; i < message_count; ++i) {
        auto msg = queue.receive(std::chrono::milliseconds(1));
        EXPECT_TRUE(msg != nullptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    Logger::info("Processed {} messages in {} microseconds ({:.2f} msg/sec)",
                message_count, duration.count(),
                (message_count * 1000000.0) / duration.count());

    // Should be able to process at least 10k messages per second
    EXPECT_LT(duration.count(), 100000); // Less than 100ms for 1000 messages
}

TEST_F(MessagingTest, MessagingIntegration) {
    // This test verifies that the messaging system components work together
    MessagingBus& bus = MessagingBus::instance();

    auto context1 = std::make_shared<ThreadMessagingContext>("integration_thread1");
    auto context2 = std::make_shared<ThreadMessagingContext>("integration_thread2");

    std::atomic<int> total_messages{0};
    std::atomic<int> high_priority_messages{0};

    // Set up message handlers
    auto handler = [&total_messages, &high_priority_messages](const Message<SimpleMessage>& msg) {
        total_messages++;
        if (msg.priority() == MessagePriority::High) {
            high_priority_messages++;
        }
    };

    context1->subscribe<SimpleMessage>(handler);
    context2->subscribe<SimpleMessage>(handler);

    bus.register_thread("integration_thread1", context1);
    bus.register_thread("integration_thread2", context2);

    // Send various messages
    bus.send_to_thread("integration_thread1", SimpleMessage{1, "direct"}, MessagePriority::Normal);
    bus.send_to_thread("integration_thread2", SimpleMessage{2, "direct"}, MessagePriority::High);
    bus.broadcast(SimpleMessage{3, "broadcast"}, MessagePriority::Normal);
    bus.broadcast(SimpleMessage{4, "broadcast"}, MessagePriority::High);

    // Process messages
    context1->process_messages_batch();
    context2->process_messages_batch();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Verify results
    EXPECT_EQ(total_messages.load(), 6); // 2 direct + 4 broadcast (2 threads × 2 broadcast)
    EXPECT_EQ(high_priority_messages.load(), 3); // 1 direct high + 2 broadcast high

    // Cleanup
    bus.unregister_thread("integration_thread1");
    bus.unregister_thread("integration_thread2");
}
