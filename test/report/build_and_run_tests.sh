#!/bin/bash

# ZeroCopy Report Test Suite Builder and Runner
# This script builds and runs all report module tests

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "========================================"
echo "  ZeroCopy Report Test Suite Builder"
echo "========================================"
echo ""

# Clean old build
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning old build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
echo -e "${BLUE}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run CMake
echo -e "${BLUE}Running CMake...${NC}"
cmake .. || {
    echo -e "${RED}❌ CMake configuration failed!${NC}"
    exit 1
}

# Build
echo ""
echo -e "${BLUE}Building tests...${NC}"
make -j$(nproc) || {
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
}

echo ""
echo -e "${GREEN}✅ Build completed successfully!${NC}"
echo ""

# Ask user which test to run
echo "========================================"
echo "  Available Tests"
echo "========================================"
echo "  1. test_comprehensive  - Comprehensive functionality test"
echo "  2. test_performance    - Performance benchmark"
echo "  3. Run all tests"
echo "  0. Exit without running"
echo "========================================"
echo ""

read -p "Select test to run (0-3): " choice

run_test() {
    local test_name=$1
    echo ""
    echo "========================================"
    echo "  Running: $test_name"
    echo "========================================"
    echo ""
    
    if [ -f "./$test_name" ]; then
        ./"$test_name"
        echo ""
        echo -e "${GREEN}✅ $test_name completed${NC}"
    else
        echo -e "${RED}❌ Test executable not found: $test_name${NC}"
    fi
}

case $choice in
    1)
        run_test "test_comprehensive"
        ;;
    2)
        run_test "test_performance"
        ;;
    3)
        echo ""
        echo "========================================"
        echo "  Running All Tests"
        echo "========================================"
        
        for test in test_comprehensive test_performance; do
            run_test "$test"
        done
        
        echo ""
        echo "========================================"
        echo -e "${GREEN}✅ All tests completed!${NC}"
        echo "========================================"
        ;;
    0)
        echo ""
        echo "Exiting without running tests."
        ;;
    *)
        echo -e "${RED}Invalid choice!${NC}"
        exit 1
        ;;
esac

echo ""
echo "========================================"
echo "  Test Suite Completed"
echo "========================================"

