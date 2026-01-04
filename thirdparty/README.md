# pybind11 Submodule

This directory should contain the pybind11 library as a git submodule or direct copy.

## Setup via Git Submodule (Recommended)

```bash
cd /path/to/photoguru-viewer
git submodule add https://github.com/pybind/pybind11.git thirdparty/pybind11
git submodule update --init --recursive
```

## Alternative: Direct Download

If you don't want to use git submodules:

```bash
cd thirdparty
git clone https://github.com/pybind/pybind11.git
cd pybind11
git checkout v2.11.1  # or latest stable version
```

## System Installation

Alternatively, use system-installed pybind11 and modify CMakeLists.txt to use:

```cmake
find_package(pybind11 REQUIRED)
```

instead of:

```cmake
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/pybind11)
```
