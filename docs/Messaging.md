# Messaging System

The Base framework provides a high-performance, type-safe messaging system designed for inter-thread communication with publish-subscribe patterns, priority queues, and event-driven processing.

## 🚀 Overview

The messaging system is the core communication layer that enables:

- **Type-Safe Communication**: Compile-time type checking with C++20 concepts
- **High Performance**: 100,000+ messages/second with sub-microsecond latency
- **Event-Driven Processing**: Immediate message processing with condition variables
- **Priority Support**: Critical/High/Normal/Low priority message handling
- **Publish/Subscribe**: Event-driven message routing and broadcasting
- **Lock-Free Operations**: Optimized for minimal contention and maximum throughput

## 📋 Key Components

### MessageType Concept

All messages must satisfy the MessageType concept:

```cpp
template<typename T>
concept MessageType = std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>;

// Valid message types
struct OrderMessage {
    int order_id;
    double amount;
    std::string customer_id;
};

struct StatusMessage {
    std::string status;
    std::chrono::steady_clock::time_point timestamp;
};
```

### Message Priorities

```cpp
enum class MessagePriority : std::uint8_t {
    Low = 0,        // Background tasks
    Normal = 1,     // Default priority
    High = 2,       // Important operations
    Critical = 3    // Emergency/system messages
};
```

### Message Structure

```cpp
template<MessageType T>
class Message : public MessageBase {
public:
    Message(MessageId id, T data, MessagePriority priority = MessagePriority::Normal);

    const T& data() const noexcept;
    T& data() noexcept;

    MessageId id() const noexcept;
    MessagePriority priority() const noexcept;
    std::chrono::steady_clock::time_point timestamp() const noexcept;
};
```

## 🏗️ Architecture

### High-Performance Message Queue

The core messaging component uses an event-driven queue for optimal performance:

```cpp
class EventDrivenMessageQueue {
public:
    // Send message (event-driven notification)
    template<MessageType T>
    bool send(T data, MessagePriority priority = MessagePriority::Normal);

    // Receive with timeout (event-driven)
    std::unique_ptr<MessageBase> receive(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

    // Batch processing for maximum throughput (event-driven)
    std::vector<std::unique_ptr<MessageBase>> receive_batch(size_t max_batch_size = 32);

    // Wait and process batch (event-driven)
    template<typename ProcessFunc>
    bool wait_and_process_batch(ProcessFunc&& processor,
                               std::chrono::milliseconds timeout = std::chrono::milliseconds(100),
                               size_t max_batch_size = 32);
};
```

### Thread Messaging Context

Each thread has its own messaging context for isolation and performance:

```cpp
class ThreadMessagingContext {
public:
    // Send message to this thread
    template<MessageType T>
    bool send_message(T data, MessagePriority priority = MessagePriority::Normal);

    // Subscribe to message type
    template<MessageType T>
    void subscribe(MessageHandler<T> handler);

    // Process messages in batch
    void process_messages_batch(size_t max_batch_size = 32);

    // Wait and process with timeout
    bool wait_and_process(std::chrono::milliseconds timeout = std::chrono::milliseconds(1),
                         size_t max_batch_size = 32);

    // Background processing
    void start_background_processing();
    void stop();
};
```

### Global Messaging Bus

The messaging bus coordinates communication between threads:

```cpp
class MessagingBus {
public:
    static MessagingBus& instance();

    // Register thread for messaging
    void register_thread(const std::string& thread_name,
                        std::shared_ptr<ThreadMessagingContext> context);

    // Send message to specific thread
    template<MessageType T>
    bool send_to_thread(const std::string& target_thread, T data,
                       MessagePriority priority = MessagePriority::Normal);

    // Broadcast to all threads
    template<MessageType T>
    void broadcast(T data, MessagePriority priority = MessagePriority::Normal);

    // Thread management
    void unregister_thread(const std::string& thread_name);
    std::size_t thread_count() const;
    bool is_thread_registered(const std::string& thread_name) const;
};
```

## 🔧 Basic Usage

### Simple Message Passing

```cpp
// Define message type
struct WorkMessage {
    int task_id;
    std::string data;
};

// Send message
app.send_message("worker-thread", WorkMessage{123, "process this"});

// Handle messages
app.subscribe<WorkMessage>([](const auto& msg) {
    Logger::info("Processing task {}: {}", msg.data().task_id, msg.data().data);
});
```

### Priority Messages

```cpp
// Send high-priority message
app.send_message("emergency-handler",
                EmergencyMessage{"System overload detected"},
                MessagePriority::Critical);

// Send low-priority cleanup task
app.send_message("cleanup-worker",
                CleanupMessage{"temp files"},
                MessagePriority::Low);
```

### Publish/Subscribe Pattern

```cpp
// Subscribe to order events
app.subscribe<OrderMessage>([](const auto& msg) {
    Logger::info("New order: #{} for ${}", msg.data().order_id, msg.data().amount);
});

// Subscribe to status events
app.subscribe<StatusMessage>([](const auto& msg) {
    Logger::info("Status update: {}", msg.data().status);
});

// Publish events
MessagingBus::instance().broadcast(OrderMessage{123, 99.99, "customer123"});
MessagingBus::instance().broadcast(StatusMessage{"Order processed"});
```

## 🚀 Advanced Usage

### Request-Response Pattern

```cpp
// Request message
struct CalculationRequest {
    int request_id;
    double value_a;
    double value_b;
    std::string operation;
};

// Response message
struct CalculationResponse {
    int request_id;
    double result;
    bool success;
};

// Client thread
class CalculatorClient {
public:
    void send_calculation(double a, double b, const std::string& op) {
        int request_id = next_request_id_++;

        // Store pending request
        pending_requests_[request_id] = std::make_shared<std::promise<double>>();

        // Send request
        app_.send_message("calculator-server",
                         CalculationRequest{request_id, a, b, op});

        // Wait for response
        auto future = pending_requests_[request_id]->get_future();
        auto result = future.get();

        Logger::info("Calculation result: {}", result);
    }

private:
    void handle_response(const CalculationResponse& response) {
        if (auto it = pending_requests_.find(response.request_id);
            it != pending_requests_.end()) {

            if (response.success) {
                it->second->set_value(response.result);
            } else {
                it->second->set_exception(std::make_exception_ptr(
                    std::runtime_error("Calculation failed")));
            }

            pending_requests_.erase(it);
        }
    }

    std::atomic<int> next_request_id_{1};
    std::unordered_map<int, std::shared_ptr<std::promise<double>>> pending_requests_;
};

// Server thread
class CalculatorServer {
public:
    void initialize() {
        app_.subscribe<CalculationRequest>([this](const auto& msg) {
            handle_request(msg.data());
        });
    }

private:
    void handle_request(const CalculationRequest& request) {
        double result = 0.0;
        bool success = true;

        try {
            if (request.operation == "add") {
                result = request.value_a + request.value_b;
            } else if (request.operation == "multiply") {
                result = request.value_a * request.value_b;
            } else {
                success = false;
            }
        } catch (...) {
            success = false;
        }

        // Send response
        app_.send_message("calculator-client",
                         CalculationResponse{request.request_id, result, success});
    }
};
```

### Event Streaming

```cpp
// Stream processor
class EventProcessor {
public:
    void initialize() {
        // Subscribe to raw events
        app_.subscribe<RawEvent>([this](const auto& msg) {
            process_raw_event(msg.data());
        });

        // Start processing in batches
        app_.create_managed_thread("event-processor", [this]() {
            while (app_.is_running()) {
                process_event_batch();
            }
        });
    }

private:
    void process_raw_event(const RawEvent& event) {
        // Buffer events for batch processing
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        event_buffer_.push_back(event);

        if (event_buffer_.size() >= batch_size_) {
            buffer_ready_.notify_one();
        }
    }

    void process_event_batch() {
        std::unique_lock<std::mutex> lock(buffer_mutex_);

        // Wait for batch or timeout
        buffer_ready_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
            return event_buffer_.size() >= batch_size_;
        });

        if (event_buffer_.empty()) return;

        // Process batch
        auto batch = std::move(event_buffer_);
        event_buffer_.clear();
        lock.unlock();

        // Process events
        for (const auto& event : batch) {
            auto processed = transform_event(event);
            app_.send_message("event-sink", processed);
        }

        Logger::info("Processed batch of {} events", batch.size());
    }

    std::vector<RawEvent> event_buffer_;
    std::mutex buffer_mutex_;
    std::condition_variable buffer_ready_;
    const size_t batch_size_ = 100;
};
```

### Message Filtering and Routing

```cpp
// Message router with filtering
class MessageRouter {
public:
    void initialize() {
        // Subscribe to all message types
        app_.subscribe<OrderMessage>([this](const auto& msg) {
            route_order_message(msg.data());
        });

        app_.subscribe<PaymentMessage>([this](const auto& msg) {
            route_payment_message(msg.data());
        });
    }

private:
    void route_order_message(const OrderMessage& order) {
        // Route based on order amount
        if (order.amount > 1000.0) {
            app_.send_message("high-value-processor", order, MessagePriority::High);
        } else {
            app_.send_message("standard-processor", order, MessagePriority::Normal);
        }

        // Also send to analytics
        app_.send_message("analytics-processor", order, MessagePriority::Low);
    }

    void route_payment_message(const PaymentMessage& payment) {
        // Route based on payment method
        if (payment.method == "credit_card") {
            app_.send_message("credit-card-processor", payment);
        } else if (payment.method == "bank_transfer") {
            app_.send_message("bank-transfer-processor", payment);
        }
    }
};
```

## 📊 Performance Optimization

### Batch Processing

```cpp
// Optimized batch processing
class HighThroughputProcessor {
public:
    void initialize() {
        // Create dedicated processing thread
        app_.create_managed_thread("batch-processor", [this]() {
            process_messages_optimized();
        });
    }

private:
    void process_messages_optimized() {
        while (app_.is_running()) {
            // Process messages in large batches
            auto context = MessagingBus::instance().get_context("batch-processor");

            if (context->wait_and_process(std::chrono::milliseconds(10), 1000)) {
                // Successfully processed batch
                batch_count_++;

                // Log performance metrics periodically
                if (batch_count_ % 100 == 0) {
                    Logger::info("Processed {} batches", batch_count_);
                }
            }
        }
    }

    std::atomic<uint64_t> batch_count_{0};
};
```

### Memory Pool Integration

```cpp
// Memory pool for message allocation
class MemoryEfficientMessaging {
public:
    void initialize() {
        // Pre-allocate message pool
        message_pool_.reserve(10000);

        // Subscribe with memory-efficient processing
        app_.subscribe<DataMessage>([this](const auto& msg) {
            process_data_efficient(msg.data());
        });
    }

private:
    void process_data_efficient(const DataMessage& data) {
        // Reuse memory from pool
        auto buffer = get_buffer_from_pool();

        // Process data
        process_data(data, buffer);

        // Return buffer to pool
        return_buffer_to_pool(buffer);
    }

    std::vector<std::unique_ptr<uint8_t[]>> message_pool_;
    std::queue<std::unique_ptr<uint8_t[]>> available_buffers_;
    std::mutex pool_mutex_;
};
```

## 🔍 Monitoring and Debugging

### Performance Metrics

```cpp
// Message performance monitoring
class MessageMetrics {
public:
    void track_message_sent(const std::string& thread_name) {
        sent_count_[thread_name]++;
        total_sent_++;
    }

    void track_message_processed(const std::string& thread_name,
                                std::chrono::microseconds processing_time) {
        processed_count_[thread_name]++;
        total_processed_++;

        // Track processing time
        processing_times_[thread_name].push_back(processing_time);

        // Keep only recent measurements
        if (processing_times_[thread_name].size() > 1000) {
            processing_times_[thread_name].erase(processing_times_[thread_name].begin());
        }
    }

    void print_metrics() const {
        Logger::info("Message Metrics:");
        Logger::info("  Total sent: {}", total_sent_.load());
        Logger::info("  Total processed: {}", total_processed_.load());

        for (const auto& [thread_name, count] : sent_count_) {
            Logger::info("  Thread {}: {} sent", thread_name, count);
        }

        for (const auto& [thread_name, times] : processing_times_) {
            if (!times.empty()) {
                auto avg = std::accumulate(times.begin(), times.end(),
                                         std::chrono::microseconds{0}) / times.size();
                Logger::info("  Thread {}: avg processing time {}μs",
                           thread_name, avg.count());
            }
        }
    }

private:
    std::atomic<uint64_t> total_sent_{0};
    std::atomic<uint64_t> total_processed_{0};
    std::unordered_map<std::string, uint64_t> sent_count_;
    std::unordered_map<std::string, uint64_t> processed_count_;
    std::unordered_map<std::string, std::vector<std::chrono::microseconds>> processing_times_;
};
```

### Message Tracing

```cpp
// Message tracing for debugging
class MessageTracer {
public:
    void trace_message_send(const std::string& from_thread,
                           const std::string& to_thread,
                           const std::string& message_type,
                           MessageId message_id) {
        Logger::debug("MSG_SEND: {} -> {} (type: {}, id: {})",
                     from_thread, to_thread, message_type, message_id);
    }

    void trace_message_receive(const std::string& thread_name,
                              const std::string& message_type,
                              MessageId message_id) {
        Logger::debug("MSG_RECV: {} (type: {}, id: {})",
                     thread_name, message_type, message_id);
    }

    void trace_message_process(const std::string& thread_name,
                              const std::string& message_type,
                              MessageId message_id,
                              std::chrono::microseconds processing_time) {
        Logger::debug("MSG_PROC: {} (type: {}, id: {}, time: {}μs)",
                     thread_name, message_type, message_id, processing_time.count());
    }
};
```

## 🏆 Best Practices

### Message Design

- **Keep Messages Small**: Prefer small, focused messages over large ones
- **Use Move Semantics**: Leverage std::move for large data structures
- **Avoid Pointers**: Use value types or smart pointers instead of raw pointers
- **Immutable Data**: Design messages to be immutable when possible

### Threading Patterns

- **Single Producer, Single Consumer**: Most efficient pattern
- **Batch Processing**: Process messages in batches for better throughput
- **Backpressure Handling**: Implement proper backpressure mechanisms
- **Error Isolation**: Handle errors gracefully without affecting other threads

### Performance Optimization

- **Lock-Free Operations**: Use lock-free data structures where possible
- **Memory Pooling**: Reuse memory allocations for frequently used messages
- **CPU Affinity**: Pin threads to specific CPU cores for consistent performance
- **Profiling**: Regular performance profiling and optimization

## 🔧 Integration Examples

### HTTP Server Integration

```cpp
class HttpServerIntegration {
public:
    void initialize() {
        // Subscribe to HTTP request messages
        app_.subscribe<HttpRequestMessage>([this](const auto& msg) {
            handle_http_request(msg.data());
        });

        // Subscribe to HTTP response messages
        app_.subscribe<HttpResponseMessage>([this](const auto& msg) {
            send_http_response(msg.data());
        });
    }

private:
    void handle_http_request(const HttpRequestMessage& request) {
        // Route to appropriate handler
        if (request.path.starts_with("/api/orders")) {
            app_.send_message("order-service", request);
        } else if (request.path.starts_with("/api/payments")) {
            app_.send_message("payment-service", request);
        }
    }
};
```

### Database Integration

```cpp
class DatabaseIntegration {
public:
    void initialize() {
        // Subscribe to database operations
        app_.subscribe<DatabaseQueryMessage>([this](const auto& msg) {
            execute_query(msg.data());
        });

        // Create database worker pool
        for (int i = 0; i < 4; ++i) {
            app_.create_managed_thread("db-worker-" + std::to_string(i), [this]() {
                database_worker_loop();
            });
        }
    }

private:
    void database_worker_loop() {
        while (app_.is_running()) {
            // Process database messages
            auto context = MessagingBus::instance().get_context(
                "db-worker-" + std::to_string(worker_id_));

            context->wait_and_process(std::chrono::milliseconds(10), 100);
        }
    }
};
```

---

_This documentation covers the Messaging System component of the Base framework. For complete system documentation, see the [main documentation](README.md)._
