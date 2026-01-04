# PhotoGuru Viewer

**Professional desktop photo viewer with AI-powered semantic analysis**  
Built with C++/Qt6 and simplified Python backend

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-macOS-lightgrey.svg)
![Status](https://img.shields.io/badge/status-MVP%20Ready-brightgreen.svg)

## 🎉 MVP Status: Ready for Beta Testing!

PhotoGuru Viewer MVP is **complete and functional**. All core features are implemented with professional keyboard-driven workflows.

📖 **[Quick Start Guide](docs/QUICK_START_MVP.md)** | 📊 **[MVP Implementation Details](docs/MVP_IMPLEMENTATION.md)** | 📋 **[MVP Summary](docs/MVP_SUMMARY.md)**

---

## ✨ Features (MVP v1.0)

### 🖼️ Professional Photo Viewing
- **Universal Format Support**: JPEG, PNG, HEIF/HEIC, RAW (CR2, NEF, ARW, etc.)
- **Smooth Performance**: Hardware-accelerated rendering, 60fps navigation
- **Smart Zoom**: Mouse wheel zoom, pan, fit-to-window, actual size
- **Loading Indicators**: Visual feedback with animated spinners
- **Fullscreen Mode**: Distraction-free viewing with F11 or Escape

### ⌨️ Keyboard-Driven Workflow
- **Navigation**: Arrow keys, Space for next, Escape for fullscreen exit
- **Zoom**: +/- for zoom, F for fit, Ctrl+0 for 100%
- **File Operations**: F2 rename, Delete to trash, Ctrl+R reveal in Finder
- **Professional**: Complete keyboard control like Adobe Lightroom

### 📁 Essential File Operations
- **Copy/Move**: Select and relocate files easily
- **Rename**: Quick F2 renaming
- **Delete**: Safe deletion to macOS Trash
- **Reveal in Finder**: Jump to file location
- **Open With**: Use external editors

### 🎨 Smart Organization
- **Multi-Select**: Cmd+Click for multiple files
- **Sort Options**: By name, date, or size
- **Adjustable Thumbnails**: 80-300px with live slider
- **Efficient Caching**: Fast browsing of large collections

### 🤖 AI-Powered Analysis (Optional)
- **Cloud-Based**: Uses OpenAI GPT-4 Vision (no heavy local models!)
- **Smart Metadata**: Auto-generates titles, descriptions, tags
- **Simple Setup**: Just add API key, no 2GB downloads
- **Batch Processing**: Analyze entire directories
- **Offline Capable**: Core features work without AI

---

## 🚀 Quick Start

### 1. Build the Application

```bash
# Install dependencies
brew install qt@6 cmake

# Build
./scripts/build.sh

# Run
cd build && ./PhotoGuruViewer
```

### 2. Use the App

```bash
# Open photos
Press Ctrl+Shift+O → Select directory

# Navigate
Use arrow keys or Space

# Organize
- Adjust thumbnail size with slider
- Sort by name/date/size
- Multi-select with Cmd+Click
- Copy/move/rename/delete files
```

### 3. Optional: AI Setup

```bash
# Install Python dependencies
pip install -r python/requirements_mvp.txt

# Set API key
export OPENAI_API_KEY="sk-..."

# Analyze photos
python python/agent_mvp.py analyze photo.jpg --write
```

**See [docs/QUICK_START_MVP.md](docs/QUICK_START_MVP.md) for detailed instructions.**

---

## 📚 Documentation

- **[Quick Start Guide](docs/QUICK_START_MVP.md)** - Get started in 5 minutes
- **[MVP Implementation](docs/MVP_IMPLEMENTATION.md)** - Complete feature documentation  
- **[MVP Summary](docs/MVP_SUMMARY.md)** - Technical implementation details
- **[MVP Analysis](docs/MVP_ANALYSIS.md)** - Strategic planning and decisions

---

## 🎯 MVP Achievements

### Before vs After

| Feature | Before | After MVP |
|---------|--------|-----------|
| Keyboard shortcuts | ❌ None | ✅ Complete set |
| File operations | ❌ None | ✅ Copy/move/rename/delete |
| Multi-select | ❌ No | ✅ Yes |
| Thumbnail size | ❌ Fixed | ✅ Adjustable (80-300px) |
| Sorting | ❌ None | ✅ Name/date/size |
| Loading feedback | ❌ None | ✅ Animated spinner |
| Python agent | 2,893 lines | ✅ 350 lines |
| Dependencies | 2GB | ✅ 50MB |
| AI setup | 10-20 min | ✅ 1 minute |

### Code Quality

- ✅ **87% reduction** in Python agent complexity
- ✅ **99% reduction** in dependency size  
- ✅ **Zero compilation errors**
- ✅ **Production-ready** build

---

## 🛠️ Technical Stack

### Frontend (C++/Qt6)
- **Qt 6.5+**: Modern UI framework
- **libraw**: RAW format support
- **libheif**: HEIF/HEIC support
- **CMake**: Build system

### Backend (Python - Optional)
- **OpenAI API**: Cloud-based image analysis
- **Pillow**: Image processing
- **PyExifTool**: Metadata management

### Why Cloud AI?
- ✅ Superior accuracy (GPT-4 Vision vs local models)
- ✅ Tiny installation (~50MB vs 2GB)
- ✅ Fast startup (<1s vs 10-20s)
- ✅ No GPU required
- ✅ Always up-to-date models

---

#### Linux (Ubuntu/Debian)
```bash
# System dependencies
sudo apt install build-essential cmake pkg-config
sudo apt install qt6-base-dev libqt6concurrent6 libqt6sql6
sudo apt install libopencv-dev libraw-dev libheif-dev
sudo apt install libimage-exiftool-perl
sudo apt install python3-dev pybind11-dev

# Python dependencies for ML backend
pip3 install torch torchvision clip pillow sentence-transformers opencv-python
pip3 install pillow-heif numpy requests
```

### Python Dependencies

Install Python ML dependencies:

```bash
pip3 install -r requirements.txt
```

### Building

```bash
# Clone repository
git clone https://github.com/yourusername/photoguru-viewer.git
cd photoguru-viewer

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run
./PhotoGuruViewer
```

### Building on macOS (Bundle)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(sysctl -n hw.ncpu)

# Create application bundle
make install
open PhotoGuruViewer.app
```

## Usage

### Quick Start

1. **Open Images**: Drag & drop a folder or use `File > Open Directory`
2. **Browse**: Navigate with arrow keys or click thumbnails
3. **Zoom**: Mouse wheel, or toolbar buttons
4. **AI Analysis**: Select image and press `Ctrl+A`
5. **Semantic Search**: Press `Ctrl+F` and enter natural language query

### Keyboard Shortcuts

- `Left/Right Arrow`: Navigate images
- `F`: Fit to window
- `Ctrl+0`: Actual size (100%)
- `Ctrl++/-`: Zoom in/out
- `F11`: Fullscreen
- `Ctrl+F`: Semantic search
- `Ctrl+A`: Run AI analysis
- `Ctrl+O`: Open files
- `Ctrl+Shift+O`: Open directory

### Python Backend Integration

The viewer integrates with `python/agent_v2.py` for ML features. Place `agent_v2.py` in the python directory, or the build will automatically detect it.

## Architecture

```
┌─────────────────────────────────┐
│   Qt6 C++ Application           │
│                                 │
│  ┌─────────────────────────┐   │
│  │  MainWindow (UI)        │   │
│  │  - ImageViewer          │   │
│  │  - ThumbnailGrid        │   │
│  │  - MetadataPanel        │   │
│  │  - SKPBrowser           │   │
│  └───────────┬─────────────┘   │
│              │                  │
│  ┌───────────▼─────────────┐   │
│  │  Core Engines           │   │
│  │  - ImageLoader (LibRaw) │   │
│  │  - MetadataReader       │   │
│  │  - ThumbnailCache       │   │
│  └───────────┬─────────────┘   │
│              │                  │
│  ┌───────────▼─────────────┐   │
│  │  PythonBridge           │   │
│  │  (pybind11)             │   │
│  └───────────┬─────────────┘   │
└──────────────┼─────────────────┘
               │
┌──────────────▼─────────────────┐
│   Python ML Backend            │
│   (agent_v2.py)                │
│                                │
│  - CLIP Vision Analysis        │
│  - LLM Integration            │
│  - Semantic Key Protocol       │
│  - Quality Analysis            │
│  - Face Detection              │
└────────────────────────────────┘
```

## Project Structure

```
photoguru-viewer/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── LICENSE                 # MIT License
├── requirements.txt        # Main Python dependencies
├── docs/                   # Documentation
│   ├── QUICK_START_MVP.md
│   ├── MVP_IMPLEMENTATION.md
│   ├── MVP_SUMMARY.md
│   └── ...
├── scripts/                # Build and utility scripts
│   ├── build.sh           # Build script
│   ├── check_dependencies.sh
│   └── run_tests.sh
├── python/                 # Python ML backend
│   ├── agent_mvp.py       # MVP agent (cloud-based)
│   ├── agent_v2.py        # Advanced agent (CLIP)
│   └── requirements_mvp.txt
├── src/
│   ├── main.cpp            # Application entry
│   ├── core/               # Core functionality
│   │   ├── ImageLoader.*   # Universal image loading
│   │   ├── MetadataReader.*# EXIF/XMP reading
│   │   └── PhotoMetadata.h # Data structures
│   ├── ml/                 # Python integration
│   │   └── PythonBridge.*  # pybind11 wrapper
│   └── ui/                 # User interface
│       ├── MainWindow.*    # Main application window
│       ├── ImageViewer.*   # Image display widget
│       ├── ThumbnailGrid.* # Gallery view
│       ├── MetadataPanel.* # Metadata display
│       ├── SKPBrowser.*    # Semantic keys browser
│       └── DarkTheme.h     # Professional dark theme
├── resources/              # Icons, stylesheets
├── tests/                  # Unit tests
└── thirdparty/             # External dependencies
    └── pybind11/           # Python binding library
```

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

MIT License - see LICENSE file for details

## Acknowledgments

- **Qt Framework**: Cross-platform UI framework
- **LibRaw**: RAW image decoding
- **OpenAI CLIP**: Vision-language model
- **PyIQA**: Image quality assessment
- **exiftool**: Metadata reading/writing

## Support

For issues, questions, or feature requests:
- GitHub Issues: https://github.com/yourusername/photoguru-viewer/issues
- Documentation: https://photoguru.ai/docs

## Roadmap

- [ ] Non-destructive editing (curves, levels, color)
- [ ] Batch processing
- [ ] Cloud sync
- [ ] Plugin system
- [ ] Video support
- [ ] Windows support

---

**Built with ❤️ for photographers who need professional tools**
