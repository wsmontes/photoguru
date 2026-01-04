#!/bin/bash
# Verify PhotoGuru Viewer build environment
# Run this before building to check all dependencies

echo "========================================="
echo "PhotoGuru Viewer - Dependency Checker"
echo "========================================="
echo ""

ERRORS=0
WARNINGS=0

# Function to check command
check_command() {
    if command -v $1 >/dev/null 2>&1; then
        VERSION=$($2 2>&1 || echo "unknown")
        echo "✅ $1: $VERSION"
    else
        echo "❌ $1: NOT FOUND"
        echo "   Install: $3"
        ((ERRORS++))
    fi
}

# Function to check library
check_library() {
    if pkg-config --exists $1 2>/dev/null; then
        VERSION=$(pkg-config --modversion $1)
        echo "✅ $1: $VERSION"
    else
        echo "❌ $1: NOT FOUND"
        echo "   Install: $2"
        ((ERRORS++))
    fi
}

# Function to check Python module
check_python_module() {
    if python3 -c "import $1" 2>/dev/null; then
        VERSION=$(python3 -c "import $1; print(getattr($1, '__version__', 'installed'))" 2>/dev/null || echo "installed")
        echo "✅ Python $1: $VERSION"
    else
        echo "❌ Python $1: NOT FOUND"
        echo "   Install: pip3 install $2"
        ((WARNINGS++))
    fi
}

echo "📋 Checking Build Tools..."
check_command "cmake" "cmake --version | head -1" "brew install cmake"
check_command "make" "make --version | head -1" "xcode-select --install"
check_command "git" "git --version" "xcode-select --install"

echo ""
echo "📋 Checking Required Libraries..."
check_command "exiftool" "exiftool -ver" "brew install exiftool"

# Check Qt6
if command -v qmake6 >/dev/null 2>&1; then
    QT_VERSION=$(qmake6 -query QT_VERSION)
    echo "✅ Qt6: $QT_VERSION"
elif command -v qmake >/dev/null 2>&1; then
    QT_VERSION=$(qmake -query QT_VERSION)
    if [[ $QT_VERSION == 6.* ]]; then
        echo "✅ Qt6: $QT_VERSION"
    else
        echo "❌ Qt6: Found Qt $QT_VERSION (need Qt6)"
        echo "   Install: brew install qt6"
        ((ERRORS++))
    fi
else
    echo "❌ Qt6: NOT FOUND"
    echo "   Install: brew install qt6"
    ((ERRORS++))
fi

# Check OpenCV
if pkg-config --exists opencv4; then
    check_library "opencv4" "brew install opencv"
elif pkg-config --exists opencv; then
    check_library "opencv" "brew install opencv"
else
    echo "❌ OpenCV: NOT FOUND"
    echo "   Install: brew install opencv"
    ((ERRORS++))
fi

# Check LibRaw
if [ -f "/opt/homebrew/lib/libraw.dylib" ] || [ -f "/usr/local/lib/libraw.dylib" ]; then
    echo "✅ LibRaw: installed"
else
    echo "❌ LibRaw: NOT FOUND"
    echo "   Install: brew install libraw"
    ((ERRORS++))
fi

# Check libheif
if [ -f "/opt/homebrew/lib/libheif.dylib" ] || [ -f "/usr/local/lib/libheif.dylib" ]; then
    echo "✅ libheif: installed"
else
    echo "⚠️  libheif: NOT FOUND (HEIF support will be disabled)"
    echo "   Install: brew install libheif"
    ((WARNINGS++))
fi

echo ""
echo "📋 Checking Python Environment..."
check_command "python3" "python3 --version" "brew install python@3.11"

# Check Python modules (warnings only, not required for build)
echo ""
echo "📋 Checking Python ML Dependencies (optional for AI features)..."
check_python_module "torch" "torch"
check_python_module "PIL" "pillow"
check_python_module "clip" "git+https://github.com/openai/CLIP.git"
check_python_module "sentence_transformers" "sentence-transformers"
check_python_module "cv2" "opencv-python"
check_python_module "numpy" "numpy"

echo ""
echo "📋 Checking pybind11..."
if [ -d "thirdparty/pybind11" ]; then
    echo "✅ pybind11: found in thirdparty/"
else
    echo "⚠️  pybind11: NOT FOUND"
    echo "   Will be downloaded automatically during build"
    echo "   Or run: git clone https://github.com/pybind/pybind11.git thirdparty/pybind11"
    ((WARNINGS++))
fi

echo ""
echo "========================================="
if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo "✅ All dependencies satisfied!"
    echo "Ready to build. Run: ./build.sh"
elif [ $ERRORS -eq 0 ]; then
    echo "⚠️  $WARNINGS warning(s) - build will work but some features may be limited"
    echo "Ready to build. Run: ./build.sh"
else
    echo "❌ $ERRORS error(s) found - please install missing dependencies"
    echo ""
    echo "Quick fix (macOS):"
    echo "  brew install qt6 opencv libraw libheif exiftool cmake"
    echo "  pip3 install -r requirements.txt"
fi
echo "========================================="

exit $ERRORS
