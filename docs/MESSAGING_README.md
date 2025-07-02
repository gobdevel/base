# Crux Messaging System

The Crux messaging system provides a type-safe, high-performance messaging infrastructure for inter-thread communication, designed for microservice and event-driven architectures.

## Features

- **Type-Safe Messaging**: Template-based messages ensure compile-time type safety
- **Priority-Based Queue**: Messages can be prioritized (Critical, High, Normal, Low)
- **Thread-Safe Operations**: All messaging operations are thread-safe and lock-free where possible
- **Publish/Subscribe Pattern**: Threads can subscribe to specific message types
- **Global Messaging Bus**: Centralized message routing between threads
- **Performance Optimized**: High-throughput message processing (>600K msg/sec)
- **Memory Efficient**: Smart pointer-based message management
- **Asynchronous Processing**: Non-blocking message sending and receiving

## Architecture

### Core Components

1. **MessageBase**: Abstract base class for all messages
2. **Message<T>**: Type-safe message wrapper with priority and metadata
3. **MessageQueue**: Thread-safe priority queue for message storage
4. **MessageRouter**: Publish/subscribe message routing
5. **ThreadMessagingContext**: Per-thread messaging interface
6. **MessagingBus**: Global singleton for inter-thread messaging

### Message Flow

```
[Thread A] --send--> [MessagingBus] --route--> [Thread B MessageQueue] --process--> [Handler]
```

## Basic Usage

### 1. Define Message Types

```cpp
struct OrderMessage {
    int order_id;
    std::string product;
    double price;

    OrderMessage(int id, std::string prod, double p)
        : order_id(id), product(std::move(prod)), price(p) {}
};
```

### 2. Create Threads and Subscribe to Messages

```cpp
// Create application and threads
Application app;
auto processor = app.create_worker_thread("order-processor");

// Subscribe to message type
processor->subscribe_to_messages<OrderMessage>([](const Message<OrderMessage>& msg) {
    const auto& order = msg.data();
    Logger::info("Processing order #{}: {}", order.order_id, order.product);
});
```

### 3. Send Messages

```cpp
// Send message to specific thread
app.send_message_to_thread("order-processor",
    OrderMessage{123, "Laptop", 999.99}, MessagePriority::High);

// Send directly through thread
processor->send_message(OrderMessage{124, "Phone", 599.99});

// Broadcast to all subscribers
app.broadcast_message(OrderMessage{125, "Tablet", 299.99});
```

## Advanced Features

### Message Priorities

Messages support four priority levels:

```cpp
enum class MessagePriority {
    Critical = 0,  // Highest priority
    High = 1,
    Normal = 2,
    Low = 3        // Lowest priority
};
```

Higher priority messages are processed first, with timestamp-based FIFO ordering within the same priority.

### Message Metadata

Every message includes:

- **Unique ID**: Auto-generated UUID
- **Timestamp**: Creation time (high-resolution)
- **Priority**: Processing priority
- **Type Info**: Runtime type information

### Thread Safety

All messaging operations are thread-safe:

- Lock-free message ID generation
- Atomic reference counting for messages
- Mutex-protected message queues
- Thread-safe subscription management

## Performance Characteristics

- **Throughput**: >600,000 messages/second
- **Latency**: Sub-microsecond message sending
- **Memory**: Efficient shared_ptr-based message storage
- **Scalability**: Supports unlimited threads and message types

## Integration with Application Framework

The messaging system is fully integrated with the Crux Application Framework:

```cpp
class MyApp : public Application {
    void setup() {
        // Create specialized service threads
        auto auth_service = create_worker_thread("auth");
        auto data_service = create_worker_thread("data");
        auto notification_service = create_worker_thread("notify");

        // Set up message routing
        auth_service->subscribe_to_messages<AuthRequest>([this](const auto& msg) {
            // Handle authentication
            send_message_to_thread("data", DataRequest{msg.data().user_id});
        });

        data_service->subscribe_to_messages<DataRequest>([this](const auto& msg) {
            // Process data request
            send_message_to_thread("notify", NotifyUser{msg.data().user_id, "Data ready"});
        });
    }
};
```

## Error Handling

### Message Validation

- Type safety prevents incorrect message types
- Thread existence validated before sending
- Queue capacity monitoring available

### Error Recovery

```cpp
// Check if message was sent successfully
if (!app.send_message_to_thread("service", MyMessage{})) {
    Logger::error("Failed to send message - thread not found");
}

// Check pending message count
size_t pending = thread->pending_message_count();
if (pending > 1000) {
    Logger::warn("High message backlog: {} pending", pending);
}
```

## Best Practices

### 1. Message Design

- Keep messages lightweight and serializable
- Use move semantics for large data
- Include correlation IDs for request/response patterns

```cpp
struct RequestMessage {
    std::string correlation_id;
    RequestData data;

    RequestMessage(std::string id, RequestData d)
        : correlation_id(std::move(id)), data(std::move(d)) {}
};
```

### 2. Thread Organization

- Create specialized threads for different services
- Use descriptive thread names for debugging
- Limit the number of message types per thread

### 3. Performance Optimization

- Use appropriate message priorities
- Batch related operations when possible
- Monitor queue depths and processing times

```cpp
// Use higher priority for time-critical messages
app.send_message_to_thread("emergency", AlertMessage{}, MessagePriority::Critical);

// Batch operations
std::vector<DataItem> batch;
// ... collect items ...
app.send_message_to_thread("processor", BatchMessage{std::move(batch)});
```

## Example: Microservice Communication

See `src/messaging_example.cpp` for a complete microservice-style example demonstrating:

- Order processing workflow
- Payment service integration
- Inventory management
- Notification system
- Cross-service communication

## Thread Safety Guarantees

- **Send Operations**: Thread-safe, can be called from any thread
- **Subscription Management**: Thread-safe registration/unregistration
- **Message Processing**: Isolated per thread, no shared mutable state
- **Bus Operations**: Globally thread-safe

## Memory Management

- Messages use `std::shared_ptr` for safe sharing
- Automatic cleanup when all references are released
- No manual memory management required
- RAII-based resource management throughout

## Monitoring and Debugging

### Built-in Metrics

```cpp
// Check system health
Logger::info("Active threads: {}", app.managed_thread_count());
Logger::info("Pending messages: {}", thread->pending_message_count());

// Message flow tracing
Logger::debug("Message sent: id={}, type={}", msg.id(), msg.type_name());
```

### Thread Identification

All messaging operations include thread identification for debugging:

```
[2025-07-01 22:44:38.181] [crux] [info] Thread 'order-processor' subscribed to message type 'OrderMessage'
[2025-07-01 22:44:38.191] [crux] [info] Processing order #1: 2 x Laptop @ $150.00
```

## Testing Support

The messaging system includes comprehensive test coverage:

- Unit tests for all core components
- Integration tests with application framework
- Performance benchmarks
- Error condition testing

Run tests with:

```bash
./build/Release/tests/test_crux
```

## Dependencies

- **C++20**: Requires concepts, chrono, and modern STL
- **ASIO**: For event loop integration
- **spdlog**: For logging integration
- **Google Test**: For unit testing

## Future Enhancements

Potential areas for extension:

- **Message Persistence**: Durable message queues
- **Network Messaging**: Inter-process communication
- **Message Correlation**: Built-in request/response patterns
- **Flow Control**: Backpressure and rate limiting
- **Message Encryption**: Security for sensitive data
- **Distributed Tracing**: Cross-service observability

---

The Crux messaging system provides a solid foundation for building scalable, maintainable microservice architectures with C++20's modern features and type safety guarantees.
