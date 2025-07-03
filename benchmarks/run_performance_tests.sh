#!/bin/bash

# Base Framework Performance Test Runner
# This script automates performance testing and comparison

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/benchmarks"
RESULTS_DIR="$PROJECT_ROOT/benchmark_results"

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

# Function to check system requirements
check_requirements() {
    log_info "Checking system requirements..."

    # Check for required tools
    for tool in cmake make; do
        if ! command -v $tool &> /dev/null; then
            log_error "$tool is required but not installed"
            exit 1
        fi
    done

    # Check available memory (Linux/macOS)
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        available_mem=$(grep MemAvailable /proc/meminfo | awk '{print $2}')
        available_mem_gb=$((available_mem / 1024 / 1024))
        if [ $available_mem_gb -lt 2 ]; then
            log_warning "Low available memory: ${available_mem_gb}GB (recommended: 4GB+)"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        available_mem=$(vm_stat | grep "Pages free" | awk '{print $3}' | sed 's/\.//')
        available_mem_gb=$((available_mem * 4096 / 1024 / 1024 / 1024))
        if [ $available_mem_gb -lt 2 ]; then
            log_warning "Low available memory: ${available_mem_gb}GB (recommended: 4GB+)"
        fi
    fi

    log_success "System requirements check completed"
}

# Function to setup build environment
setup_build() {
    log_info "Setting up build environment..."

    # Create build directory
    mkdir -p "$BUILD_DIR"
    mkdir -p "$RESULTS_DIR"

    cd "$BUILD_DIR"

    # Check if Conan dependencies are available
    if [ ! -f "$PROJECT_ROOT/build/Release/generators/conandeps_legacy.cmake" ]; then
        log_warning "Conan dependencies not found. Running conan install..."
        cd "$PROJECT_ROOT"
        conan install . --build=missing -s build_type=Release
        cd "$BUILD_DIR"
    fi

    # Configure CMake
    log_info "Configuring CMake..."
    cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release

    # Build
    log_info "Building benchmark suite..."
    make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

    log_success "Build completed successfully"
}

# Function to run performance tests from the main test suite
run_existing_tests() {
    log_info "Running existing performance tests..."

    cd "$PROJECT_ROOT"
    if [ -f "build/Release/tests/test_base" ]; then
        ./build/Release/tests/test_base --gtest_filter="*Performance*" > "$RESULTS_DIR/existing_performance_tests.log" 2>&1
        log_success "Existing performance tests completed"
    else
        log_warning "Main test suite not found. Run 'cmake --build build/Release' first"
    fi
}

# Function to run benchmarks
run_benchmarks() {
    local benchmark_type="$1"
    local output_file
    output_file="$RESULTS_DIR/benchmark_${benchmark_type}_$(date +%Y%m%d_%H%M%S).log"

    log_info "Running $benchmark_type benchmarks..."

    cd "$BUILD_DIR"

    if [ "$benchmark_type" = "all" ]; then
        ./benchmark_runner > "$output_file" 2>&1
    else
        ./benchmark_runner --$benchmark_type > "$output_file" 2>&1
    fi

    log_success "$benchmark_type benchmarks completed. Results saved to: $output_file"

    # Show summary
    if grep -q "BENCHMARK RESULTS SUMMARY" "$output_file"; then
        echo ""
        log_info "Quick Summary:"
        sed -n '/BENCHMARK RESULTS SUMMARY/,/=====/p' "$output_file" | tail -n +3 | head -n -1
    fi
}

# Function to compare with baseline
compare_with_baseline() {
    local current_results="$1"
    local baseline_file="$RESULTS_DIR/baseline_results.csv"

    if [ -f "$baseline_file" ] && [ -f "$current_results" ]; then
        log_info "Comparing with baseline performance..."

        # Simple comparison (can be enhanced with more sophisticated analysis)
        python3 - << EOF
import csv
import sys

def compare_results(baseline_file, current_file):
    baseline = {}
    current = {}

    try:
        with open(baseline_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                baseline[row['benchmark']] = float(row['throughput_per_sec'])

        with open(current_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                current[row['benchmark']] = float(row['throughput_per_sec'])

        print("Performance Comparison:")
        print("-" * 60)
        for benchmark in current:
            if benchmark in baseline:
                current_perf = current[benchmark]
                baseline_perf = baseline[benchmark]
                change = ((current_perf - baseline_perf) / baseline_perf) * 100

                status = "📈" if change > 5 else "📉" if change < -5 else "➡️"
                print(f"{status} {benchmark:30} {change:+6.1f}%")
            else:
                print(f"🆕 {benchmark:30} (new benchmark)")

    except Exception as e:
        print(f"Error comparing results: {e}")

compare_results("$baseline_file", "$current_results")
EOF
    else
        log_warning "No baseline found for comparison"
    fi
}

# Function to create baseline
create_baseline() {
    log_info "Creating performance baseline..."

    cd "$BUILD_DIR"
    ./benchmark_runner

    if [ -f "benchmark_results.csv" ]; then
        cp "benchmark_results.csv" "$RESULTS_DIR/baseline_results.csv"
        log_success "Baseline created: $RESULTS_DIR/baseline_results.csv"
    else
        log_error "Failed to create baseline - benchmark_results.csv not found"
        exit 1
    fi
}

# Function to run system stress test
run_stress_test() {
    log_info "Running stress test..."

    cd "$BUILD_DIR"

    # Run messaging benchmarks multiple times to check for degradation
    for i in {1..5}; do
        log_info "Stress test iteration $i/5"
        ./benchmark_runner --messaging > "$RESULTS_DIR/stress_test_$i.log" 2>&1
        sleep 5
    done

    log_success "Stress test completed. Check results in $RESULTS_DIR/stress_test_*.log"
}

# Function to profile with perf (Linux only)
run_profiling() {
    if [[ "$OSTYPE" != "linux-gnu"* ]]; then
        log_warning "Profiling with perf is only available on Linux"
        return
    fi

    if ! command -v perf &> /dev/null; then
        log_warning "perf not found. Install with: sudo apt-get install linux-tools-generic"
        return
    fi

    log_info "Running profiling with perf..."

    cd "$BUILD_DIR"
    perf record -g ./benchmark_runner --messaging 2>/dev/null
    perf report > "$RESULTS_DIR/perf_report.txt" 2>/dev/null

    log_success "Profiling completed. Report saved to: $RESULTS_DIR/perf_report.txt"
}

# Main function
main() {
    local command="${1:-help}"

    case "$command" in
        "build")
            check_requirements
            setup_build
            ;;
        "test")
            run_existing_tests
            ;;
        "benchmark")
            local type="${2:-all}"
            check_requirements
            setup_build
            run_benchmarks "$type"
            ;;
        "baseline")
            check_requirements
            setup_build
            create_baseline
            ;;
        "compare")
            local results_file="${2:-$BUILD_DIR/benchmark_results.csv}"
            compare_with_baseline "$results_file"
            ;;
        "stress")
            check_requirements
            setup_build
            run_stress_test
            ;;
        "profile")
            check_requirements
            setup_build
            run_profiling
            ;;
        "full")
            check_requirements
            setup_build
            run_existing_tests
            run_benchmarks "all"
            compare_with_baseline "$BUILD_DIR/benchmark_results.csv"
            ;;
        "help"|*)
            echo "Base Framework Performance Test Runner"
            echo ""
            echo "Usage: $0 <command> [options]"
            echo ""
            echo "Commands:"
            echo "  build                 Build the benchmark suite"
            echo "  test                  Run existing performance tests"
            echo "  benchmark [type]      Run benchmarks (all|logger|messaging|config|threads|memory)"
            echo "  baseline              Create performance baseline"
            echo "  compare [file]        Compare results with baseline"
            echo "  stress                Run stress test"
            echo "  profile               Run with profiling (Linux only)"
            echo "  full                  Run complete performance test suite"
            echo "  help                  Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 benchmark messaging    # Run messaging benchmarks only"
            echo "  $0 full                   # Run complete test suite"
            echo "  $0 baseline               # Create new baseline"
            echo ""
            ;;
    esac
}

# Run main function with all arguments
main "$@"
