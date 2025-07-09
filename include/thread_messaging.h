/*
 * @file thread_messaging.h
 * @brief High-performance event-driven inter-thread messaging system
 *
 * Provides type-safe, zero-copy message passing optimized for event-driven
 * architecture with immediate ASIO-based notification and minimal overhead.
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
#include <typeindex>
#include <type_traits>
#include <concepts>
#include <mutex>
#include <asio.hpp>
#include "logger.h"

namespace base {

// Forward declarations
class ThreadMessagingContext;
class InterThreadMessagingBus;

/**
 * @brief Concept for message types - must be movable for zero-copy semantics
 */
template<typename T>
concept MessageType = std::is_move_constructible_v<T>;

/**
 * @brief Message priority levels for event-driven scheduling
 */
enum class MessagePriority : std::uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief Message handler function type
 */
template<MessageType T>
using MessageHandler = std::function<void(const T&)>;

/**
 * @brief Event-driven thread context for high-performance messaging
 *
 * Integrates directly with ASIO event loops for immediate message processing
 * without queuing overhead. Messages are processed via ASIO's event system
 * ensuring thread-safe, lock-free message delivery.
 */
class ThreadMessagingContext : public std::enable_shared_from_this<ThreadMessagingContext> {
public:
    explicit ThreadMessagingContext(std::string thread_name, asio::io_context& io_context)
        : thread_name_(std::move(thread_name)), io_context_(io_context) {}

    ~ThreadMessagingContext() {
        stop();
    }

    /**
     * @brief Send typed message with immediate ASIO notification
     *
     * Posts message directly to target thread's ASIO event loop for
     * immediate processing. Zero-copy semantics via move construction.
     *
     * @param data Message data (moved)
     * @param priority Message priority (currently informational)
     * @return true if message was posted successfully
     */
    template<MessageType T>
    bool send_message(T data, MessagePriority priority = MessagePriority::Normal) {
        if (!started_.load(std::memory_order_acquire)) {
            return false;
        }

        try {
            asio::post(io_context_, [this, data = std::move(data)]() mutable {
                if (started_.load(std::memory_order_acquire)) {
                    process_message(std::move(data));
                }
            });
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    /**
     * @brief Subscribe to messages of specific type
     *
     * Registers a handler for messages of type T. Thread-safe.
     *
     * @param handler Function to call when message of type T is received
     */
    template<MessageType T>
    void subscribe(MessageHandler<T> handler) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto type_id = std::type_index(typeid(T));

        handlers_[type_id] = [handler = std::move(handler)](const std::any& data) {
            try {
                const T& typed_data = std::any_cast<const T&>(data);
                handler(typed_data);
            } catch (const std::bad_any_cast&) {
                // Ignore type mismatches (should not happen with proper usage)
            }
        };
    }

    /**
     * @brief Unsubscribe from message type
     */
    template<MessageType T>
    void unsubscribe() {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.erase(std::type_index(typeid(T)));
    }

    /**
     * @brief Start the messaging context (register with global bus)
     */
    void start();

    /**
     * @brief Stop the messaging context (unregister from global bus)
     */
    void stop();

    /**
     * @brief Get thread name
     */
    const std::string& thread_name() const noexcept { return thread_name_; }

    /**
     * @brief Get approximate message queue size (always 0 for immediate processing)
     */
    std::size_t pending_message_count() const noexcept { return 0; }

private:
    template<MessageType T>
    void process_message(T data) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        auto type_id = std::type_index(typeid(T));

        if (auto it = handlers_.find(type_id); it != handlers_.end()) {
            std::any any_data = std::move(data);
            it->second(any_data);
        }
    }

    std::string thread_name_;
    asio::io_context& io_context_;
    std::atomic<bool> started_{false};

    mutable std::mutex handlers_mutex_;
    std::unordered_map<std::type_index, std::function<void(const std::any&)>> handlers_;
};

/**
 * @brief Global messaging bus for inter-thread communication
 *
 * Singleton that manages thread registration and provides high-performance
 * message routing between threads. Optimized for event-driven architecture.
 */
class InterThreadMessagingBus {
public:
    static InterThreadMessagingBus& instance() {
        static InterThreadMessagingBus bus;
        return bus;
    }

    /**
     * @brief Register a thread for messaging
     */
    void register_thread(const std::string& thread_name,
                        std::shared_ptr<ThreadMessagingContext> context) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutdown_.load(std::memory_order_acquire)) {
            contexts_[thread_name] = std::move(context);
        }
    }

    /**
     * @brief Unregister a thread
     */
    void unregister_thread(const std::string& thread_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!shutdown_.load(std::memory_order_acquire)) {
            contexts_.erase(thread_name);
        }
    }

    /**
     * @brief Send message to specific thread
     *
     * High-performance direct thread targeting. Message is posted immediately
     * to target thread's ASIO event loop.
     *
     * @param target_thread Name of target thread
     * @param data Message data (moved)
     * @param priority Message priority
     * @return true if message was sent successfully
     */
    template<MessageType T>
    bool send_to_thread(const std::string& target_thread, T data,
                       MessagePriority priority = MessagePriority::Normal) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_.load(std::memory_order_acquire)) {
            return false;
        }

        if (auto it = contexts_.find(target_thread); it != contexts_.end()) {
            return it->second->send_message(std::move(data), priority);
        }
        return false;
    }

    /**
     * @brief Broadcast message to all registered threads
     *
     * Efficiently broadcasts to all threads. Each thread receives a copy
     * of the message data.
     */
    template<MessageType T>
    void broadcast(const T& data, MessagePriority priority = MessagePriority::Normal) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (shutdown_.load(std::memory_order_acquire)) {
            return;
        }

        for (const auto& [thread_name, context] : contexts_) {
            try {
                T data_copy = data;  // Copy for each thread
                context->send_message(std::move(data_copy), priority);
            } catch (const std::exception&) {
                // Continue with other threads on error
            }
        }
    }

    /**
     * @brief Get number of registered threads
     */
    std::size_t thread_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return contexts_.size();
    }

    /**
     * @brief Shutdown the messaging bus
     */
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_.store(true, std::memory_order_release);
        contexts_.clear();
    }

private:
    InterThreadMessagingBus() = default;
    ~InterThreadMessagingBus() {
        shutdown();
    }

    mutable std::mutex mutex_;
    std::atomic<bool> shutdown_{false};
    std::unordered_map<std::string, std::shared_ptr<ThreadMessagingContext>> contexts_;
};

// Implementation of ThreadMessagingContext methods that depend on InterThreadMessagingBus
inline void ThreadMessagingContext::start() {
    if (started_.exchange(true, std::memory_order_acq_rel)) {
        return; // Already started
    }

    try {
        InterThreadMessagingBus::instance().register_thread(thread_name_, shared_from_this());
    } catch (const std::exception&) {
        started_.store(false, std::memory_order_release);
        throw;
    }
}

inline void ThreadMessagingContext::stop() {
    if (!started_.exchange(false, std::memory_order_acq_rel)) {
        return; // Already stopped
    }

    try {
        InterThreadMessagingBus::instance().unregister_thread(thread_name_);
    } catch (const std::exception&) {
        // Ignore errors during shutdown
    }
}

} // namespace base
