# Third-party Libraries

This directory contains third-party dependencies used by PhotoGuru:

## llama.cpp

Vision Language Model backend for image captioning.

**Setup:**
```bash
git clone https://github.com/ggerganov/llama.cpp thirdparty/llama.cpp
cd thirdparty/llama.cpp
mkdir build && cd build
cmake .. -DGGML_METAL=ON
make -j4
```

## googletest

Unit testing framework (automatically downloaded during build).

No manual setup required - CMake handles this automatically.

## Models

AI models are stored in the `models/` directory at the project root:
- `clip-vit-base-patch32.onnx` - CLIP vision embeddings (335MB)
- `Qwen3VL-4B-Instruct-Q4_K_M.gguf` - VLM for image captioning
- `mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf` - Multimodal projector

Download using:
```bash
./scripts/download_models.sh
```
