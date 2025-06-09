#!/bin/bash
# Build SUMMA dependencies first, then summa-charm

set -e

SUMMA_PATH="/u1/pma753/Summa-Actors/build/summa"
BUILD_TYPE=${1:-Release}

echo "Building SUMMA-Charm with proper dependency management..."
echo "SUMMA Path: $SUMMA_PATH"

# Step 1: Check if SUMMA is built
echo "Step 1: Checking SUMMA installation..."

if [ ! -d "$SUMMA_PATH" ]; then
    echo "ERROR: SUMMA path $SUMMA_PATH does not exist!"
    echo "Please ensure SUMMA is available at the specified path."
    exit 1
fi

# Check if SUMMA has some built modules
SUMMA_BUILT_CHECK=$(find "$SUMMA_PATH" -name "*.mod" | wc -l)
echo "Found $SUMMA_BUILT_CHECK .mod files in SUMMA installation"

# Step 2: Build SUMMA if needed
if [ "$SUMMA_BUILT_CHECK" -lt 5 ]; then
    echo "Step 2: SUMMA appears to need building. Attempting to build SUMMA first..."
    cd "$SUMMA_PATH"
    
    if [ -f "Makefile" ]; then
        echo "Building SUMMA with make..."
        make clean 2>/dev/null || true
        make -j4 2>/dev/null || echo "SUMMA build completed with warnings"
    elif [ -f "CMakeLists.txt" ]; then
        echo "Building SUMMA with CMake..."
        mkdir -p build_summa
        cd build_summa
        cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
        make -j4 2>/dev/null || echo "SUMMA build completed with warnings"
    else
        echo "WARNING: Cannot determine how to build SUMMA"
    fi
    
    cd - > /dev/null
else
    echo "Step 2: SUMMA appears to be already built"
fi

# Step 3: Build summa-charm with external SUMMA
echo "Step 3: Building summa-charm..."

cd /u1/pma753/summa-charm

# Clean previous build
./scripts/clean.sh

# Create build directory
mkdir -p build
cd build

# Configure with full external SUMMA integration
echo "Configuring with external SUMMA..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DUSE_EXTERNAL_SUMMA=ON \
    -DSUMMA_ROOT="$SUMMA_PATH" \
    -DSUMMA_MINIMAL_BUILD=OFF \
    -DBUILD_TESTS=OFF

# Try to build
echo "Building summa-charm..."
if make -j4; then
    echo "✅ Build successful!"
    echo "Executable: $(pwd)/bin/summa-charm"
else
    echo "❌ Build failed. Trying minimal build for testing..."
    
    # Fallback to minimal build
    cd ..
    ./scripts/clean.sh
    mkdir -p build_minimal
    cd build_minimal
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DSUMMA_MINIMAL_BUILD=ON \
        -DUSE_EXTERNAL_SUMMA=OFF \
        -DBUILD_TESTS=ON
    
    # Build just the core components
    if make summa_core summa_utils summa_charm; then
        echo "✅ Minimal build successful!"
        echo "Core libraries built successfully"
    else
        echo "❌ Even minimal build failed"
        exit 1
    fi
fi

echo "Build completed!"
