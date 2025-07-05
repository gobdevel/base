#!/bin/bash

# Table Benchmark Performance Runner
# High-scale performance testing for Table data structure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/Release"
RESULTS_DIR="$BUILD_DIR/benchmark_results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}TABLE HIGH-SCALE PERFORMANCE BENCHMARK RUNNER${NC}"
echo -e "${BLUE}================================================${NC}\n"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found at $BUILD_DIR${NC}"
    echo -e "${YELLOW}Please run 'cmake --build build/Release' first${NC}"
    exit 1
fi

# Check if table benchmark exists
if [ ! -f "$BUILD_DIR/benchmarks/table_benchmark" ]; then
    echo -e "${YELLOW}Table benchmark not found. Building...${NC}"
    cd "$PROJECT_ROOT"
    cmake --build build/Release --target table_benchmark

    if [ ! -f "$BUILD_DIR/benchmarks/table_benchmark" ]; then
        echo -e "${RED}Error: Failed to build table benchmark${NC}"
        exit 1
    fi
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

# Function to run benchmark with logging
run_benchmark() {
    local name="$1"
    local args="$2"
    local output_file="$RESULTS_DIR/table_benchmark_$(date +%Y%m%d_%H%M%S).log"

    echo -e "${GREEN}Running: $name${NC}"
    echo -e "${YELLOW}Output will be saved to: $output_file${NC}\n"

    # Run benchmark with both console output and file logging
    cd "$BUILD_DIR"
    ./benchmarks/table_benchmark $args 2>&1 | tee "$output_file"

    local exit_code=${PIPESTATUS[0]}

    if [ $exit_code -eq 0 ]; then
        echo -e "\n${GREEN}✓ $name completed successfully${NC}"
        echo -e "${BLUE}Results saved to: $output_file${NC}\n"
    else
        echo -e "\n${RED}✗ $name failed with exit code $exit_code${NC}"
        return $exit_code
    fi
}

# Function to check system resources
check_system() {
    echo -e "${BLUE}System Information:${NC}"
    echo "Date: $(date)"
    echo "Host: $(hostname)"
    echo "OS: $(uname -s) $(uname -r)"
    echo "CPU: $(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo 'Unknown')"
    echo "CPU Cores: $(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 'Unknown')"
    echo "Memory: $(( $(sysctl -n hw.memsize 2>/dev/null || echo 0) / 1024 / 1024 / 1024 ))GB"
    echo ""
}

# Function to show disk space
check_disk_space() {
    echo -e "${BLUE}Disk Space Check:${NC}"
    local available=$(df -h "$BUILD_DIR" | awk 'NR==2 {print $4}')
    echo "Available space in build directory: $available"
    echo ""
}

# Main execution
main() {
    check_system
    check_disk_space

    echo -e "${YELLOW}WARNING: This benchmark will test very large datasets (up to 10M+ rows)${NC}"
    echo -e "${YELLOW}It may take significant time and memory. Ensure you have:${NC}"
    echo -e "${YELLOW}  - At least 8GB free RAM${NC}"
    echo -e "${YELLOW}  - At least 2GB free disk space${NC}"
    echo -e "${YELLOW}  - 30+ minutes available${NC}\n"

    if [ "$1" = "--auto" ]; then
        echo -e "${GREEN}Running in automatic mode...${NC}\n"
    else
        echo -e "${BLUE}Press Enter to continue or Ctrl+C to cancel...${NC}"
        read -r
    fi

    # Run the comprehensive table benchmark
    run_benchmark "Table High-Scale Performance Benchmark" ""

    echo -e "${GREEN}================================================${NC}"
    echo -e "${GREEN}TABLE BENCHMARK SUITE COMPLETED${NC}"
    echo -e "${GREEN}================================================${NC}\n"

    echo -e "${BLUE}Results directory: $RESULTS_DIR${NC}"
    echo -e "${BLUE}Latest results:${NC}"
    ls -la "$RESULTS_DIR"/*.log 2>/dev/null | tail -5 || echo "No log files found"

    echo -e "\n${YELLOW}To run again: $0${NC}"
    echo -e "${YELLOW}To run automatically: $0 --auto${NC}"
}

# Handle command line arguments
case "${1:-}" in
    --help|-h)
        echo "Table Benchmark Performance Runner"
        echo ""
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h    Show this help message"
        echo "  --auto        Run without user confirmation"
        echo "  (no args)     Run interactive mode"
        echo ""
        echo "This script runs comprehensive high-scale performance tests on the Table system."
        echo "Results are saved to the build directory with timestamps."
        exit 0
        ;;
    *)
        main "$@"
        ;;
esac
