# SUMMA-Charm++ Build System

This document explains how to compile and run the SUMMA-Charm++ project using the provided Makefile and build scripts.

## Quick Start

### Prerequisites

- **Charm++**: Must be built in the `charm/` directory
- **NetCDF**: C and Fortran libraries
- **LAPACK/BLAS**: Linear algebra libraries
- **GNU Fortran**: gfortran compiler
- **C++17** compatible compiler

### Build and Run

```bash
# Build the project
make

# Run with default settings (4 processors)
make run

# Run basic tests
make test

# Clean build artifacts
make clean
```

## Makefile Targets

### Build Targets

- **`make all`** (default) - Build the project
- **`make debug`** - Build with debug flags
- **`make release`** - Build with optimization flags (default)
- **`make clean`** - Clean build artifacts
- **`make clean-all`** - Clean everything including SUMMA components

### Run Targets

- **`make run`** - Run with default settings (4 processors)
- **`make run-args ARGS="..."`** - Run with custom arguments
- **`make test`** - Run basic functionality test

### Utility Targets

- **`make info`** - Show build configuration
- **`make check-deps`** - Check for required dependencies
- **`make help`** - Show detailed help message

## Configuration Variables

You can customize the build by setting these variables:

```bash
# Number of processors for running
make run PROCS=8

# Enable debug build
make debug

# Custom Charm++ directory
make all CHARM_DIR=/path/to/charm

# Use different compiler flags
make all CXX_FLAGS="-O2 -g"
```

## Build Script

For convenience, use the `build.sh` script:

```bash
# Show help
./build.sh help

# Build the project
./build.sh build

# Debug build
./build.sh build --debug

# Build and run with 8 processors
./build.sh run -p 8

# Clean and rebuild
./build.sh build --clean-first

# Run tests
./build.sh test
```

## Examples

### Basic Usage

```bash
# Build and run
make && make run

# Debug build and test
make debug && make test

# Clean rebuild with 8 processors
make clean && make && make run PROCS=8
```

### Advanced Usage

```bash
# Run with custom config file
make run-args ARGS="-c config.json -m fileManager.txt"

# Debug build with custom processors
make clean && make debug && make run PROCS=2

# Check dependencies before building
make check-deps && make all
```

## Troubleshooting

### Common Issues

1. **Charm++ not found**
   ```
   Error: Charm++ not found at ./charm/bin/charmc
   ```
   **Solution**: Ensure Charm++ is built in the `charm/` directory

2. **NetCDF not found**
   ```
   Error: NetCDF ✗ Not found
   ```
   **Solution**: Install NetCDF development packages

3. **Compilation errors**
   ```
   Error: fatal error: someheader.h: No such file or directory
   ```
   **Solution**: Check include paths and ensure all dependencies are installed

### Debugging

```bash
# Check build configuration
make info

# Check dependencies
make check-deps

# Clean and debug build
make clean && make debug

# Verbose compilation
make all V=1
```

## File Structure

```
build/                 # Build directory
├── bin/              # Compiled executables
│   └── summa-charm   # Main executable
└── obj/              # Object files
    ├── core/         # Core component objects
    ├── charm/        # Charm++ component objects
    └── utils/        # Utility component objects

src/                   # Source files
├── core/             # Core C++ source
├── charm/            # Charm++ interface source
└── utils/            # Utility source

include/               # Header files
├── core/             # Core headers
├── charm/            # Charm++ headers
└── utils/            # Utility headers
```

## Performance Tips

1. **Parallel Building**: Use `make -j N` for faster compilation
2. **Release Build**: Use `make release` for optimized code
3. **Processor Count**: Adjust `PROCS` for your system
4. **Memory**: Ensure sufficient memory for large processor counts

## Development Workflow

```bash
# Daily development cycle
make clean          # Clean previous build
make debug          # Debug build for development
make test           # Run tests

# Before committing
make clean          # Clean build
make release        # Optimized build
make test           # Verify functionality
```

## Integration with IDEs

### VS Code

Add to `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build SUMMA-Charm++",
            "type": "shell",
            "command": "make",
            "group": "build",
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Run SUMMA-Charm++",
            "type": "shell",
            "command": "make",
            "args": ["run"],
            "group": "test"
        }
    ]
}
```

### CLion/CMake

While CMake files exist, the Makefile provides more direct control over the Charm++ build process.

## Advanced Configuration

### Custom Compiler Flags

```bash
# Add custom flags
make CXX_FLAGS="-O3 -march=native -DCUSTOM_FLAG"

# Debug with sanitizers
make debug CXX_FLAGS="-g -O0 -fsanitize=address -fsanitize=undefined"
```

### Library Paths

```bash
# Custom library paths
make LIBS="-L/custom/path -lnetcdf -lnetcdff -llapack"

# Additional includes
make INCLUDES="-I./include -I/custom/include"
```

## Testing

The build system includes several test targets:

```bash
# Basic functionality test
make test

# Run with different processor counts
make run PROCS=1   # Single processor
make run PROCS=2   # Two processors  
make run PROCS=8   # Eight processors
```

## Support

For build issues:

1. Check `make check-deps` output
2. Review `make info` configuration
3. Try `make clean && make debug` for better error messages
4. Check the project README.md for additional troubleshooting
