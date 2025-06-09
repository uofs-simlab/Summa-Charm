#!/bin/bash
# Simple validation script to test each component

set -e

echo "=== SUMMA-Charm Component Validation ==="

# Test 1: Check if all source files are in place
echo "Test 1: Checking source file organization..."

check_file() {
    if [ -f "$1" ]; then
        echo "  ✅ $1"
    else
        echo "  ❌ $1 (missing)"
    fi
}

check_file "src/core/main.cpp"
check_file "src/core/settings_functions.cpp"
check_file "src/charm/summa_char.cpp"
check_file "src/charm/main.ci"
check_file "src/charm/SummaChare.ci"
check_file "include/core/settings_functions.hpp"
check_file "include/charm/summa_char.hpp"

# Test 2: Check CMake files
echo "Test 2: Checking CMake configuration..."
check_file "CMakeLists.txt"
check_file "src/CMakeLists.txt"
check_file "src/core/CMakeLists.txt"
check_file "src/charm/CMakeLists.txt"
check_file "cmake/FindCharm.cmake"
check_file "cmake/FindNetCDF.cmake"

# Test 3: Check if we can compile individual components
echo "Test 3: Testing individual component compilation..."

# Create a temporary test build
TEST_BUILD_DIR="test_validation"
if [ -d "$TEST_BUILD_DIR" ]; then
    rm -rf "$TEST_BUILD_DIR"
fi

mkdir "$TEST_BUILD_DIR"
cd "$TEST_BUILD_DIR"

echo "  Testing basic C++ compilation..."
if g++ -std=c++17 -I../include/core -I../include/utils -c ../src/core/settings_functions.cpp -o settings_functions.o 2>/dev/null; then
    echo "  ✅ C++ core compilation works"
else
    echo "  ❌ C++ core compilation failed"
fi

echo "  Testing Charm++ interface compilation..."
if ../charm/bin/charmc ../src/charm/main.ci 2>/dev/null; then
    echo "  ✅ Charm++ interface compilation works"
else
    echo "  ❌ Charm++ interface compilation failed"
fi

cd ..
rm -rf "$TEST_BUILD_DIR"

# Test 4: Check dependencies
echo "Test 4: Checking system dependencies..."

check_command() {
    if command -v "$1" &> /dev/null; then
        echo "  ✅ $1 available"
    else
        echo "  ❌ $1 not found"
    fi
}

check_command "cmake"
check_command "g++"
check_command "gfortran"
check_command "pkg-config"

# Test 5: Check library dependencies
echo "Test 5: Checking library dependencies..."

check_lib() {
    if pkg-config --exists "$1" 2>/dev/null; then
        echo "  ✅ $1 available"
    else
        echo "  ❌ $1 not found"
    fi
}

check_lib "netcdf"

echo "=== Validation Complete ==="
echo ""
echo "If most tests pass, the project structure is correct."
echo "To build: ./scripts/build_full.sh"
