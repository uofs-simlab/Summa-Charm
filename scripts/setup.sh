#!/bin/bash
# Configuration script for development environment

set -e

echo "Setting up summa-charm development environment..."

# Check for required tools
echo "Checking for required tools..."

if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake is required but not found"
    exit 1
fi

if ! command -v gfortran &> /dev/null; then
    echo "ERROR: gfortran is required but not found"  
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ is required but not found"
    exit 1
fi

# Check for NetCDF
if ! pkg-config --exists netcdf; then
    echo "WARNING: NetCDF not found via pkg-config"
    echo "Please ensure NetCDF is installed and in your PATH"
fi

# Check for LAPACK
if ! pkg-config --exists lapack; then
    echo "WARNING: LAPACK not found via pkg-config"
    echo "LAPACK will be searched using find_package"
fi

# Check Charm++ installation
CHARM_DIR="./charm"
if [ ! -d "${CHARM_DIR}" ]; then
    echo "ERROR: Charm++ not found at ${CHARM_DIR}"
    echo "Please ensure Charm++ is built and available"
    exit 1
fi

if [ ! -f "${CHARM_DIR}/bin/charmc" ]; then
    echo "ERROR: charmc not found in ${CHARM_DIR}/bin/"
    echo "Please build Charm++ first"
    exit 1
fi

# Check SUMMA installation
SUMMA_ROOT="/u1/pma753/Summa-Actors/build/summa"
if [ ! -d "${SUMMA_ROOT}" ]; then
    echo "ERROR: SUMMA not found at ${SUMMA_ROOT}"
    echo "Please check the SUMMA installation path"
    exit 1
fi

echo "Environment check completed successfully!"
echo ""
echo "Ready to build. Run: ./scripts/build.sh [Debug|Release]"
