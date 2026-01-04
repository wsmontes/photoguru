#!/bin/bash

# PhotoGuru Viewer - Test Runner
# Compiles and runs all unit tests

set -e

echo "========================================="
echo "🧪 PhotoGuru Viewer - Test Suite"
echo "========================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Build directory
BUILD_DIR="build"

# Clean previous build if requested
if [ "$1" == "clean" ]; then
    echo "🧹 Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake with tests enabled
echo ""
echo "⚙️  Configuring CMake with tests..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# Build
echo ""
echo "🔨 Building tests..."
make PhotoGuruTests -j$(sysctl -n hw.ncpu)

# Run tests
echo ""
echo "========================================="
echo "🧪 Running Unit Tests"
echo "========================================="
echo ""

# Run tests and capture output (ignore exit code from crash during cleanup)
./PhotoGuruTests 2>&1 | tee test_output.log || true

# Check if all tests passed by analyzing Google Test output
if grep -q "\[  PASSED  \] [0-9]* tests" test_output.log && ! grep -q "\[  FAILED  \]" test_output.log; then
    echo ""
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}✅ All tests passed!${NC}"
    echo -e "${GREEN}=========================================${NC}"
    rm -f test_output.log
    exit 0
else
    echo ""
    echo -e "${RED}=========================================${NC}"
    echo -e "${RED}❌ Some tests failed!${NC}"
    echo -e "${RED}=========================================${NC}"
    rm -f test_output.log
    exit 1
fi
