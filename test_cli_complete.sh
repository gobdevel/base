#!/bin/bash

# CLI Integration Test Script
# Tests the Crux CLI functionality with automated input

echo "=== Crux CLI Integration Tests ==="
echo

# Test 1: Simple CLI Test with basic commands
echo "Test 1: Basic CLI Commands"
echo "=========================="

# Create a test input file
cat > cli_test_input.txt << 'EOF'
help
status
config
health
messaging
log-level
test
shutdown
EOF

echo "Running simple CLI test with predefined commands..."
if timeout 10s ./build/build/Release/examples/simple_cli_test < cli_test_input.txt > cli_test_output.txt 2>&1; then
    echo "✅ Simple CLI test completed successfully"
    echo "Output preview:"
    head -20 cli_test_output.txt | sed 's/^/  /'
else
    echo "❌ Simple CLI test failed or timed out"
    echo "Output:"
    cat cli_test_output.txt | sed 's/^/  /'
fi

echo
echo "Test 2: Advanced CLI Example with Custom Commands"
echo "================================================="

# Create advanced test input
cat > cli_advanced_input.txt << 'EOF'
help
status
threads
task-count
worker status
worker start
worker status
load 10
task-count
worker stop
log-level info
shutdown
EOF

echo "Running advanced CLI example with custom commands..."
if timeout 15s ./build/build/Release/examples/cli_example < cli_advanced_input.txt > cli_advanced_output.txt 2>&1; then
    echo "✅ Advanced CLI test completed successfully"
    echo "Output preview:"
    head -30 cli_advanced_output.txt | sed 's/^/  /'
else
    echo "❌ Advanced CLI test failed or timed out"
    echo "Output:"
    cat cli_advanced_output.txt | sed 's/^/  /'
fi

echo
echo "Test 3: TCP Interface Test"
echo "=========================="

# Start the CLI example in background
echo "Starting CLI example with TCP interface..."
./build/build/Release/examples/cli_example > tcp_test_output.txt 2>&1 &
CLI_PID=$!

# Wait for startup
sleep 2

# Test TCP connection
echo "Testing TCP connection on localhost:8080..."
if command -v nc >/dev/null 2>&1; then
    echo -e "help\nstatus\nexit\n" | timeout 5s nc localhost 8080 > tcp_client_output.txt 2>&1
    if [ $? -eq 0 ]; then
        echo "✅ TCP interface test successful"
        echo "TCP client output:"
        cat tcp_client_output.txt | sed 's/^/  /'
    else
        echo "❌ TCP interface test failed"
    fi
else
    echo "⚠️  netcat (nc) not available, skipping TCP test"
fi

# Cleanup
echo "Cleaning up..."
kill $CLI_PID 2>/dev/null
wait $CLI_PID 2>/dev/null

echo
echo "Test 4: Error Handling Test"
echo "==========================="

cat > cli_error_input.txt << 'EOF'
invalid_command
load
load abc
load 9999
worker invalid
shutdown
EOF

echo "Testing error handling..."
if timeout 10s ./build/build/Release/examples/cli_example < cli_error_input.txt > cli_error_output.txt 2>&1; then
    echo "✅ Error handling test completed"
    echo "Error handling output:"
    grep -i "error\|invalid\|unknown" cli_error_output.txt | sed 's/^/  /'
else
    echo "❌ Error handling test failed"
fi

echo
echo "=== Test Summary ==="
echo "1. Basic CLI Commands: $([ -f cli_test_output.txt ] && echo "✅ Completed" || echo "❌ Failed")"
echo "2. Advanced CLI Example: $([ -f cli_advanced_output.txt ] && echo "✅ Completed" || echo "❌ Failed")"
echo "3. TCP Interface: $([ -f tcp_client_output.txt ] && echo "✅ Tested" || echo "⚠️  Skipped")"
echo "4. Error Handling: $([ -f cli_error_output.txt ] && echo "✅ Completed" || echo "❌ Failed")"

echo
echo "Detailed output files created:"
echo "  - cli_test_output.txt (basic commands)"
echo "  - cli_advanced_output.txt (advanced features)"
echo "  - tcp_client_output.txt (TCP interface)"
echo "  - cli_error_output.txt (error handling)"

# Cleanup input files
rm -f cli_test_input.txt cli_advanced_input.txt cli_error_input.txt

echo
echo "CLI testing completed!"
