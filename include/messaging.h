/*
 * @file messaging.h
 * @brief Type-safe messaging system for inter-thread communication
 *
 * Provides high-performance, type-safe message passing between ManagedThreads
 * with publish-subscribe patterns and direct thread-to-thread messaging.
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <concepts>
#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <source_location>
#include "logger.h"

namespace base {

/**
 * @brief Concept for serializable message types
 */
template<typename T>
concept MessageType = std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>;

/**
 * @brief Unique message ID for tracking and correlation
 */
using MessageId = std::uint64_t;

/**
 * @brief Message priority levels
 */
enum class MessagePriority : std::uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Base message interface for type erasure
 */
class MessageBase {
public:
    MessageBase(MessageId id, MessagePriority priority = MessagePriority::Normal)
        : id_(id), priority_(priority), timestamp_(std::chrono::steady_clock::now()) {}

    virtual ~MessageBase() = default;

    MessageId id() const noexcept { return id_; }
    MessagePriority priority() const noexcept { return priority_; }
    std::chrono::steady_clock::time_point timestamp() const noexcept { return timestamp_; }
    std::type_index type() const noexcept { return type_; }

    // For priority queue ordering (higher priority first)
    bool operator<(const MessageBase& other) const noexcept {
        if (priority_ != other.priority_) {
            return priority_ < other.priority_; // Reverse for max-heap
        }
        return timestamp_ > other.timestamp_; // Older messages first for same priority
    }

protected:
    MessageId id_;
    MessagePriority priority_;
    std::chrono::steady_clock::time_point timestamp_;
    std::type_index type_{typeid(void)};
};

/**
 * @brief Typed message wrapper
 */
template<MessageType T>
class Message : public MessageBase {
public:
    Message(MessageId id, T data, MessagePriority priority = MessagePriority::Normal)
        : MessageBase(id, priority), data_(std::move(data)) {
        type_ = std::type_index(typeid(T));
    }

    const T& data() const noexcept { return data_; }
    T& data() noexcept { return data_; }

private:
    T data_;
};

/**
 * @brief Message handler function type
 */
template<MessageType T>
using MessageHandler = std::function<void(const Message<T>&)>;

/**
 * @brief Event-driven message queue for high-performance inter-thread communication
 * Uses condition variables for immediate message processing with minimal latency
 */
class EventDrivenMessageQueue {
private:
    // Cache-aligned structure for better performance
    struct alignas(64) Node {
        std::atomic<Node*> next{nullptr};
        std::unique_ptr<MessageBase> data{nullptr};
        MessagePriority priority;
        std::chrono::steady_clock::time_point timestamp;

        Node() = default;
        Node(std::unique_ptr<MessageBase> msg)
            : data(std::move(msg))
            , priority(data->priority())
            , timestamp(data->timestamp()) {}
    };

public:
    EventDrivenMessageQueue(std::size_t max_size = 10000) : max_size_(max_size) {
        // Initialize with dummy node for lock-free operation
        Node* dummy = new Node;
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~EventDrivenMessageQueue() {
        shutdown();
        while (Node* node = head_.load()) {
            head_.store(node->next.load());
            delete node;
        }
    }

    /**
     * @brief Send message with event-driven notification
     */
    template<MessageType T>
    bool send(T data, MessagePriority priority = MessagePriority::Normal) {
        // Fast path: check size without lock for performance
        if (size_.load(std::memory_order_relaxed) >= max_size_) {
            return false; // Drop message if queue full
        }

        auto message = std::make_unique<Message<T>>(next_id_.fetch_add(1), std::move(data), priority);
        Node* newNode = new Node(std::move(message));

        // Lock-free enqueue
        Node* prevTail = tail_.exchange(newNode, std::memory_order_acq_rel);
        prevTail->next.store(newNode, std::memory_order_release);

        size_.fetch_add(1, std::memory_order_relaxed);

        // Immediately wake up waiting thread (event-driven)
        condition_.notify_one();
        return true;
    }

    /**
     * @brief Receive message with timeout (event-driven)
     */
    std::unique_ptr<MessageBase> receive(std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(receive_mutex_);

        if (!condition_.wait_for(lock, timeout, [this] {
            return has_messages() || shutdown_.load(std::memory_order_relaxed);
        })) {
            return nullptr; // Timeout
        }

        if (shutdown_.load(std::memory_order_relaxed)) {
            return nullptr;
        }

        return try_dequeue();
    }

    /**
     * @brief Receive multiple messages in batch (event-driven)
     */
    std::vector<std::unique_ptr<MessageBase>> receive_batch(size_t max_batch_size = 32) {
        std::vector<std::unique_ptr<MessageBase>> batch;
        batch.reserve(max_batch_size);

        for (size_t i = 0; i < max_batch_size; ++i) {
            auto msg = try_dequeue();
            if (!msg) break;
            batch.push_back(std::move(msg));
        }

        return batch;
    }

    /**
     * @brief Wait and process messages in batch for optimal performance (event-driven)
     */
    template<typename ProcessFunc>
    bool wait_and_process_batch(ProcessFunc&& processor,
                               std::chrono::milliseconds timeout = std::chrono::milliseconds(100),
                               size_t max_batch_size = 32) {
        std::unique_lock<std::mutex> lock(receive_mutex_);

        if (!condition_.wait_for(lock, timeout, [this] {
            return has_messages() || shutdown_.load(std::memory_order_relaxed);
        })) {
            return false; // Timeout
        }

        if (shutdown_.load(std::memory_order_relaxed)) {
            return false;
        }

        lock.unlock();

        // Process batch without holding lock
        std::vector<std::unique_ptr<MessageBase>> batch;
        batch.reserve(max_batch_size);

        for (size_t i = 0; i < max_batch_size; ++i) {
            auto msg = try_dequeue();
            if (!msg) break;
            batch.push_back(std::move(msg));
        }

        for (auto& message : batch) {
            processor(std::move(message));
        }

        return !batch.empty();
    }

    size_t size() const { return size_.load(std::memory_order_relaxed); }
    bool empty() const { return size() == 0; }

    void shutdown() {
        shutdown_.store(true, std::memory_order_relaxed);
        condition_.notify_all();
    }

private:
    bool has_messages() const {
        Node* head = head_.load(std::memory_order_acquire);
        return head->next.load(std::memory_order_acquire) != nullptr;
    }

    std::unique_ptr<MessageBase> try_dequeue() {
        Node* head = head_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);

        if (!next) {
            return nullptr; // Queue is empty
        }

        // Move head pointer
        head_.store(next, std::memory_order_release);

        auto result = std::move(next->data);
        delete head; // Delete old head

        size_.fetch_sub(1, std::memory_order_relaxed);
        return result;
    }

    // Lock-free queue pointers
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_{0};
    std::atomic<MessageId> next_id_{1};
    std::atomic<bool> shutdown_{false};

    // Receive synchronization (only for blocking operations)
    std::mutex receive_mutex_;
    std::condition_variable condition_;

    const std::size_t max_size_;
};

/**
 * @brief Message router for publish-subscribe patterns
 */
class MessageRouter {
public:
    /**
     * @brief Subscribe to messages of a specific type
     */
    template<MessageType T>
    void subscribe(const std::string& subscriber_name, MessageHandler<T> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_id = std::type_index(typeid(T));

        handlers_[type_id][subscriber_name] = [handler = std::move(handler)](const MessageBase& base_msg) {
            if (auto* typed_msg = dynamic_cast<const Message<T>*>(&base_msg)) {
                handler(*typed_msg);
            }
        };

        Logger::info("Subscriber '{}' registered for message type '{}'",
                    subscriber_name, type_id.name());
    }

    /**
     * @brief Unsubscribe from messages
     */
    template<MessageType T>
    void unsubscribe(const std::string& subscriber_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_id = std::type_index(typeid(T));

        if (auto it = handlers_.find(type_id); it != handlers_.end()) {
            it->second.erase(subscriber_name);
            if (it->second.empty()) {
                handlers_.erase(it);
            }
        }

        Logger::info("Subscriber '{}' unregistered from message type '{}'",
                    subscriber_name, type_id.name());
    }

    /**
     * @brief Publish message to all subscribers
     */
    template<MessageType T>
    void publish(const Message<T>& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_id = std::type_index(typeid(T));

        if (auto it = handlers_.find(type_id); it != handlers_.end()) {
            for (const auto& [subscriber_name, handler] : it->second) {
                try {
                    handler(message);
                    Logger::debug("Message delivered to subscriber '{}'", subscriber_name);
                } catch (const std::exception& e) {
                    Logger::error("Error delivering message to subscriber '{}': {}",
                                subscriber_name, e.what());
                }
            }
        }
    }

    /**
     * @brief Get subscriber count for a message type
     */
    template<MessageType T>
    std::size_t subscriber_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type_id = std::type_index(typeid(T));

        if (auto it = handlers_.find(type_id); it != handlers_.end()) {
            return it->second.size();
        }
        return 0;
    }

    /**
     * @brief Clear all subscriptions
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.clear();
        Logger::info("All message subscriptions cleared");
    }

private:
    using TypedHandler = std::function<void(const MessageBase&)>;
    using SubscriberMap = std::unordered_map<std::string, TypedHandler>;
    using HandlerMap = std::unordered_map<std::type_index, SubscriberMap>;

    mutable std::mutex mutex_;
    HandlerMap handlers_;
};

/**
 * @brief High-performance thread messaging context optimized for production
 * Combines the best features with minimal overhead and cache-friendly design
 */
class ThreadMessagingContext {
public:
    ThreadMessagingContext(std::string thread_name)
        : thread_name_(std::move(thread_name)) {}

    ~ThreadMessagingContext() {
        stop();
    }

    /**
     * @brief Send message to this thread (high-performance path)
     */
    template<MessageType T>
    bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
        return queue_.send(std::move(data), priority);
    }

    /**
     * @brief Process messages in high-performance batch mode
     */
    void process_messages_batch(size_t max_batch_size = 32) {
        auto batch = queue_.receive_batch(max_batch_size);
        if (batch.empty()) return;

        // Single lock acquisition for the entire batch
        std::lock_guard<std::mutex> lock(router_mutex_);

        for (auto& message : batch) {
            auto type_id = message->type();
            if (auto it = handlers_.find(type_id); it != handlers_.end()) {
                for (const auto& [subscriber_name, handler] : it->second) {
                    try {
                        handler(*message);
                    } catch (const std::exception& e) {
                        Logger::error("Error in message handler '{}': {}", subscriber_name, e.what());
                    } catch (...) {
                        Logger::error("Unknown error in message handler '{}'", subscriber_name);
                    }
                }
            }
        }
    }

    /**
     * @brief Wait and process messages with timeout (blocking)
     */
    bool wait_and_process(std::chrono::milliseconds timeout = std::chrono::milliseconds(1),
                         size_t max_batch_size = 32) {
        return queue_.wait_and_process_batch([this](std::unique_ptr<MessageBase> message) {
            process_single_message_lockfree(std::move(message));
        }, timeout, max_batch_size);
    }

    /**
     * @brief Subscribe to message type
     */
    template<MessageType T>
    void subscribe(MessageHandler<T> handler) {
        std::lock_guard<std::mutex> lock(router_mutex_);
        auto type_id = std::type_index(typeid(T));

        handlers_[type_id][thread_name_] = [handler = std::move(handler)](const MessageBase& base_msg) {
            if (auto* typed_msg = dynamic_cast<const Message<T>*>(&base_msg)) {
                handler(*typed_msg);
            }
        };

        Logger::info("Thread '{}' subscribed to message type '{}'", thread_name_, type_id.name());
    }

    /**
     * @brief Unsubscribe from message type
     */
    template<MessageType T>
    void unsubscribe() {
        std::lock_guard<std::mutex> lock(router_mutex_);
        auto type_id = std::type_index(typeid(T));

        if (auto it = handlers_.find(type_id); it != handlers_.end()) {
            it->second.erase(thread_name_);
            if (it->second.empty()) {
                handlers_.erase(it);
            }
        }

        Logger::info("Thread '{}' unsubscribed from message type '{}'", thread_name_, type_id.name());
    }

    /**
     * @brief Start automatic message processing in background
     */
    void start_background_processing() {
        if (processing_thread_.joinable()) return; // Already started

        processing_active_ = true;
        processing_thread_ = std::thread([this]() {
            background_processing_loop();
        });
    }

    /**
     * @brief Stop background processing
     */
    void stop() {
        processing_active_ = false;
        queue_.shutdown();

        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
    }

    // Performance monitoring
    std::size_t pending_message_count() const { return queue_.size(); }
    const std::string& thread_name() const noexcept { return thread_name_; }

private:
    void background_processing_loop() {
        Logger::info("Started background message processing for thread '{}'", thread_name_);

        while (processing_active_) {
            if (!wait_and_process(std::chrono::milliseconds(10), 64)) {
                // Timeout - continue if still active
                if (!processing_active_) break;
            }
        }

        Logger::info("Stopped background message processing for thread '{}'", thread_name_);
    }

    void process_single_message_lockfree(std::unique_ptr<MessageBase> message) {
        // Fast path without lock for single message
        std::lock_guard<std::mutex> lock(router_mutex_);
        auto type_id = message->type();

        if (auto it = handlers_.find(type_id); it != handlers_.end()) {
            for (const auto& [subscriber_name, handler] : it->second) {
                try {
                    handler(*message);
                } catch (const std::exception& e) {
                    Logger::error("Error in message handler '{}': {}", subscriber_name, e.what());
                } catch (...) {
                    Logger::error("Unknown error in message handler '{}'", subscriber_name);
                }
            }
        }
    }

    using TypedHandler = std::function<void(const MessageBase&)>;
    using SubscriberMap = std::unordered_map<std::string, TypedHandler>;
    using HandlerMap = std::unordered_map<std::type_index, SubscriberMap>;

    std::string thread_name_;
    EventDrivenMessageQueue queue_;

    // Message routing
    mutable std::mutex router_mutex_;
    HandlerMap handlers_;

    // Background processing
    std::atomic<bool> processing_active_{false};
    std::thread processing_thread_;
};

/**
 * @brief Global messaging bus for inter-thread communication
 */
class MessagingBus {
public:
    static MessagingBus& instance() {
        static MessagingBus instance;
        return instance;
    }

    /**
     * @brief Register a thread for messaging
     */
    void register_thread(const std::string& thread_name,
                        std::shared_ptr<ThreadMessagingContext> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        contexts_[thread_name] = std::move(context);
        Logger::info("Thread '{}' registered with messaging bus", thread_name);
    }

    /**
     * @brief Unregister a thread
     */
    void unregister_thread(const std::string& thread_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        contexts_.erase(thread_name);
        Logger::info("Thread '{}' unregistered from messaging bus", thread_name);
    }

    /**
     * @brief Send message to specific thread
     */
    template<MessageType T>
    bool send_to_thread(const std::string& target_thread, T data,
                       MessagePriority priority = MessagePriority::Normal) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (auto it = contexts_.find(target_thread); it != contexts_.end()) {
            return it->second->send_message(std::move(data), priority);
        }

        Logger::warn("Target thread '{}' not found for message delivery", target_thread);
        return false;
    }

    /**
     * @brief Broadcast message to all threads
     */
    template<MessageType T>
    void broadcast(T data, MessagePriority priority = MessagePriority::Normal) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& [thread_name, context] : contexts_) {
            context->send_message(data, priority);
        }

        Logger::debug("Message broadcast to {} threads", contexts_.size());
    }

    /**
     * @brief Get registered thread count
     */
    std::size_t thread_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return contexts_.size();
    }

    /**
     * @brief Check if thread is registered
     */
    bool is_thread_registered(const std::string& thread_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return contexts_.find(thread_name) != contexts_.end();
    }

private:
    MessagingBus() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<ThreadMessagingContext>> contexts_;
};

} // namespace base
