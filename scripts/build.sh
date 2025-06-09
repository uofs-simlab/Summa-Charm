#!/bin/bash
# Build script for summa-charm project

set -e

# Configuration
BUILD_TYPE=${1:-Release}
BUILD_DIR="build"
SUMMA_ROOT="/u1/pma753/Summa-Actors/build/summa"

echo "Building summa-charm in ${BUILD_TYPE} mode..."

# Create build directory
if [ ! -d "${BUILD_DIR}" ]; then
    mkdir -p "${BUILD_DIR}"
fi

cd "${BUILD_DIR}"

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DUSE_EXTERNAL_SUMMA=ON \
    -DSUMMA_ROOT=${SUMMA_ROOT} \
    -DCHARM_ROOT="../charm" \
    -DBUILD_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF

# Build
make -j$(nproc)

echo "Build completed successfully!"
echo "Executable: ${BUILD_DIR}/bin/summa-charm"
