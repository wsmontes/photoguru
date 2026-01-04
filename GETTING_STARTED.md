# Getting Started with PhotoGuru Viewer

Welcome! This guide will help you build and run PhotoGuru Viewer in under 10 minutes.

## 🚀 Quick Start (macOS)

### Step 1: Install Dependencies (5 minutes)

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install all system dependencies in one command
brew install qt6 opencv libraw libheif exiftool cmake python@3.11

# Install Python ML packages
pip3 install torch torchvision
pip3 install git+https://github.com/openai/CLIP.git
pip3 install pillow pillow-heif opencv-python numpy sentence-transformers pyiqa
```

### Step 2: Verify Dependencies

```bash
cd /path/to/Photoguru-viewer
./check_dependencies.sh
```

You should see all green checkmarks. If not, install any missing dependencies.

### Step 3: Build (2 minutes)

```bash
./build.sh
```

This will:
1. Download pybind11 if needed
2. Configure CMake
3. Compile the application

### Step 4: Run!

```bash
cd build
./PhotoGuruViewer
```

Or open a specific folder:
```bash
./PhotoGuruViewer ~/Pictures
```

---

## 🎯 First Use Walkthrough

### 1. Open Images

**Option A: Drag & Drop**
- Drag a folder containing photos onto the window

**Option B: File Menu**
- `File > Open Directory` (Cmd+Shift+O)
- Select your photo folder

**Option C: Command Line**
```bash
./PhotoGuruViewer /path/to/photos
```

### 2. Navigate Images

- **Arrow Keys**: Left/Right to navigate
- **Mouse Wheel**: Zoom in/out
- **Click & Drag**: Pan the image
- **F**: Fit to window
- **Cmd+0**: Actual size (100%)

### 3. View Metadata

The right panel shows:
- **Metadata Tab**: EXIF, camera settings, location
- **Semantic Keys Tab**: AI-generated semantic keys

Click images in the bottom thumbnail grid to switch between them.

### 4. Run AI Analysis (Optional)

If you've installed the Python dependencies:

1. Select an image
2. Press **Cmd+A** or click the 🤖 button
3. Wait for analysis (2-5 seconds)
4. View results in the Metadata panel

The AI will generate:
- Title
- Description  
- Keywords
- Quality scores
- Semantic keys

### 5. Search (Coming Soon)

Press **Cmd+F** for semantic search:
- "sunset on the beach"
- "people smiling"
- "indoor photos with cats"

---

## 🎨 UI Overview

```
┌─────────────────────────────────────────────────────────────┐
│  File  View  Navigate  AI  Help          [Toolbar]          │
├─────────────────────────────────────────────┬───────────────┤
│                                             │   Metadata    │
│                                             │  ┌──────────┐ │
│                                             │  │ Title    │ │
│           Main Image Viewer                 │  │ Desc     │ │
│         (Zoom, Pan, Fullscreen)             │  │ Keywords │ │
│                                             │  │ EXIF     │ │
│                                             │  │ Quality  │ │
│                                             │  └──────────┘ │
│                                             │  [Semantic   │
│                                             │   Keys Tab]  │
├─────────────────────────────────────────────┴───────────────┤
│  [Thumbnail Grid - Bottom Panel]                            │
│  [img] [img] [img] [img] [img] [img] [img] [img] [img]     │
└─────────────────────────────────────────────────────────────┘
```

### Panels (Lightroom-style)

- **Center**: Main image viewer
- **Right**: Metadata & Semantic Keys (tabbed)
- **Bottom**: Thumbnail grid / Library view

All panels are **dockable** - drag them to rearrange!

---

## ⌨️ Keyboard Shortcuts

### Navigation
- `Left/Right Arrow`: Previous/Next image
- `Home/End`: First/Last image

### View
- `F`: Fit to window
- `Cmd+0`: Actual size (100%)
- `Cmd++`: Zoom in
- `Cmd+-`: Zoom out
- `F11`: Fullscreen
- `Esc`: Exit fullscreen

### Files
- `Cmd+O`: Open files
- `Cmd+Shift+O`: Open directory
- `Cmd+Q`: Quit

### AI Features
- `Cmd+A`: Analyze current image
- `Cmd+F`: Semantic search

---

## 🔧 Configuration

### Settings Location

- **macOS**: `~/Library/Preferences/PhotoGuru/Viewer.plist`
- **Linux**: `~/.config/PhotoGuru/Viewer.conf`

Settings auto-save:
- Window size and position
- Panel layout
- Last opened directory

### Python Backend

The viewer looks for `agent_v2.py` in:
1. Same directory as executable
2. Parent directory
3. Build directory

If not found, AI features will be disabled but the viewer still works.

---

## 📁 Supported Formats

### RAW Formats ✅
Canon CR2, CR3 | Nikon NEF, NRW | Sony ARW, SRF, SR2 | 
Fuji RAF | Olympus ORF | Panasonic RW2 | Pentax PEF |
Adobe DNG | And 30+ more...

### Standard Formats ✅
JPEG, PNG, TIFF, WebP

### Modern Formats ✅
HEIF, HEIC (Apple)

### Coming Soon 🚧
Video (MP4, MOV)

---

## 🐛 Troubleshooting

### "No images found"
- Ensure the directory contains supported image formats
- Check file permissions

### "Python bridge not initialized"
- Run: `python3 -c "import torch, clip; print('OK')"`
- If error, reinstall: `pip3 install -r requirements.txt`

### "LibRaw error"
- Some RAW files may not be supported
- Try updating LibRaw: `brew upgrade libraw`

### Slow performance
- Close other applications
- Enable GPU acceleration in preferences (coming soon)
- For huge libraries (50k+ images), increase thumbnail cache

### Crash on startup
- Check dependencies: `./check_dependencies.sh`
- Look for errors in terminal output
- Try debug build: `./build.sh debug`

---

## 📚 Next Steps

1. **Read the full README**: [README.md](README.md)
2. **Explore AI features**: Run analysis on your photos
3. **Learn about SKP**: [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)
4. **Customize**: Modify source code and rebuild
5. **Contribute**: Submit issues and PRs on GitHub

---

## 💡 Pro Tips

1. **Keyboard Shortcuts**: Learn them! They make navigation 10x faster
2. **RAW + JPEG**: The viewer prefers RAW when both exist
3. **Metadata**: AI-generated metadata is written to image files (non-destructive)
4. **Search**: Use semantic search for natural language queries (coming soon)
5. **Batch**: Select multiple images for batch operations (coming soon)

---

## 🆘 Getting Help

- **Documentation**: Check README.md and INSTALL.md
- **Dependencies**: Run `./check_dependencies.sh`
- **Issues**: https://github.com/yourusername/photoguru-viewer/issues
- **Discussions**: https://github.com/yourusername/photoguru-viewer/discussions

---

**Enjoy your new photo viewer! 📸**

*Built with ❤️ for photographers*
