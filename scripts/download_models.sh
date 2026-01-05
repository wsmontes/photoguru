#!/bin/bash
# Download AI models for local inference
# Updated: Focus on CLIP only (LLM already available locally)

set -e

MODELS_DIR="$(cd "$(dirname "$0")/../models" && pwd)"
mkdir -p "$MODELS_DIR"

echo "📦 Downloading AI models to: $MODELS_DIR"
echo

# ============================================================================
# CLIP Vision Model (ONNX)
# ============================================================================
echo "1️⃣ Downloading CLIP ViT-B/32 (ONNX)..."
echo "   Source: LAION-trained model from Hugging Face"
echo "   Size: ~170MB"
echo ""

CLIP_URL="https://huggingface.co/rocm/CLIP-ViT-B-32-laion2B-s34B-b79K/resolve/main/CLIP-ViT-B-32-laion2B-s34B-b79K_visual.onnx"

if [ ! -f "$MODELS_DIR/clip-vit-base-patch32.onnx" ]; then
    if command -v curl &> /dev/null; then
        curl -L --progress-bar "$CLIP_URL" -o "$MODELS_DIR/clip-vit-base-patch32.onnx"
    elif command -v wget &> /dev/null; then
        wget --show-progress "$CLIP_URL" -O "$MODELS_DIR/clip-vit-base-patch32.onnx"
    else
        echo "❌ Error: Neither curl nor wget found"
        exit 1
    fi
    echo "   ✅ clip-vit-base-patch32.onnx downloaded"
else
    echo "   ✅ clip-vit-base-patch32.onnx (already exists)"
fi

# Verify file size
CLIP_SIZE=$(stat -f%z "$MODELS_DIR/clip-vit-base-patch32.onnx" 2>/dev/null || stat -c%s "$MODELS_DIR/clip-vit-base-patch32.onnx" 2>/dev/null)
CLIP_SIZE_MB=$((CLIP_SIZE / 1024 / 1024))

if [ "$CLIP_SIZE_MB" -lt 150 ]; then
    echo "   ⚠️  Warning: File size too small (${CLIP_SIZE_MB}MB), expected ~170MB"
    exit 1
fi

echo ""
echo "✅ Model downloaded successfully: ${CLIP_SIZE_MB}MB"
echo ""

# ============================================================================
# LLM Model Status
# ============================================================================
echo "📋 LLM Model Status:"
echo "   ✅ LLM model already available locally (confirmed by user)"
echo "   ⏸️  llama.cpp integration pending"
echo ""

# ============================================================================
# Summary
# ============================================================================
echo "✅ Setup complete!"
echo ""
echo "📊 Summary:"
echo "   CLIP ViT-B/32: ${CLIP_SIZE_MB}MB ✅"
echo "   LLM Model: Available locally ✅"
echo ""
echo "📍 Model location: $MODELS_DIR"
echo ""
echo "🧪 Next Steps:"
echo "   1. Test CLIP inference:"
echo "      cd build && ./PhotoGuruTests --gtest_filter='CLIPAnalyzerTest.*'"
echo ""
echo "   2. Enable real image tests:"
echo "      ./PhotoGuruTests --gtest_filter='*DISABLED*' --gtest_also_run_disabled_tests"
echo ""
echo "   3. Integrate llama.cpp for LLM descriptions"
echo ""
