#!/bin/bash
# SUMMA Configuration Script for Charm++ Integration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Configuring SUMMA for Charm++ Integration${NC}"

# Check if we're in the right directory
if [ ! -f "Makefile" ] || [ ! -d "charm" ]; then
    echo -e "${RED}Error: Please run this script from the summa-charm project root directory${NC}"
    exit 1
fi

# Initialize SUMMA submodule if not already done
if [ ! -d "summa/.git" ]; then
    echo -e "${YELLOW}Initializing SUMMA submodule...${NC}"
    git submodule update --init --recursive
else
    echo -e "${GREEN}SUMMA submodule already initialized${NC}"
fi

# Check if SUMMA source files exist
if [ ! -f "summa/build/source/driver/summa_driver.f90" ]; then
    echo -e "${RED}Error: SUMMA source files not found. Please check submodule initialization.${NC}"
    exit 1
fi

# Create symbolic links to avoid copying files
SUMMA_ACTORS_PATH="/u1/pma753/Summa-Actors/build/summa"

if [ -d "$SUMMA_ACTORS_PATH" ]; then
    echo -e "${YELLOW}Found existing SUMMA installation at $SUMMA_ACTORS_PATH${NC}"
    echo -e "${YELLOW}Creating symbolic links to avoid duplication...${NC}"
    
    # Create backup of current summa directory
    if [ -d "summa" ] && [ ! -L "summa" ]; then
        echo -e "${YELLOW}Backing up current summa directory...${NC}"
        mv summa summa_backup_$(date +%Y%m%d_%H%M%S)
    fi
    
    # Create symbolic link
    ln -sf "$SUMMA_ACTORS_PATH" summa_external
    echo -e "${GREEN}Created symbolic link to external SUMMA installation${NC}"
    
    # Update Makefile to use external SUMMA
    sed -i 's|SUMMA_DIR     = ./summa|SUMMA_DIR     = ./summa_external|g' Makefile
    echo -e "${GREEN}Updated Makefile to use external SUMMA installation${NC}"
else
    echo -e "${YELLOW}External SUMMA installation not found, using submodule${NC}"
fi

# Check dependencies
echo -e "${YELLOW}Checking dependencies...${NC}"

# Check for NetCDF
if ! pkg-config --exists netcdf-fortran 2>/dev/null; then
    echo -e "${RED}Warning: NetCDF-Fortran not found via pkg-config${NC}"
    echo -e "${YELLOW}Make sure NetCDF is installed and LIBS path is correct in Makefile${NC}"
else
    echo -e "${GREEN}NetCDF-Fortran found${NC}"
fi

# Check for gfortran
if ! command -v gfortran &> /dev/null; then
    echo -e "${RED}Error: gfortran not found. Please install GNU Fortran compiler.${NC}"
    exit 1
else
    echo -e "${GREEN}GNU Fortran compiler found${NC}"
fi

# Check Charm++ installation
if [ ! -f "charm/bin/charmc" ]; then
    echo -e "${RED}Error: Charm++ not properly installed. Please check charm directory.${NC}"
    exit 1
else
    echo -e "${GREEN}Charm++ installation found${NC}"
fi

echo -e "${GREEN}SUMMA configuration completed successfully!${NC}"
echo -e "${YELLOW}You can now run 'make all' to build the project${NC}"
