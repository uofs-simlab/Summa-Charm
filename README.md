# SUMMA-Charm++

A Charm++ parallel implementation of the SUMMA hydrological model, converted from the original CAF-based SUMMA-Actors.

## 🎉 Project Status: **Reorganized & Core Components Working!**

✅ **Complete project restructuring with modern CMake**  
✅ **Core C++ components building successfully**  
✅ **Professional development infrastructure**  
⚠️ **Charm++ integration** (needs CkMarshall.decl.h fix)  
⚠️ **SUMMA Fortran integration** (needs dependency resolution)  

**👉 See [COMPLETION_SUMMARY.md](COMPLETION_SUMMARY.md) for detailed status and next steps.**

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

## Configuration

### SUMMA Integration
The project is configured to use an external SUMMA installation located at:
```
/u1/pma753/Summa-Actors/build/summa
```

To use a different SUMMA installation, either:
1. Modify the path in `scripts/build.sh`
2. Set `SUMMA_ROOT` when running cmake
3. Use the SUMMA submodule by setting `-DUSE_EXTERNAL_SUMMA=OFF`

### Charm++ Integration
The project expects Charm++ to be built in the `charm/` directory. Ensure you have:
- `charm/bin/charmc` - Charm++ compiler
- `charm/bin/charmrun` - Charm++ runtime

## Development

### Code Organization
- **Core**: Main application logic, data structures, and algorithms
- **Charm++**: Charm++ specific chares, message handling, and parallel coordination
- **Fortran**: SUMMA interface code and custom Fortran modules
- **Utils**: Logging, timing, and utility functions

### Adding New Features
1. Place source files in appropriate `src/` subdirectory
2. Place headers in corresponding `include/` subdirectory
3. Update the relevant `CMakeLists.txt` file
4. Rebuild with `./scripts/build.sh`

## Converting from CAF to Charm++

This project is a conversion from the original CAF-based SUMMA-Actors. Key differences:

### Replaced Components
- **CAF Actors** → **Charm++ Chares**
- **CAF Message Passing** → **Charm++ Entry Methods**
- **CAF Scheduler** → **Charm++ Runtime**

### Architecture Changes
- Distributed actor model using Charm++ chares
- Asynchronous message passing via entry methods
- Automatic load balancing through Charm++ runtime

## Troubleshooting

### Common Issues

1. **Charm++ not found**
   - Ensure Charm++ is built in `charm/` directory
   - Check that `charmc` and `charmrun` are executable

2. **NetCDF not found**
   - Install NetCDF development packages
   - Set `NETCDF_ROOT` environment variable

3. **SUMMA path errors**
   - Verify SUMMA installation path
   - Check that SUMMA source files exist

4. **Fortran compilation errors**
   - Ensure gfortran is installed and recent
   - Check that NetCDF Fortran bindings are available

## Performance

### Running in Parallel
```bash
cd build
../charm/bin/charmrun +p8 ./bin/summa-charm ++local \
    -c ../scripts/config.json -m ../scripts/fileManager.txt
```

### Load Balancing
Charm++ provides automatic load balancing. To enable:
- Use `++lb` runtime option
- Compile with load balancing strategies

## Contributing

1. Follow the existing code organization
2. Add tests for new functionality
3. Update documentation
4. Ensure CMake build works cleanly

## License

This project maintains compatibility with SUMMA's licensing terms.
