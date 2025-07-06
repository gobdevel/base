# Daemonization Feature

The Base application framework supports UNIX daemon process creation with comprehensive configuration options for robust service deployment.

## Overview

Daemonization transforms your application into a background service that:

- Runs independently of the terminal session
- Continues running after user logout
- Has no controlling terminal
- Follows UNIX daemon best practices

## Configuration

Configure daemonization through `ApplicationConfig`:

```cpp
ApplicationConfig config;
config.daemonize = true;                          // Enable daemonization
config.daemon_work_dir = "/var/lib/myapp";        // Working directory
config.daemon_pid_file = "/var/run/myapp.pid";    // PID file location
config.daemon_log_file = "/var/log/myapp.log";    // Log file for stdout/stderr
config.daemon_user = "myapp";                     // User to run as
config.daemon_group = "myapp";                    // Group to run as
config.daemon_umask = 022;                        // File creation mask
config.daemon_close_fds = true;                   // Close all file descriptors
```

## Configuration Options

### Basic Options

- `daemonize`: Enable/disable daemonization (default: false)
- `daemon_work_dir`: Working directory for daemon (default: "/")
- `daemon_umask`: File creation mask (default: 022)
- `daemon_close_fds`: Close all file descriptors when daemonizing (default: true)

### Security Options

- `daemon_user`: Username to drop privileges to (empty = current user)
- `daemon_group`: Group name to drop privileges to (empty = current group)

### Logging Options

- `daemon_pid_file`: PID file path (empty = no PID file)
- `daemon_log_file`: Log file for stdout/stderr redirection (empty = /dev/null)

## Usage Example

```cpp
#include "application.h"

class MyDaemon : public Application {
public:
    MyDaemon() : Application(create_config()) {}

private:
    ApplicationConfig create_config() {
        ApplicationConfig config;
        config.name = "mydaemon";
        config.daemonize = true;
        config.daemon_work_dir = "/var/lib/mydaemon";
        config.daemon_pid_file = "/var/run/mydaemon.pid";
        config.daemon_log_file = "/var/log/mydaemon.log";
        config.daemon_user = "mydaemon";
        config.daemon_group = "mydaemon";
        return config;
    }
};

int main() {
    MyDaemon daemon;
    return daemon.run();
}
```

## Daemonization Process

The framework follows the standard UNIX double-fork process:

1. **First Fork**: Creates child process and exits parent
2. **Session Leader**: Child becomes session leader with `setsid()`
3. **Second Fork**: Creates grandchild and exits first child
4. **Change Directory**: Changes to specified working directory
5. **Set Umask**: Sets file creation permissions mask
6. **Close FDs**: Closes all open file descriptors
7. **Redirect Streams**: Redirects stdin/stdout/stderr
8. **Drop Privileges**: Changes user/group if specified
9. **Create PID File**: Writes process ID to file

## Security Considerations

### User and Group

Running as a dedicated user improves security:

```cpp
config.daemon_user = "myapp";    // Non-privileged user
config.daemon_group = "myapp";   // Non-privileged group
```

### File Permissions

The umask controls default file permissions:

```cpp
config.daemon_umask = 022;  // Creates files with 644 permissions
```

### Working Directory

Use a dedicated directory with appropriate permissions:

```cpp
config.daemon_work_dir = "/var/lib/myapp";  // Application-specific directory
```

## Service Integration

### systemd Service

Create `/etc/systemd/system/myapp.service`:

```ini
[Unit]
Description=My Application Service
After=network.target

[Service]
Type=forking
ExecStart=/usr/local/bin/myapp
PIDFile=/var/run/myapp.pid
User=myapp
Group=myapp
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### SysV Init Script

Basic init script structure:

```bash
#!/bin/bash
# chkconfig: 35 80 20
# description: My Application Service

. /etc/rc.d/init.d/functions

USER="myapp"
DAEMON="myapp"
ROOT_DIR="/var/lib/myapp"

start() {
    if [ -f $ROOT_DIR/myapp.pid ]; then
        echo "$DAEMON is already running."
        exit 1
    fi
    echo -n "Starting $DAEMON: "
    runuser -l "$USER" -c "$ROOT_DIR/$DAEMON" && echo_success || echo_failure
    RETVAL=$?
    echo
}

stop() {
    echo -n "Shutting down $DAEMON: "
    pid=$(ps -aefw | grep "$DAEMON" | grep -v " grep " | awk '{print $2}')
    kill -9 $pid > /dev/null 2>&1
    [ $? -eq 0 ] && echo_success || echo_failure
    RETVAL=$?
    echo
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        start
        ;;
    *)
        echo "Usage: {start|stop|restart}"
        exit 1
        ;;
esac

exit $RETVAL
```

## Logging

When daemonized, the application's stdout and stderr are redirected:

- If `daemon_log_file` is set: redirected to that file
- If empty: redirected to `/dev/null`

The application's internal logger continues to work normally based on its configuration.

## PID File Management

The framework automatically:

- Creates the PID file on successful daemonization
- Removes the PID file on clean shutdown
- Handles cleanup in the destructor

## Error Handling

Daemonization errors are logged and cause application startup to fail:

```cpp
if (!app.run()) {
    // Check logs for daemonization errors
    return EXIT_FAILURE;
}
```

## Platform Support

- **UNIX/Linux**: Full support
- **Windows**: Daemonization disabled, logs error if requested
- **macOS**: Full support (same as Linux)

## Best Practices

1. **Test in Foreground First**: Test your application in foreground mode before daemonizing
2. **Proper Logging**: Configure logging to files when daemonizing
3. **Signal Handling**: The framework handles SIGTERM/SIGINT for graceful shutdown
4. **Resource Cleanup**: Ensure proper cleanup in component stop() methods
5. **Health Monitoring**: Use the built-in health check system
6. **Privilege Separation**: Run as dedicated non-root user when possible

## Troubleshooting

### Common Issues

1. **Permission Denied**: Check user/group permissions for directories and files
2. **PID File Errors**: Ensure PID file directory is writable by daemon user
3. **Log File Issues**: Verify log directory permissions and disk space
4. **Port Binding**: Ensure daemon user can bind to required ports

### Debugging

Run in foreground mode for debugging:

```cpp
config.daemonize = false;  // Disable daemonization for debugging
```

## Example Applications

See `examples/daemon_example.cpp` for a complete working example of a daemonized application.
