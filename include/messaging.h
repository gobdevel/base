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
#include <mutex>
#include <condition_variable>
#include <source_location>
#include "logger.h"

namespace crux {

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
 * @brief Thread-safe message queue with priority support
 */
class MessageQueue {
public:
    MessageQueue(std::size_t max_size = 10000) : max_size_(max_size) {}

    /**
     * @brief Send a message to the queue
     */
    template<MessageType T>
    bool send(T data, MessagePriority priority = MessagePriority::Normal,
              const std::source_location& loc = std::source_location::current()) {
        auto message = std::make_unique<Message<T>>(next_id_++, std::move(data), priority);

        std::unique_lock<std::mutex> lock(mutex_);

        if (messages_.size() >= max_size_) {
            Logger::warn("Message queue full, dropping message (type: {}, location: {}:{})",
                        typeid(T).name(), loc.file_name(), loc.line());
            return false;
        }

        // Insert message in priority order
        MessageId msg_id = message->id();
        // Convert to MessageBase pointer for comparison
        std::unique_ptr<MessageBase> msg_base = std::move(message);
        auto insert_pos = std::upper_bound(messages_.begin(), messages_.end(), msg_base,
            [](const std::unique_ptr<MessageBase>& a, const std::unique_ptr<MessageBase>& b) {
                return a->priority() > b->priority() ||
                       (a->priority() == b->priority() && a->timestamp() < b->timestamp());
            });

        messages_.insert(insert_pos, std::move(msg_base));

        lock.unlock();
        condition_.notify_one();

        Logger::debug("Message sent (type: {}, id: {}, priority: {})",
                     typeid(T).name(), msg_id, static_cast<int>(priority));
        return true;
    }

    /**
     * @brief Receive next message (blocking)
     */
    std::unique_ptr<MessageBase> receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !messages_.empty() || shutdown_; });

        if (shutdown_ && messages_.empty()) {
            return nullptr;
        }

        auto message = std::move(messages_.front());
        messages_.erase(messages_.begin());
        return message;
    }

    /**
     * @brief Try to receive message (non-blocking)
     */
    std::unique_ptr<MessageBase> try_receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (messages_.empty()) {
            return nullptr;
        }

        auto message = std::move(messages_.front());
        messages_.erase(messages_.begin());
        return message;
    }

    /**
     * @brief Get queue size
     */
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_.size();
    }

    /**
     * @brief Check if queue is empty
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_.empty();
    }

    /**
     * @brief Shutdown the queue
     */
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        condition_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<std::unique_ptr<MessageBase>> messages_;
    std::size_t max_size_;
    std::atomic<MessageId> next_id_{1};
    std::atomic<bool> shutdown_{false};
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
 * @brief Thread messaging context - manages messaging for a single thread
 */
class ThreadMessagingContext {
public:
    ThreadMessagingContext(std::string thread_name)
        : thread_name_(std::move(thread_name)) {}

    ~ThreadMessagingContext() {
        queue_.shutdown();
    }

    /**
     * @brief Send message to this thread
     */
    template<MessageType T>
    bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
        return queue_.send(std::move(data), priority);
    }

    /**
     * @brief Process pending messages
     */
    void process_messages() {
        while (auto message = queue_.try_receive()) {
            // Find handlers for this message type
            std::lock_guard<std::mutex> lock(router_mutex_);
            auto type_id = message->type();

            if (auto it = handlers_.find(type_id); it != handlers_.end()) {
                for (const auto& [subscriber_name, handler] : it->second) {
                    try {
                        handler(*message);
                        Logger::debug("Message delivered to subscriber '{}'", subscriber_name);
                    } catch (const std::exception& e) {
                        Logger::error("Error delivering message to subscriber '{}': {}",
                                    subscriber_name, e.what());
                    }
                }
            }
        }
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
     * @brief Get pending message count
     */
    std::size_t pending_message_count() const {
        return queue_.size();
    }

    const std::string& thread_name() const noexcept { return thread_name_; }

private:
    using TypedHandler = std::function<void(const MessageBase&)>;
    using SubscriberMap = std::unordered_map<std::string, TypedHandler>;
    using HandlerMap = std::unordered_map<std::type_index, SubscriberMap>;

    std::string thread_name_;
    MessageQueue queue_;
    mutable std::mutex router_mutex_;
    HandlerMap handlers_;
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

} // namespace crux
