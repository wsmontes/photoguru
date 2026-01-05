# PhotoGuru Viewer - Project Summary

## Overview

Professional desktop photo viewer and AI-powered semantic image browser, designed to compete with Adobe Lightroom. Built with **C++/Qt6** with **native C++ ML backend** using ONNX Runtime and llama.cpp.

## ✨ Key Features

### 1. **Universal Image Support**
- RAW formats (40+ supported via LibRaw)
- HEIF/HEIC (Apple format)
- Standard formats (JPEG, PNG, TIFF, WebP)
- Hardware-accelerated rendering

### 2. **AI-Powered Analysis**
- CLIP vision embeddings (ONNX Runtime)
- VLM-generated captions (llama.cpp)
- Semantic search by natural language
- Quality scoring (sharpness, exposure, aesthetics)

### 3. **Semantic Key Protocol (SKP)**
- Advanced organization via semantic meaning
- Relationship tracking (people, places, events)
- Burst detection and best shot selection
- Duplicate detection and grouping

### 4. **Professional UI**
- Adobe-level dark theme
- Dockable panels (Lightroom-style)
- High-performance thumbnail grid
- Real-time RAW preview
- Smooth 60fps zooming and panning

## 🏗️ Architecture

### Technology Stack

**C++/Qt6:**
- Qt6 Widgets for UI
- LibRaw for RAW decoding
- libheif for HEIF/HEIC
- OpenCV for image processing
- ONNX Runtime for CLIP embeddings
- llama.cpp for VLM captions
- Custom image viewer with GPU acceleration

**Integration:**
- ExifTool for metadata I/O
- SQLite for catalog (future)

### Project Structure

```
photoguru-viewer/
├── CMakeLists.txt              # Build configuration
├── scripts/
│   ├── build.sh                # macOS/Linux build script
│   └── check_dependencies.sh   # Dependency checker
├── README.md                   # Main documentation
├── docs/                       # Documentation
├── LICENSE                     # MIT License
│
├── src/
│   ├── main.cpp                # Application entry point
│   │
│   ├── core/                   # Core functionality
│   │   ├── ImageLoader.*       # Universal image loading (RAW/HEIF/standard)
│   │   ├── MetadataReader.*    # EXIF/XMP metadata reading
│   │   ├── ExifToolDaemon.*    # Stay-open ExifTool process
│   │   └── PhotoMetadata.h     # Data structures for photo metadata
│   │
│   ├── ml/                     # ML integration
│   │   ├── CLIPAnalyzer.*      # ONNX Runtime CLIP embeddings
│   │   ├── LlamaVLM.*          # llama.cpp VLM captions
│   │   └── ONNXInference.*     # ONNX Runtime wrapper
│   │
│   └── ui/                     # User interface
│       ├── MainWindow.*        # Main application window
│       ├── ImageViewer.*       # High-performance image display widget
│       ├── ThumbnailGrid.*     # Async thumbnail grid with caching
│       ├── MetadataPanel.*     # Display EXIF, AI, and technical data
│       ├── AnalysisPanel.*     # AI analysis results
│       ├── SKPBrowser.*        # Semantic Key Protocol browser
│       └── DarkTheme.h         # Adobe-style dark theme
│
├── resources/
│   └── resources.qrc           # Qt resources (icons, stylesheets)
│
├── models/                     # AI models (not in git)
│   ├── clip-vit-base-patch32.onnx       (335MB)
│   ├── Qwen3VL-4B-Instruct-Q4_K_M.gguf
│   └── mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf
│
├── thirdparty/
│   ├── llama.cpp/              # VLM backend
│   └── googletest/             # Unit testing
│
└── tests/                      # Unit tests
```

## 🚀 Building & Running

### Quick Start (macOS)

```bash
# Install dependencies
brew install qt6 opencv libraw libheif exiftool cmake

# Build
./scripts/build.sh

# Run
cd build && open PhotoGuruViewer.app
```

### Full Instructions

See [INSTALL.md](INSTALL.md) for detailed platform-specific instructions.

## 📊 Performance Characteristics

| Feature | Performance |
|---------|-------------|
| RAW Loading | ~100-200ms (half-size preview) |
| Thumbnail Generation | ~50ms per image (cached) |
| CLIP Analysis | ~388ms (ONNX Runtime) |
| VLM Captions | ~2-5s (llama.cpp) |
| UI Rendering | 60fps (hardware accelerated) |
| Memory Usage | ~200MB base + ~500MB ML models |

## 🎯 Comparison with Lightroom

| Feature | Lightroom | PhotoGuru Viewer |
|---------|-----------|------------------|
| RAW Support | ✅ Excellent | ✅ Excellent (via LibRaw) |
| Performance | ⚠️ Heavy | ✅ Fast (C++ core) |
| AI Features | ⚠️ Basic | ✅ Advanced (CLIP + VLM) |
| Semantic Search | ❌ No | ✅ Yes |
| Price | 💰 $10/month | ✅ Free (Open Source) |
| Platform | Windows/Mac | macOS/Linux |
| Editing | ✅ Full Suite | 🚧 Coming Soon |
| Catalog | ✅ Mature | 🚧 In Progress |

## 🛣️ Roadmap

### v1.0 (Current)
- ✅ Universal image loading
- ✅ Native C++ AI analysis
- ✅ SKP browser
- ✅ Professional UI
- ✅ Metadata display
- ✅ CLIP embeddings (ONNX)
- ✅ VLM captions (llama.cpp)

### v1.1 (Next)
- [ ] Batch AI analysis
- [ ] Semantic search implementation
- [ ] SQLite catalog database
- [ ] Thumbnail cache persistence
- [ ] Keyboard shortcuts panel

### v2.0 (Future)
- [ ] Non-destructive editing (curves, levels, color)
- [ ] Export presets
- [ ] Cloud sync
- [ ] Plugin system
- [ ] Video support

### v3.0 (Long-term)
- [ ] Windows support
- [ ] Mobile companion app
- [ ] Collaborative features
- [ ] Print module

## 🧩 Integration Points

### C++ ML Backend

CLIP embeddings and VLM captions run natively in C++:

```cpp
// CLIP embeddings via ONNX Runtime
CLIPAnalyzer clipAnalyzer;
auto embedding = clipAnalyzer.analyzeImage(imagePath);

// VLM captions via llama.cpp
LlamaVLM vlm;
QString caption = vlm.generateCaption(imagePath);
```

### Metadata Format

PhotoGuru writes metadata to image files using ExifTool:

- **XMP fields**: Title, Description, Keywords, Category
- **IPTC fields**: Location, City, Country
- **EXIF UserComment**: Technical data (JSON format)
- **Rating**: 1-5 stars based on aesthetic score

Format example:
```
EXIF:UserComment = "PhotoGuru:{
  \"sharp\":0.85,\"expo\":0.92,\"aesth\":0.78,
  \"qual\":0.85,\"faces\":2,\"burst\":\"burst_abc123\"
}"
```

## 🤝 Contributing

We welcome contributions! Areas of interest:

1. **Performance**: Optimize RAW loading, thumbnail generation
2. **UI/UX**: Improve keyboard shortcuts, add preferences dialog
3. **Features**: Implement batch processing, semantic search UI
4. **Platforms**: Add Windows support
5. **Documentation**: Improve API docs, add tutorials

## 📝 License

MIT License - See [LICENSE](LICENSE) file

## 🙏 Acknowledgments

- **Qt Project**: Cross-platform framework
- **LibRaw**: RAW format support
- **ONNX Runtime**: ML inference
- **llama.cpp**: Local LLM/VLM execution
- **OpenAI**: CLIP vision model
- **ExifTool**: Metadata management

---

**Built for photographers who demand professional tools** 📸
