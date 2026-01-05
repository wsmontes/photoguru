#!/bin/bash
# Build script for PhotoGuru Viewer
# Usage: ./build.sh [clean|release|debug]

set -e  # Exit on error

BUILD_TYPE="Release"
CLEAN=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        clean)
            CLEAN=true
            ;;
        release)
            BUILD_TYPE="Release"
            ;;
        debug)
            BUILD_TYPE="Debug"
            ;;
        *)
            echo "Usage: $0 [clean|release|debug]"
            exit 1
            ;;
    esac
done

echo "========================================="
echo "PhotoGuru Viewer Build Script"
echo "Build Type: $BUILD_TYPE"
echo "========================================="

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "❌ CMake not found. Install with: brew install cmake"; exit 1; }
command -v exiftool >/dev/null 2>&1 || { echo "⚠️  Warning: exiftool not found. Install with: brew install exiftool"; }

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "🧹 Cleaning build directory..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Configure
echo "⚙️  Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# Build
echo "🔨 Building..."
NPROC=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
make -j$NPROC

echo ""
echo "========================================="
echo "✅ Build complete!"
echo "========================================="
echo ""
echo "Executable: $(pwd)/PhotoGuruViewer"
echo ""
echo "To run:"
echo "  cd build"
echo "  ./PhotoGuruViewer"
echo ""
echo "Or to create macOS bundle:"
echo "  make install"
echo "  open PhotoGuruViewer.app"
echo ""
