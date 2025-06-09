#!/bin/bash

# Build script for SUMMA-Charm++
# This script provides an easy way to build and run the project

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
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

# Function to show help
show_help() {
    echo "SUMMA-Charm++ Build Script"
    echo ""
    echo "Usage: $0 [OPTIONS] [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  build     - Build the project (default)"
    echo "  clean     - Clean build artifacts"
    echo "  run       - Build and run the project"
    echo "  test      - Build and run basic tests"
    echo "  help      - Show this help message"
    echo ""
    echo "Options:"
    echo "  -d, --debug     - Build with debug flags"
    echo "  -j N            - Use N parallel jobs for building"
    echo "  -p N            - Use N processors for running (default: 4)"
    echo "  --clean-first   - Clean before building"
    echo ""
    echo "Examples:"
    echo "  $0                      # Build the project"
    echo "  $0 build --debug        # Debug build"
    echo "  $0 run -p 8             # Build and run with 8 processors"
    echo "  $0 clean                # Clean build artifacts"
    echo "  $0 test                 # Build and run tests"
}

# Default values
COMMAND="build"
DEBUG=0
JOBS=4
PROCS=4
CLEAN_FIRST=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        build|clean|run|test|help)
            COMMAND="$1"
            shift
            ;;
        -d|--debug)
            DEBUG=1
            shift
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        -p)
            PROCS="$2"
            shift 2
            ;;
        --clean-first)
            CLEAN_FIRST=1
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -d "src" ]]; then
    print_error "This script must be run from the SUMMA-Charm++ root directory"
    exit 1
fi

# Check for Charm++
if [[ ! -f "charm/bin/charmc" ]]; then
    print_error "Charm++ not found. Please ensure Charm++ is built in the 'charm' directory."
    exit 1
fi

print_status "SUMMA-Charm++ Build Script"
print_status "Command: $COMMAND"
if [[ $DEBUG -eq 1 ]]; then
    print_status "Build type: Debug"
else
    print_status "Build type: Release"
fi

case $COMMAND in
    help)
        show_help
        ;;
    
    clean)
        print_status "Cleaning build artifacts..."
        make clean
        print_success "Clean completed"
        ;;
    
    build)
        if [[ $CLEAN_FIRST -eq 1 ]]; then
            print_status "Cleaning before build..."
            make clean
        fi
        
        print_status "Building SUMMA-Charm++..."
        if [[ $DEBUG -eq 1 ]]; then
            make debug -j$JOBS
        else
            make release -j$JOBS
        fi
        print_success "Build completed successfully"
        ;;
    
    run)
        # Build first
        if [[ $CLEAN_FIRST -eq 1 ]]; then
            print_status "Cleaning before build..."
            make clean
        fi
        
        print_status "Building SUMMA-Charm++..."
        if [[ $DEBUG -eq 1 ]]; then
            make debug -j$JOBS
        else
            make release -j$JOBS
        fi
        
        print_status "Running SUMMA-Charm++ with $PROCS processors..."
        make run PROCS=$PROCS
        print_success "Run completed"
        ;;
    
    test)
        # Build first
        if [[ $CLEAN_FIRST -eq 1 ]]; then
            print_status "Cleaning before build..."
            make clean
        fi
        
        print_status "Building SUMMA-Charm++..."
        if [[ $DEBUG -eq 1 ]]; then
            make debug -j$JOBS
        else
            make release -j$JOBS
        fi
        
        print_status "Running basic tests..."
        make test
        print_success "Tests completed"
        ;;
    
    *)
        print_error "Unknown command: $COMMAND"
        show_help
        exit 1
        ;;
esac
