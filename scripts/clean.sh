#!/bin/bash
# Clean build script - removes all build artifacts

set -e

echo "Cleaning summa-charm build artifacts..."

# Remove build directory
if [ -d "build" ]; then
    echo "Removing build directory..."
    rm -rf build
fi

# Remove object files
echo "Removing object files..."
find . -name "*.o" -type f -delete 2>/dev/null || true
find . -name "*.mod" -type f -delete 2>/dev/null || true
find . -name "*.smod" -type f -delete 2>/dev/null || true

# Remove Charm++ generated files
echo "Removing Charm++ generated files..."
find . -name "*.decl.h" -type f -delete 2>/dev/null || true
find . -name "*.def.h" -type f -delete 2>/dev/null || true

# Remove output files
echo "Removing output files..."
find . -name "out.txt" -type f -delete 2>/dev/null || true
find . -name "*.log" -type f -delete 2>/dev/null || true

# Remove CMake cache
echo "Removing CMake cache files..."
find . -name "CMakeCache.txt" -type f -delete 2>/dev/null || true
find . -name "cmake_install.cmake" -type f -delete 2>/dev/null || true
find . -name "Makefile" -path "*/CMakeFiles/*" -delete 2>/dev/null || true

echo "Clean completed successfully!"
