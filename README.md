# SUMMA-Charm++

A Charm++ parallel implementation of the SUMMA hydrological model.

## Overview

This project provides a parallel implementation of the Structure for Unifying Multiple Modeling Alternatives (SUMMA) using the Charm++ runtime system. It's designed to leverage distributed memory parallelism for large-scale hydrological simulations.

## Project Structure

```
summa-charm/
├── CMakeLists.txt          # Main CMake configuration
├── README.md               # This file
├── build/                  # Build artifacts (generated)
├── cmake/                  # CMake modules
│   ├── FindCharm.cmake     # Charm++ detection
│   └── FindNetCDF.cmake    # NetCDF detection
├── external/               # External dependencies and examples
├── include/                # Header files
│   ├── charm/              # Charm++ specific headers
│   ├── core/               # Core functionality headers
│   ├── fortran/            # Fortran interface headers
│   └── utils/              # Utility headers
├── scripts/                # Build and configuration scripts
│   ├── build.sh            # Main build script
│   ├── setup.sh            # Environment setup
│   └── config.json         # Configuration files
├── src/                    # Source code
│   ├── charm/              # Charm++ specific code
│   ├── core/               # Core functionality
│   ├── fortran/            # Fortran interface code
│   └── utils/              # Utilities
├── summa/                  # SUMMA submodule
└── tests/                  # Test programs
```

## Dependencies

### Required
- **CMake** (>= 3.15)
- **Charm++** (built and available)
- **GNU Fortran** (gfortran)
- **NetCDF** (C and Fortran libraries)
- **LAPACK**
- **C++17** compatible compiler

### Optional
- **SUNDIALS** (for advanced solvers)
- **TBB** (Threading Building Blocks)

## Quick Start

### 1. Setup Environment
```bash
./scripts/setup.sh
```

### 2. Build the Project
```bash
./scripts/build.sh Release
```

### 3. Run the Application
```bash
cd build
./bin/summa-charm -c ../scripts/config.json -m ../scripts/fileManager.txt
```

## Build Options

The project supports several CMake options:

- `USE_EXTERNAL_SUMMA=ON/OFF` - Use external SUMMA installation (default: ON)
- `BUILD_TESTS=ON/OFF` - Build test programs (default: OFF)
- `BUILD_EXAMPLES=ON/OFF` - Build example programs (default: OFF)
- `CMAKE_BUILD_TYPE=Release/Debug` - Build type (default: Release)

### Custom Build
```bash
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DUSE_EXTERNAL_SUMMA=ON \
    -DSUMMA_ROOT=/path/to/summa \
    -DCHARM_ROOT=../charm \
    -DBUILD_TESTS=ON
make -j$(nproc)
```

## Development

### Code Organization
- **Core**: Main application logic, data structures, and algorithms
- **Charm++**: Charm++ specific chares, message handling, and parallel coordination
- **Fortran**: SUMMA interface code and custom Fortran modules
- **Utils**: Logging, timing, and utility functions
