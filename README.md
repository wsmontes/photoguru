# PhotoGuru Viewer

**Professional desktop photo viewer with AI-powered semantic analysis**  
Built with C++/Qt6 and native AI models (CLIP + VLM)

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-macOS-lightgrey.svg)
![Status](https://img.shields.io/badge/status-Production%20Ready-brightgreen.svg)

## 🎉 Status: Full C++ Implementation Complete!

PhotoGuru Viewer has **completed migration from Python to C++**. All AI features now run natively with:
- **CLIP** (ONNX Runtime) for semantic embeddings
- **Qwen3-VL 4B** (llama.cpp) for image captioning
- **Zero Python dependencies** for core functionality
- **Comprehensive logging** system for debugging

📖 **[Implementation Details](docs/IMPLEMENTATION_COMPLETE.md)** | 📊 **[Performance Analysis](docs/PERFORMANCE_ANALYSIS.md)** | 🐍 **[Python Migration Summary](docs/PYTHON_TO_CPP_MIGRATION.md)**

---

## ✨ Features

### 🖼️ Professional Photo Viewing
- **Universal Format Support**: JPEG, PNG, HEIF/HEIC, RAW (CR2, NEF, ARW, etc.)
- **Smooth Performance**: Hardware-accelerated rendering, 60fps navigation
- **Smart Zoom**: Mouse wheel zoom, pan, fit-to-window, actual size
- **Loading Indicators**: Visual feedback with animated spinners
- **Fullscreen Mode**: Distraction-free viewing with F11 or Escape

### 🤖 Native AI Analysis (100% C++)
- **CLIP Vision Embeddings**: 512-dim semantic vectors (50-230ms)
- **VLM Image Captioning**: Qwen3-VL 4B generates natural language descriptions (0.6-6.6s)
- **Automatic Metadata Writing**: ExifTool daemon integration
- **5 Analysis Functions**:
  1. Single image analysis (CLIP + VLM + metadata)
  2. Batch directory analysis
  3. Duplicate detection (CLIP similarity > 0.95)
  4. Burst sequence detection
  5. Quality report generation
- **Copy to Clipboard**: Generated captions accessible instantly
- **Comprehensive Logging**: All operations logged with timestamps

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

---

## 🚀 Quick Start

### 1. Install Dependencies

```bash
# macOS - Install Homebrew packages
brew install qt@6 cmake onnxruntime

# Add Qt to PATH
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
```

### 2. Download AI Models

```bash
# Download CLIP model (335MB)
./scripts/download_models.sh clip

# Download VLM models (2.7GB - optional but recommended)
./scripts/download_models.sh vlm

# Or download both
./scripts/download_models.sh all
```

**Models will be saved to `models/` directory:**
- `clip-vit-base-patch32.onnx` (335MB) - CLIP embeddings
- `Qwen3VL-4B-Instruct-Q4_K_M.gguf` (2.3GB) - VLM model
- `mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf` (433MB) - Vision projector

### 3. Build the Application

```bash
# Build
./scripts/build.sh

# Run
cd build && ./PhotoGuruViewer.app/Contents/MacOS/PhotoGuruViewer
```

### 4. Use the App

```bash
# Open photos
Press Ctrl+Shift+O → Select directory

# Navigate
Use arrow keys or Space

# Analyze with AI
Click "Analyze with AI" button
- CLIP embeddings computed (~50ms)
- VLM caption generated (~5s first time, <1s cached)
- Metadata written to image file
- Caption displayed with copy button

# Batch operations
- "Analyze All Images in Folder" - Process entire directory
- "Find Duplicates" - Detect similar images (>95% similarity)
- "Detect Burst Groups" - Find photo sequences
- "Generate Quality Report" - Score all images

# View logs
Click "📄 Open Full Log File" button
Log location: ~/Library/Application Support/PhotoGuru/PhotoGuru Viewer/photoguru.log
```

**See [docs/QUICK_START_MVP.md](docs/QUICK_START_MVP.md) for detailed instructions.**

---

## 📚 Documentation

- **[Implementation Complete](docs/IMPLEMENTATION_COMPLETE.md)** - C++ migration details
- **[Performance Analysis](docs/PERFORMANCE_ANALYSIS.md)** - Benchmarks and metrics
- **[Python Migration](docs/PYTHON_TO_CPP_MIGRATION.md)** - Migration process
- **[Local AI Setup](docs/LOCAL_AI_SETUP.md)** - CLIP and VLM configuration
- **[ExifTool Daemon](docs/EXIFTOOL_DAEMON.md)** - Metadata writing
- **[Quick Start Guide](docs/QUICK_START_MVP.md)** - Basic usage

---

## 🎯 Key Achievements

### Migration Results

| Aspect | Before (Python) | After (C++) |
|--------|----------------|-------------|
| **AI Backend** | Python subprocess | Native C++ |
| **CLIP** | Python/PyTorch | ONNX Runtime |
| **VLM** | N/A | llama.cpp (Qwen3-VL) |
| **Dependencies** | 2GB+ Python packages | 335MB ONNX model |
| **Startup Time** | 10-20s | <1s |
| **Analysis Speed** | Variable | 50-230ms (CLIP) |
| **Memory Usage** | ~2GB | ~500MB |
| **Logging** | Basic print | Comprehensive file logging |
| **Tests** | 183/191 passing | 185/191 passing |

### Performance Metrics (macOS M4)

- **CLIP Embeddings**: 50-230ms per image
- **VLM Caption (first)**: 6.6s (loading 2.3GB model)
- **VLM Caption (cached)**: 0.6-1.0s (10x faster!)
- **Batch Processing**: 14 images in 1.5s (CLIP only)
- **Duplicate Detection**: 14 images compared in 1.5s
- **Burst Detection**: <10ms (instantaneous)

---

## 🛠️ Technical Stack

### Core (C++/Qt6)
- **Qt 6.5+**: Modern UI framework
- **ONNX Runtime 1.22**: CLIP inference engine
- **llama.cpp**: VLM inference (Qwen3-VL)
- **libraw**: RAW format support
- **libheif**: HEIF/HEIC support
- **ExifTool**: Metadata reading/writing (daemon mode)
- **CMake**: Build system

### AI Models
- **CLIP-ViT-Base-Patch32**: 512-dimensional semantic embeddings
  - Source: OpenAI CLIP
  - Format: ONNX (optimized)
  - Size: 335MB
  - Performance: 50-230ms per image
  
- **Qwen3-VL-4B-Instruct**: Vision-Language Model for image captioning
  - Source: Qwen (Alibaba Cloud)
  - Format: GGUF (quantized Q4_K_M)
  - Size: 2.3GB model + 433MB projector
  - Performance: 0.6-6.6s per caption

### Logging System
- **Custom Logger Class**: Singleton pattern with thread-safety
- **4 Log Levels**: DEBUG, INFO, WARNING, ERROR
- **Auto-Rotation**: 10MB max size with .old backup
- **File Location**: `~/Library/Application Support/PhotoGuru/PhotoGuru Viewer/photoguru.log`
- **Captures**: All user actions, timings, errors, performance metrics

---

## 🔧 Building from Source

### Prerequisites

#### macOS
```bash
# Install Homebrew dependencies
brew install qt@6 cmake onnxruntime

# Add Qt to PATH
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"

# Download models (required)
./scripts/download_models.sh all
```

#### Linux (Ubuntu/Debian)
```bash
# System dependencies
sudo apt install build-essential cmake pkg-config
sudo apt install qt6-base-dev libqt6concurrent6 libqt6sql6
sudo apt install libraw-dev libheif-dev libimage-exiftool-perl

# ONNX Runtime (build from source or download binary)
# See: https://onnxruntime.ai/docs/build/

# llama.cpp will be downloaded automatically by CMake
```

### Build Steps

```bash
# Clone repository
git clone https://github.com/wsmontes/photoguru.git
cd photoguru

# Download AI models
./scripts/download_models.sh all

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build (use all CPU cores)
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # macOS

# Run
./PhotoGuruViewer.app/Contents/MacOS/PhotoGuruViewer  # macOS
./PhotoGuruViewer  # Linux
```

### Running Tests

```bash
cd build
./PhotoGuruTests

# Current status: 185/191 passing (96.9%)
```

---

## 📖 Usage Guide

### Analyzing Images

1. **Single Image Analysis**:
   - Select an image in the viewer
   - Click "🔍 Analyze with AI" button
   - Wait for CLIP embeddings (~50ms) and VLM caption (~5s)
   - Caption appears in display panel
   - Click "📋 Copy" to copy to clipboard
   - Metadata automatically written to image file

2. **Batch Directory Analysis**:
   - Select a directory
   - Click "📁 Analyze All Images in Folder"
   - Progress bar shows processing status
   - Each image gets CLIP embeddings
   - Optional: VLM captions if enabled
   - Check log for detailed results

3. **Find Duplicates**:
   - Click "🔄 Find Duplicates"
   - CLIP compares all images in directory
   - Reports pairs with >95% similarity
   - Results shown in log panel

4. **Detect Bursts**:
   - Click "📸 Detect Burst Groups"
   - Finds sequences of photos taken within 5 seconds
   - Groups shown in log with filenames

5. **Generate Report**:
   - Click "📊 Generate Quality Report"
   - Analyzes resolution and file size
   - Shows top 20 images by quality score
   - Sorted by resolution × file size heuristic

### Viewing Logs

All operations are logged with timestamps:
```bash
# View log file
tail -f ~/Library/Application\ Support/PhotoGuru/PhotoGuru\ Viewer/photoguru.log

# Or click "📄 Open Full Log File" button in app
```

Log includes:
- User actions (clicks, selections)
- AI operation timings
- Error messages and warnings
- Performance metrics
- Caption generation results

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────┐
│   PhotoGuru Viewer (Qt6 C++ Application)   │
│                                             │
│  ┌───────────────────────────────────────┐ │
│  │  UI Layer                             │ │
│  │  - MainWindow (main application)      │ │
│  │  - ImageViewer (display & zoom)       │ │
│  │  - ThumbnailGrid (gallery)            │ │
│  │  - AnalysisPanel (AI controls)        │ │
│  │  - MetadataPanel (EXIF/XMP)           │ │
│  │  - FilterPanel (search/filter)        │ │
│  └─────────────┬─────────────────────────┘ │
│                │                             │
│  ┌─────────────▼─────────────────────────┐ │
│  │  Core Layer                           │ │
│  │  - ImageLoader (LibRaw + LibHeif)     │ │
│  │  - MetadataReader (ExifTool)          │ │
│  │  - MetadataWriter (ExifTool daemon)   │ │
│  │  - Logger (file logging + rotation)   │ │
│  │  - ThumbnailCache                     │ │
│  └─────────────┬─────────────────────────┘ │
│                │                             │
│  ┌─────────────▼─────────────────────────┐ │
│  │  ML Layer (Native C++)                │ │
│  │  - CLIPAnalyzer (ONNX Runtime)        │ │
│  │    • 512-dim embeddings               │ │
│  │    • Cosine similarity                │ │
│  │  - LlamaVLM (llama.cpp)               │ │
│  │    • Qwen3-VL 4B model                │ │
│  │    • mtmd multi-modal API             │ │
│  │  - ONNXInference (helper)             │ │
│  └───────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
         │                        │
         │ ONNX                   │ GGUF
         │ Runtime                │ llama.cpp
         ▼                        ▼
┌──────────────────┐    ┌──────────────────┐
│ CLIP Model       │    │ Qwen3-VL Model   │
│ (335MB ONNX)     │    │ (2.7GB GGUF)     │
│                  │    │                  │
│ • Vision encoder │    │ • VLM inference  │
│ • Text encoder   │    │ • Image → Text   │
│ • 512-d output   │    │ • Natural capts  │
└──────────────────┘    └──────────────────┘
```

### Key Components

**CLIPAnalyzer** (`src/ml/CLIPAnalyzer.cpp`):
- Loads ONNX model via ONNX Runtime
- Computes 512-dimensional embeddings
- Handles image preprocessing (resize, normalize)
- Provides cosine similarity calculation
- Thread-safe inference

**LlamaVLM** (`src/ml/LlamaVLM.cpp`):
- Integrates llama.cpp for VLM inference
- Loads Qwen3-VL 4B quantized model
- Uses mtmd (multi-modal) helper API
- Clears memory cache between inferences
- Generates natural language captions

**Logger** (`src/core/Logger.cpp`):
- Singleton pattern for global access
- 4 log levels (DEBUG, INFO, WARNING, ERROR)
- Thread-safe with QMutex
- Auto-rotation at 10MB
- Persistent file: `~/Library/Application Support/.../photoguru.log`

**AnalysisPanel** (`src/ui/AnalysisPanel.cpp`):
- 5 main AI functions (analyze, batch, duplicates, bursts, report)
- Comprehensive logging of all operations
- Caption display with copy-to-clipboard
- Progress tracking with status updates
- Button state management

---

## 📁 Project Structure

```
photoguru/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── LICENSE                     # MIT License
├── .gitignore                  # Git ignore rules
│
├── docs/                       # Documentation
│   ├── IMPLEMENTATION_COMPLETE.md
│   ├── PERFORMANCE_ANALYSIS.md
│   ├── PYTHON_TO_CPP_MIGRATION.md
│   ├── LOCAL_AI_SETUP.md
│   ├── EXIFTOOL_DAEMON.md
│   └── ...
│
├── scripts/                    # Build and utility scripts
│   ├── build.sh               # Main build script
│   ├── download_models.sh     # Model download utility
│   ├── check_dependencies.sh  # Dependency checker
│   └── run_tests.sh           # Test runner
│
├── models/                     # AI models (not in git)
│   ├── clip-vit-base-patch32.onnx  (335MB)
│   ├── Qwen3VL-4B-Instruct-Q4_K_M.gguf  (2.3GB)
│   └── mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf  (433MB)
│
├── src/
│   ├── main.cpp                # Application entry
│   │
│   ├── core/                   # Core functionality
│   │   ├── ImageLoader.*       # Universal image loading
│   │   ├── MetadataReader.*    # EXIF/XMP reading
│   │   ├── MetadataWriter.*    # EXIF/XMP writing (daemon)
│   │   ├── ExifToolDaemon.*    # ExifTool process manager
│   │   ├── Logger.*            # Logging system
│   │   └── PhotoMetadata.h     # Data structures
│   │
│   ├── ml/                     # Machine Learning (C++)
│   │   ├── CLIPAnalyzer.*      # CLIP embeddings (ONNX)
│   │   ├── LlamaVLM.*          # VLM captioning (llama.cpp)
│   │   └── ONNXInference.*     # ONNX helper utilities
│   │
│   └── ui/                     # User Interface
│       ├── MainWindow.*        # Main application window
│       ├── ImageViewer.*       # Image display widget
│       ├── ThumbnailGrid.*     # Gallery view
│       ├── AnalysisPanel.*     # AI analysis controls
│       ├── MetadataPanel.*     # Metadata display
│       ├── FilterPanel.*       # Search/filter UI
│       ├── NotificationManager.* # Toast notifications
│       └── DarkTheme.h         # Professional dark theme
│
├── tests/                      # Unit tests (Google Test)
│   ├── main.cpp                # Test runner
│   ├── test_clip_analyzer.cpp
│   ├── test_llama_vlm.cpp
│   ├── test_exiftool_daemon.cpp
│   ├── test_metadata_writer.cpp
│   └── ...
│
├── resources/                  # Application resources
│   ├── Info.plist             # macOS bundle info
│   └── resources.qrc          # Qt resources
│
└── thirdparty/                 # External dependencies
    ├── llama.cpp/             # VLM inference library
    └── exiftool-cpp.tar.gz    # ExifTool integration
```

---

## 🤝 Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Run tests (`cd build && ./PhotoGuruTests`)
5. Commit with descriptive message (`git commit -m 'feat: Add amazing feature'`)
6. Push to branch (`git push origin feature/amazing-feature`)
7. Submit a pull request

### Development Setup

```bash
# Install dependencies
brew install qt@6 cmake onnxruntime

# Download models
./scripts/download_models.sh all

# Build with tests
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(sysctl -n hw.ncpu)

# Run tests
./PhotoGuruTests
```

### Code Style

- C++17 standard
- Follow Qt naming conventions (camelCase for methods, m_ prefix for members)
- Use smart pointers (std::unique_ptr, std::shared_ptr)
- Document public APIs with Doxygen-style comments
- Keep functions focused and under 50 lines when possible

---

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **Qt Framework**: Cross-platform UI framework
- **ONNX Runtime**: High-performance ML inference
- **llama.cpp**: Efficient LLM inference
- **LibRaw**: RAW image decoding
- **Qwen Team**: Qwen3-VL vision-language model
- **OpenAI**: CLIP vision model
- **ExifTool**: Metadata manipulation

---

## 📞 Support

For issues, questions, or feature requests:
- **GitHub Issues**: https://github.com/wsmontes/photoguru/issues
- **Discussions**: https://github.com/wsmontes/photoguru/discussions

---

## 🗺️ Roadmap

### Completed ✅
- [x] C++ migration from Python
- [x] Native CLIP integration (ONNX)
- [x] Native VLM integration (llama.cpp)
- [x] Comprehensive logging system
- [x] ExifTool daemon for metadata writing
- [x] 5 AI analysis functions
- [x] Caption display with copy-to-clipboard

### In Progress 🚧
- [ ] Fix remaining 6 test failures
- [ ] Optimize VLM loading time
- [ ] Add progress indicators for long operations
- [ ] Batch VLM captioning

### Planned 📋
- [ ] Semantic search (CLIP-based)
- [ ] Duplicate image management UI
- [ ] Burst mode best shot selection
- [ ] Non-destructive editing (curves, levels)
- [ ] Cloud sync support
- [ ] Plugin system for extensibility
- [ ] Video support
- [ ] Windows/Linux support

---

**Built with ❤️ for photographers who demand professional tools with cutting-edge AI**
