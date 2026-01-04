# PhotoGuru Viewer - Quick Start Guide

## Installation Instructions

### macOS

#### 1. Install System Dependencies

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install all required dependencies
brew install qt6 opencv libraw libheif exiftool cmake pkg-config python@3.11

# Verify Qt6 installation
brew list qt6
```

#### 2. Install Python Dependencies

```bash
# Create virtual environment (recommended)
python3 -m venv venv
source venv/bin/activate

# Install Python ML packages
pip3 install --upgrade pip
pip3 install torch torchvision
pip3 install git+https://github.com/openai/CLIP.git
pip3 install pillow pillow-heif opencv-python numpy sentence-transformers pyiqa requests
```

#### 3. Build the Application

```bash
# Clone or navigate to the project
cd /path/to/Photoguru-viewer

# Run the build script
./build.sh

# Or build manually:
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)
```

#### 4. Run

```bash
cd build
./PhotoGuruViewer

# Or open a directory directly
./PhotoGuruViewer /path/to/photos
```

---

### Linux (Ubuntu/Debian)

#### 1. Install System Dependencies

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake pkg-config git

# Install Qt6
sudo apt install -y qt6-base-dev qt6-base-dev-tools \
    libqt6concurrent6 libqt6sql6 libqt6widgets6

# Install image libraries
sudo apt install -y libopencv-dev libraw-dev libheif-dev

# Install exiftool
sudo apt install -y libimage-exiftool-perl

# Install Python and pybind11
sudo apt install -y python3-dev python3-pip pybind11-dev
```

#### 2. Install Python Dependencies

```bash
# Install Python packages
pip3 install torch torchvision --index-url https://download.pytorch.org/whl/cpu
pip3 install git+https://github.com/openai/CLIP.git
pip3 install pillow pillow-heif opencv-python numpy sentence-transformers pyiqa requests
```

#### 3. Build

```bash
cd /path/to/Photoguru-viewer

# Setup pybind11
git clone --depth 1 --branch v2.11.1 https://github.com/pybind/pybind11.git thirdparty/pybind11

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### 4. Run

```bash
./PhotoGuruViewer
```

---

## First Run Checklist

1. ✅ **Verify exiftool**: Run `exiftool -ver` to confirm installation
2. ✅ **Test Python backend**: Run `python3 agent_v2.py --help` to verify ML dependencies
3. ✅ **Check image formats**: Open File > Open Directory and test with RAW, HEIF, and JPEG files
4. ✅ **Test AI features**: Select an image and press Ctrl+A to run analysis

---

## Common Issues

### Qt6 Not Found

**macOS:**
```bash
# Make sure Qt6 is in PATH
export Qt6_DIR=$(brew --prefix qt6)/lib/cmake/Qt6
cmake -DQt6_DIR=$Qt6_DIR ..
```

**Linux:**
```bash
# Install Qt6 from official installer if apt version is too old
wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
chmod +x qt-unified-linux-x64-online.run
./qt-unified-linux-x64-online.run
```

### LibRaw Not Found

**macOS:**
```bash
brew reinstall libraw
export PKG_CONFIG_PATH=$(brew --prefix libraw)/lib/pkgconfig:$PKG_CONFIG_PATH
```

**Linux:**
```bash
sudo apt install libraw-dev pkg-config
```

### Python Bridge Errors

Make sure Python packages are installed in the correct environment:

```bash
python3 -c "import torch, clip, PIL; print('✅ All imports successful')"
```

If imports fail:
```bash
pip3 install --force-reinstall torch torchvision clip pillow
```

### HEIF Support Missing

**macOS:**
```bash
brew install libheif
```

**Linux:**
```bash
sudo apt install libheif-dev
```

Then rebuild:
```bash
cd build
rm CMakeCache.txt
cmake ..
make
```

---

## Performance Tips

1. **GPU Acceleration**: If you have NVIDIA GPU, install CUDA-enabled PyTorch:
   ```bash
   pip3 install torch torchvision --index-url https://download.pytorch.org/whl/cu118
   ```

2. **Thumbnail Cache**: The app caches thumbnails in memory. For large libraries (10,000+ images), you may want to increase the cache size in [ThumbnailGrid.cpp](src/ui/ThumbnailGrid.cpp):
   ```cpp
   m_thumbnailCache.setMaxCost(1000);  // Increase from 500
   ```

3. **RAW Loading**: For faster preview, RAW files are loaded at half resolution by default. For full resolution, modify [ImageLoader.cpp](src/core/ImageLoader.cpp).

---

## Next Steps

- Read the [README.md](README.md) for full feature documentation
- Check [agent_v2.py](agent_v2.py) documentation for ML backend details
- Explore keyboard shortcuts in Help > About menu

---

## Getting Help

- **Issues**: https://github.com/yourusername/photoguru-viewer/issues
- **Discussions**: https://github.com/yourusername/photoguru-viewer/discussions
- **Email**: support@photoguru.ai

---

**Happy Photo Management! 📸**
