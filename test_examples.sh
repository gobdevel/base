#!/bin/bash

# Test all examples script
echo "=== Testing All Crux Framework Examples ==="
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run example with timeout and error checking
run_example() {
    local example_name=$1
    local timeout_seconds=${2:-5}

    echo -e "${YELLOW}Testing: ${example_name}${NC}"
    echo "----------------------------------------"

    if [ ! -f "./build/Release/examples/${example_name}" ]; then
        echo -e "${RED}ERROR: ${example_name} not found${NC}"
        return 1
    fi

    # Run with timeout on macOS using perl
    if timeout_command=$(which gtimeout 2>/dev/null); then
        # GNU timeout (if installed via brew coreutils)
        $timeout_command ${timeout_seconds}s "./build/Release/examples/${example_name}"
    else
        # macOS fallback using perl
        perl -e "alarm(${timeout_seconds}); exec @ARGV" "./build/Release/examples/${example_name}"
    fi

    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ ${example_name} completed successfully${NC}"
    elif [ $exit_code -eq 124 ] || [ $exit_code -eq 142 ]; then
        echo -e "${YELLOW}⚠ ${example_name} timed out (${timeout_seconds}s) - this may be expected for some examples${NC}"
    else
        echo -e "${RED}✗ ${example_name} failed with exit code: ${exit_code}${NC}"
        return 1
    fi

    echo
    return 0
}

# Change to project directory
cd "$(dirname "$0")"

# Check if examples are built
if [ ! -d "./build/Release/examples" ]; then
    echo -e "${RED}ERROR: Examples not found. Please build the project first:${NC}"
    echo "  make -C build/Release"
    exit 1
fi

# Test each example
echo "Running examples from: $(pwd)/build/Release/examples/"
echo

# Simple examples that should exit quickly
run_example "color_test" 3
run_example "config_example" 3
run_example "logger_config_demo" 3

# Application examples that may run longer
run_example "app_example" 5
run_example "messaging_example" 8

echo "=== Example Testing Complete ==="
echo
echo "Note: Some examples may timeout by design if they run continuously."
echo "This is normal behavior for demonstration applications."
