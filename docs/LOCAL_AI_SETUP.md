# 🔧 Local AI Setup Instructions

## Pré-requisitos

```bash
# macOS
brew install onnxruntime
brew install wget

# Verificar instalação
pkg-config --cflags --libs onnxruntime
```

## Download dos Modelos

```bash
cd /Users/wagnermontes/Documents/GitHub/photoguru
./scripts/download_models.sh
```

Isso vai baixar (~6GB total):
- ✅ CLIP ViT-B/32 (ONNX) - 170MB - Embeddings de imagens
- ✅ CLIP Text (ONNX) - 250MB - Text embeddings (opcional)
- ✅ LLaVA 7B (GGUF) - 4.1GB - LLM Vision para descrições
- ✅ LLaVA Projector (GGUF) - 600MB - Vision encoder
- ✅ MobileVLM 1.7B (GGUF) - 1.2GB - Alternativa mais rápida

## Compilação

```bash
# Limpar build anterior
rm -rf build

# Recompilar com suporte a ONNX
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8
```

## Uso

### CLIP Embeddings (Instantâneo)
```cpp
CLIPAnalyzer clip;
clip.initialize("models/clip_vision.onnx");

auto embedding = clip.computeEmbedding(image);
// embedding = vector<float> com 512 dimensões
```

### LLM Analysis (Lento, ~5s)
```cpp
LlamaAnalyzer llama;
llama.initialize("models/mobilevlm-1.7b-q4.gguf");

auto result = llama.analyzeImage(imagePath);
// result.title, result.description, result.keywords
```

## Troubleshooting

### ONNX Runtime not found
```bash
# Instalar manualmente
brew install onnxruntime

# Ou apontar para instalação customizada
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
```

### Modelos não encontrados
```bash
# Verificar se existem
ls -lh models/

# Re-baixar se necessário
rm -rf models/*.onnx models/*.gguf
./scripts/download_models.sh
```

### Performance ruim
```bash
# Verificar se CoreML está sendo usado (macOS)
# Logs devem mostrar: "Using CoreML execution provider"

# Verificar se Metal está habilitado para llama.cpp
# Build com: make LLAMA_METAL=1
```

## Desenvolvimento

### Adicionar novo modelo ONNX
1. Converter para ONNX (PyTorch → ONNX)
2. Salvar em `models/`
3. Criar classe wrapper herdando de `ONNXInference`
4. Implementar preprocessing específico

### Testar performance
```bash
# Benchmark CLIP
time ./build/PhotoGuruViewer --benchmark-clip models/clip_vision.onnx test.jpg

# Benchmark LLaVA
time ./build/PhotoGuruViewer --benchmark-llama models/mobilevlm-1.7b-q4.gguf test.jpg
```

## Referências

- [ONNX Runtime](https://onnxruntime.ai/)
- [llama.cpp](https://github.com/ggerganov/llama.cpp)
- [CLIP Paper](https://arxiv.org/abs/2103.00020)
- [LLaVA Paper](https://arxiv.org/abs/2304.08485)
