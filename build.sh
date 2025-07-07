#!/bin/bash

##
# Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#
# SPDX-License-Identifier: MIT
##

# Build script wrapper for the base library
# Provides convenient commands for different build modes

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Base Library Build Script"
    echo ""
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  build           Build library only (default/release mode)"
    echo "  dev             Build with tests, benchmarks, and examples"
    echo "  test            Build with tests and run them"
    echo "  bench           Build with benchmarks and list them"
    echo "  examples        Build examples and list them"
    echo "  full            Build everything (tests, benchmarks, examples, docs)"
    echo "  create          Create Conan package (library only)"
    echo "  create-dev      Create Conan package with all development features"
    echo "  clean           Clean build directory"
    echo "  install         Install dependencies only"
    echo ""
    echo "Options:"
    echo "  --debug         Build in Debug mode (default: Release)"
    echo "  --shared        Build shared library (default: static)"
    echo "  --verbose       Verbose output"
    echo "  --help, -h      Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 build                    # Build library only"
    echo "  $0 dev                      # Build with dev features"
    echo "  $0 test                     # Build and run tests"
    echo "  $0 bench                    # Build benchmarks"
    echo "  $0 full --debug             # Build everything in debug mode"
    echo "  $0 create-dev               # Create development package"
}

# Parse command line arguments
COMMAND=""
BUILD_TYPE="Release"
LIBRARY_TYPE="static"
VERBOSE=""
EXTRA_ARGS=""

while [[ $# -gt 0 ]]; do
    case $1 in
        build|dev|test|bench|examples|full|create|create-dev|clean|install)
            COMMAND="$1"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --shared)
            LIBRARY_TYPE="shared"
            shift
            ;;
        --verbose)
            VERBOSE="-v"
            shift
            ;;
        --help|-h)
            show_usage
            exit 0
            ;;
        *)
            EXTRA_ARGS="$EXTRA_ARGS $1"
            shift
            ;;
    esac
done

# Set default command if none provided
if [[ -z "$COMMAND" ]]; then
    COMMAND="build"
fi

# Build the conan command based on parameters
build_conan_cmd() {
    local cmd="conan"
    local options=""

    if [[ "$BUILD_TYPE" == "Debug" ]]; then
        options="$options -s build_type=Debug"
    fi

    if [[ "$LIBRARY_TYPE" == "shared" ]]; then
        options="$options -o \"&:shared=True\""
    fi

    case $1 in
        "build"|"install")
            echo "$cmd $1 . --build=missing $options $VERBOSE $EXTRA_ARGS"
            ;;
        "dev")
            echo "$cmd build . --build=missing $options -o \"&:with_tests=True\" -o \"&:with_benchmarks=True\" -o \"&:with_examples=True\" $VERBOSE $EXTRA_ARGS"
            ;;
        "test")
            echo "$cmd build . --build=missing $options -o \"&:with_tests=True\" $VERBOSE $EXTRA_ARGS"
            ;;
        "bench")
            echo "$cmd build . --build=missing $options -o \"&:with_benchmarks=True\" $VERBOSE $EXTRA_ARGS"
            ;;
        "examples")
            echo "$cmd build . --build=missing $options -o \"&:with_examples=True\" $VERBOSE $EXTRA_ARGS"
            ;;
        "full")
            echo "$cmd build . --build=missing $options -o \"&:with_tests=True\" -o \"&:with_benchmarks=True\" -o \"&:with_examples=True\" -o \"&:with_docs=True\" $VERBOSE $EXTRA_ARGS"
            ;;
        "create")
            echo "$cmd create . --build=missing $options $VERBOSE $EXTRA_ARGS"
            ;;
        "create-dev")
            echo "$cmd create . --build=missing $options -o \"&:with_tests=True\" -o \"&:with_benchmarks=True\" -o \"&:with_examples=True\" -o \"&:with_docs=True\" $VERBOSE $EXTRA_ARGS"
            ;;
    esac
}

# Execute commands
case $COMMAND in
    clean)
        print_info "Cleaning build directory..."
        rm -rf build
        print_success "Build directory cleaned"
        ;;

    install)
        print_info "Installing dependencies..."
        eval "$(build_conan_cmd install)"
        print_success "Dependencies installed"
        ;;

    build)
        print_info "Building library (release mode)..."
        eval "$(build_conan_cmd build)"
        print_success "Library built successfully"
        ;;

    dev)
        print_info "Building in development mode (tests + benchmarks + examples)..."
        eval "$(build_conan_cmd dev)"
        print_success "Development build completed"
        print_info "Available executables:"
        echo "  Tests:      ./build/$BUILD_TYPE/tests/test_base"
        echo "  Examples:   ./build/$BUILD_TYPE/examples/"
        echo "  Benchmarks: ./build/$BUILD_TYPE/benchmarks/"
        ;;

    test)
        print_info "Building with tests..."
        eval "$(build_conan_cmd test)"
        print_success "Build with tests completed"

        if [[ -f "./build/$BUILD_TYPE/tests/test_base" ]]; then
            print_info "Running tests..."
            ./build/$BUILD_TYPE/tests/test_base
            print_success "Tests completed"
        else
            print_error "Test executable not found"
            exit 1
        fi
        ;;

    bench)
        print_info "Building benchmarks..."
        eval "$(build_conan_cmd bench)"
        print_success "Benchmarks built successfully"

        print_info "Available benchmarks:"
        if [[ -d "./build/$BUILD_TYPE/benchmarks" ]]; then
            for file in ./build/$BUILD_TYPE/benchmarks/*; do
                if [[ -x "$file" && -f "$file" ]]; then
                    echo "  $(basename "$file")"
                fi
            done
        fi
        ;;

    examples)
        print_info "Building examples..."
        eval "$(build_conan_cmd examples)"
        print_success "Examples built successfully"

        print_info "Available examples:"
        if [[ -d "./build/$BUILD_TYPE/examples" ]]; then
            for file in ./build/$BUILD_TYPE/examples/*; do
                if [[ -x "$file" && -f "$file" ]]; then
                    echo "  $(basename "$file")"
                fi
            done
        fi
        ;;

    full)
        print_info "Building everything (full development mode)..."
        eval "$(build_conan_cmd full)"
        print_success "Full build completed"

        print_info "Build summary:"
        echo "  Library:    ./build/$BUILD_TYPE/libbase.a"
        echo "  Tests:      ./build/$BUILD_TYPE/tests/test_base"
        echo "  Examples:   ./build/$BUILD_TYPE/examples/"
        echo "  Benchmarks: ./build/$BUILD_TYPE/benchmarks/"
        ;;

    create)
        print_info "Creating Conan package (library only)..."
        eval "$(build_conan_cmd create)"
        print_success "Conan package created successfully"
        ;;

    create-dev)
        print_info "Creating Conan package with all development features..."
        eval "$(build_conan_cmd create-dev)"
        print_success "Development Conan package created successfully"
        ;;

    *)
        print_error "Unknown command: $COMMAND"
        show_usage
        exit 1
        ;;
esac
