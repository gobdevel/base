# Crux CLI - Runtime Inspection and Debugging

The Crux CLI provides runtime inspection and debugging capabilities for Crux applications. It offers both stdin-based and optional TCP-based interfaces for monitoring and controlling application internals in real-time.

## Features

- **Real-time application monitoring**: View application status, thread information, and health
- **Runtime configuration inspection**: Examine current configuration values
- **Messaging system statistics**: Monitor message queues and throughput
- **Dynamic log level control**: Change logging verbosity on-the-fly
- **Custom command registration**: Add application-specific debugging commands
- **Multiple interfaces**: Both stdin and TCP access for local and remote debugging
- **Safe execution**: Command timeout protection and error handling
- **Graceful shutdown**: Controlled application shutdown via CLI

## Built-in Commands

### Core Application Commands

- **`help [command]`** - Show available commands or detailed help for a specific command
- **`status`** - Display application status including state, version, and thread counts
- **`threads [--detail]`** - List managed threads and their status
- **`config [--section <name>]`** - Show configuration values
- **`health`** - Run health checks on application components

### Messaging Commands

- **`messaging [--detail]`** - Show messaging system statistics
- **`log-level [level]`** - View or change the current log level
  - Available levels: trace, debug, info, warn, error, critical

### Control Commands

- **`shutdown`** - Initiate graceful application shutdown
- **`force-shutdown`** - Force immediate application shutdown
- **`exit`** - Exit CLI session (application continues running)

## Configuration

### Automatic Startup

Configure CLI in your `ApplicationConfig`:

```cpp
ApplicationConfig config{
    .name = "My App",
    .enable_cli = true,           // Enable CLI automatically
    .cli_enable_stdin = true,     // Enable stdin interface
    .cli_enable_tcp = true,       // Enable TCP interface
    .cli_bind_address = "127.0.0.1",
    .cli_port = 8080
};
```

### Manual Control

Enable CLI programmatically:

```cpp
CLIConfig cli_config{
    .enable = true,
    .bind_address = "127.0.0.1",
    .port = 8080,
    .enable_stdin = true,
    .enable_tcp_server = true
};

app.enable_cli(cli_config);
```

## Usage Examples

### Basic Interaction

```bash
# Via stdin (direct terminal input)
> status
Application Status
=================
Name: My App
Version: 1.0.0
State: Running
...

> threads
Thread Information
==================
Total Managed Threads: 3
...

> help
Available commands:
status          - Show application status
threads         - List and inspect managed threads
...
```

### Remote Access

```bash
# Via TCP (from another terminal or remote machine)
telnet localhost 8080

# Same commands available
> status
> threads --detail
> log-level debug
```

## Custom Commands

Register application-specific commands:

```cpp
// In your application class
void register_custom_commands() {
    auto& cli = this->cli();

    // Simple status command
    cli.register_command("task-count", "Show current task counter", "task-count",
        [this](const CLIContext& context) -> CLIResult {
            std::ostringstream oss;
            oss << "Task counter: " << task_counter_.load();
            return CLIResult::ok(oss.str());
        });

    // Command with arguments
    cli.register_command("worker", "Control worker thread", "worker [start|stop|status]",
        [this](const CLIContext& context) -> CLIResult {
            if (context.args.size() < 2) {
                return CLIResult::error("Usage: worker [start|stop|status]");
            }

            const std::string& action = context.args[1];
            if (action == "start") {
                start_worker();
                return CLIResult::ok("Worker started");
            }
            // ... handle other actions
        });
}
```

### Command Context

The `CLIContext` provides:

- **`args`**: Command arguments as a vector of strings
- **`options`**: Parsed command-line options (--key=value or --flag)
- **`app`**: Pointer to the Application instance
- **`has_option(name)`**: Check if option exists
- **`get_option(name, default)`**: Get option value with default

```cpp
// Example command using options
cli.register_command("inspect", "Inspect component", "inspect --component=<name> [--detail]",
    [this](const CLIContext& context) -> CLIResult {
        std::string component = context.get_option("component");
        bool detailed = context.has_option("detail");

        if (component.empty()) {
            return CLIResult::error("Component name required: --component=<name>");
        }

        // Perform inspection...
        return CLIResult::ok("Inspection results...");
    });
```

## Security Considerations

### TCP Interface

When enabling the TCP interface:

1. **Bind Address**: Use `127.0.0.1` for local-only access
2. **Firewall**: Ensure the CLI port is not exposed externally
3. **Authentication**: No built-in authentication - control access via network configuration
4. **Command Validation**: All commands have timeout protection and error handling

### Production Use

- Consider disabling TCP interface in production
- Use stdin-only interface for local debugging
- Implement custom commands with appropriate access controls
- Monitor CLI usage through application logs

## Integration Examples

### Complete Application Example

```cpp
class MyApp : public Application {
public:
    MyApp() : Application({
        .name = "Production App",
        .enable_cli = true,
        .cli_enable_stdin = true,
        .cli_enable_tcp = false  // Disabled for production
    }) {}

protected:
    bool on_initialize() override {
        register_custom_commands();
        return true;
    }

private:
    void register_custom_commands() {
        auto& cli = this->cli();

        // Application-specific commands
        cli.register_command("users", "Show user statistics", "users",
            [this](const CLIContext& ctx) { return show_user_stats(); });

        cli.register_command("cache", "Cache operations", "cache [clear|stats]",
            [this](const CLIContext& ctx) { return handle_cache_cmd(ctx); });
    }
};
```

### Development vs Production

```cpp
// Development configuration
ApplicationConfig dev_config{
    .enable_cli = true,
    .cli_enable_stdin = true,
    .cli_enable_tcp = true,
    .cli_port = 8080
};

// Production configuration
ApplicationConfig prod_config{
    .enable_cli = true,
    .cli_enable_stdin = true,
    .cli_enable_tcp = false  // No remote access
};
```

## Performance Impact

- **Minimal overhead** when CLI is disabled
- **Low impact** when enabled but not actively used
- **Command timeout protection** prevents CLI from blocking application
- **Asynchronous operation** - CLI runs in separate threads
- **Memory usage**: Small fixed overhead for command registry

## Troubleshooting

### CLI Not Starting

Check logs for:

```
CLI enabled successfully
CLI TCP server listening on 127.0.0.1:8080
```

Common issues:

- Port already in use
- Invalid bind address
- Insufficient permissions

### TCP Connection Issues

```bash
# Test TCP connectivity
telnet localhost 8080

# Check if port is listening
netstat -ln | grep 8080
lsof -i :8080
```

### Command Timeouts

Commands have a default 5-second timeout. Increase if needed:

```cpp
CLIConfig config{
    .command_timeout = std::chrono::milliseconds(10000)  // 10 seconds
};
```

## Best Practices

1. **Register commands early** in `on_initialize()`
2. **Use descriptive command names** and help text
3. **Validate command arguments** and provide clear error messages
4. **Return structured output** for complex data
5. **Consider security implications** for production deployments
6. **Test custom commands** thoroughly with various inputs
7. **Use appropriate log levels** for CLI operations
8. **Document custom commands** for your team

The CLI feature significantly enhances the debuggability and maintainability of Crux applications by providing real-time access to internal state and controls.
