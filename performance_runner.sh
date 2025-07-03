#!/bin/bash

# Simple Performance Test Runner for Base Framework
# This script uses the existing test suite to run performance benchmarks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to run existing performance tests
run_existing_performance_tests() {
    log_info "Running existing performance tests from test suite..."

    cd "$PROJECT_ROOT"

    if [ ! -f "build/Release/tests/test_base" ]; then
        log_error "Test executable not found. Please build the project first:"
        echo "  cmake --build build/Release"
        exit 1
    fi

    echo ""
    echo "========================================"
    echo "BASE FRAMEWORK PERFORMANCE TESTS"
    echo "========================================"
    echo ""

    # Run performance-related tests
    log_info "Running messaging performance tests..."
    ./build/Release/tests/test_base --gtest_filter="*Performance*" || log_warning "Some performance tests may have failed"

    echo ""
    log_success "Performance tests completed"
}

# Function to run the simple benchmark
run_simple_benchmark() {
    log_info "Running simple benchmark suite..."

    cd "$PROJECT_ROOT"

    # Build if not exists
    if [ ! -f "build/Release/benchmarks/simple_benchmark" ]; then
        log_info "Building benchmark suite..."
        cmake --build build/Release || {
            log_error "Failed to build benchmarks"
            exit 1
        }
    fi

    if [ ! -f "build/Release/benchmarks/simple_benchmark" ]; then
        log_error "Simple benchmark not found after build"
        exit 1
    fi

    echo ""
    echo "========================================"
    echo "SIMPLE BENCHMARK SUITE"
    echo "========================================"
    echo ""

    ./build/Release/benchmarks/simple_benchmark

    echo ""
    log_success "Simple benchmark completed"
}

# Function to run messaging benchmarks multiple times
run_messaging_stress_test() {
    log_info "Running messaging stress test (5 iterations)..."

    cd "$PROJECT_ROOT"

    echo ""
    echo "========================================"
    echo "MESSAGING STRESS TEST"
    echo "========================================"
    echo ""

    for i in {1..5}; do
        log_info "Stress test iteration $i/5"
        ./build/Release/tests/test_base --gtest_filter="*MessagingPerformance*" --gtest_repeat=1
        echo ""
        sleep 2
    done

    log_success "Messaging stress test completed"
}

# Function to run all messaging examples
run_messaging_examples() {
    log_info "Running messaging examples for performance observation..."

    cd "$PROJECT_ROOT"

    echo ""
    echo "========================================"
    echo "MESSAGING EXAMPLES"
    echo "========================================"
    echo ""

    if [ -f "build/Release/examples/messaging_example" ]; then
        log_info "Running messaging example..."
        timeout 30s ./build/Release/examples/messaging_example || log_warning "Messaging example timed out (normal)"
        echo ""
    fi

    log_success "Messaging examples completed"
}

# Function to check system performance
check_system_performance() {
    log_info "Checking system performance characteristics..."

    echo ""
    echo "========================================"
    echo "SYSTEM PERFORMANCE INFO"
    echo "========================================"
    echo ""

    # CPU info
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "CPU Information:"
        lscpu | grep -E 'Model name|CPU\(s\)|Thread|Core|MHz'
        echo ""

        echo "Memory Information:"
        free -h
        echo ""

    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "CPU Information:"
        sysctl -n machdep.cpu.brand_string
        echo "CPU Cores: $(sysctl -n hw.ncpu)"
        echo "CPU Frequency: $(sysctl -n hw.cpufrequency 2>/dev/null || echo 'Unknown')"
        echo ""

        echo "Memory Information:"
        echo "Physical Memory: $(($(sysctl -n hw.memsize) / 1024 / 1024 / 1024))GB"
        vm_stat | head -6
        echo ""
    fi

    # Compiler info
    echo "Compiler Information:"
    if command -v clang++ &> /dev/null; then
        clang++ --version | head -1
    elif command -v g++ &> /dev/null; then
        g++ --version | head -1
    fi
    echo ""

    # Build type
    echo "Build Information:"
    if [ -f "$PROJECT_ROOT/build/Release/CMakeCache.txt" ]; then
        echo "Build Type: Release"
        echo "Optimizations: Enabled"
    elif [ -f "$PROJECT_ROOT/build/Debug/CMakeCache.txt" ]; then
        echo "Build Type: Debug"
        echo "⚠️  WARNING: Debug builds are much slower than Release builds"
    fi
    echo ""
}

# Function to measure build performance
measure_build_performance() {
    log_info "Measuring build performance..."

    cd "$PROJECT_ROOT"

    echo ""
    echo "========================================"
    echo "BUILD PERFORMANCE TEST"
    echo "========================================"
    echo ""

    # Clean build
    log_info "Performing clean build measurement..."

    if [ -d "build/Release" ]; then
        rm -rf build/Release
    fi

    start_time=$(date +%s.%N)

    conan install . --build=missing -s build_type=Release > /dev/null 2>&1
    cmake --preset conan-release > /dev/null 2>&1
    cmake --build build/Release > /dev/null 2>&1

    end_time=$(date +%s.%N)
    build_time=$(echo "$end_time - $start_time" | bc)

    log_success "Clean build completed in ${build_time}s"

    # Incremental build
    log_info "Measuring incremental build performance..."

    start_time=$(date +%s.%N)
    cmake --build build/Release > /dev/null 2>&1
    end_time=$(date +%s.%N)
    incremental_time=$(echo "$end_time - $start_time" | bc)

    log_success "Incremental build completed in ${incremental_time}s"
    echo ""
}

# Main menu function
show_menu() {
    echo "Base Framework Performance Testing"
    echo ""
    echo "Available tests:"
    echo "1. Quick performance tests (existing test suite)"
    echo "2. Simple benchmark suite"
    echo "3. Simple performance example"
    echo "4. Messaging stress test"
    echo "5. Messaging examples"
    echo "6. System performance info"
    echo "7. Build performance test"
    echo "8. Run all tests"
    echo "9. Exit"
    echo ""
    read -p "Select an option (1-9): " choice
    echo ""
}

# Main function
main() {
    if [ $# -eq 0 ]; then
        # Interactive mode
        while true; do
            show_menu

            case $choice in
                1)
                    run_existing_performance_tests
                    ;;
                2)
                    run_simple_benchmark
                    ;;
                3)
                    run_simple_performance_example
                    ;;
                4)
                    run_messaging_stress_test
                    ;;
                5)
                    run_messaging_examples
                    ;;
                6)
                    check_system_performance
                    ;;
                7)
                    measure_build_performance
                    ;;
                8)
                    check_system_performance
                    run_existing_performance_tests
                    run_simple_benchmark
                    run_simple_performance_example
                    run_messaging_stress_test
                    ;;
                9)
                    log_info "Exiting performance test runner"
                    exit 0
                    ;;
                *)
                    log_error "Invalid option. Please select 1-9."
                    ;;
            esac

            echo ""
            read -p "Press Enter to continue..."
            echo ""
        done
    else
        # Command line mode
        case "$1" in
            "quick"|"q")
                run_existing_performance_tests
                ;;
            "simple"|"s")
                run_simple_benchmark
                ;;
            "example"|"e")
                run_simple_performance_example
                ;;
            "stress"|"st")
                run_messaging_stress_test
                ;;
            "messaging"|"msg")
                run_messaging_examples
                ;;
            "system"|"sys")
                check_system_performance
                ;;
            "build"|"b")
                measure_build_performance
                ;;
            "all"|"a")
                check_system_performance
                run_existing_performance_tests
                run_simple_benchmark
                run_simple_performance_example
                run_messaging_stress_test
                ;;
            "help"|"h"|*)
                echo "Base Framework Performance Test Runner"
                echo ""
                echo "Usage: $0 [command]"
                echo ""
                echo "Commands:"
                echo "  quick, q      Run existing performance tests"
                echo "  simple, s     Run simple benchmark suite"
                echo "  example, e    Run simple performance example"
                echo "  stress, st    Run messaging stress test"
                echo "  messaging, msg Run messaging examples"
                echo "  system, sys   Show system performance info"
                echo "  build, b      Measure build performance"
                echo "  all, a        Run all tests"
                echo "  help, h       Show this help"
                echo ""
                echo "Run without arguments for interactive mode."
                echo ""
                ;;
        esac
    fi
}

# Run main function with all arguments
main "$@"
