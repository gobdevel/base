#!/bin/bash

# Simple CLI test script
echo "Testing Base CLI functionality..."

# Start the simple CLI test with timeout and input
{
    echo "help"
    sleep 1
    echo "test"
    sleep 1
    echo "status"
    sleep 1
    echo "shutdown"
} | timeout 10s ./build/build/Release/examples/simple_cli_test

echo "CLI test completed."
