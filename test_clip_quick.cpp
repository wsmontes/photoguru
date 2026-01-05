#include "src/ml/CLIPAnalyzer.h"
#include <QImage>
#include <QDebug>

using namespace PhotoGuru;

int main() {
    qDebug() << "Testing CLIP model...";
    
    CLIPAnalyzer clip;
    bool loaded = clip.initialize("models/clip-vit-base-patch32.onnx");
    
    if (!loaded) {
        qDebug() << "Failed to load model:" << clip.lastError();
        return 1;
    }
    
    qDebug() << "✅ Model loaded successfully!";
    
    // Create a simple test image
    QImage testImage(224, 224, QImage::Format_RGB888);
    testImage.fill(QColor(128, 128, 128));
    
    qDebug() << "Computing embedding...";
    auto result = clip.computeEmbedding(testImage);
    
    if (!result.has_value()) {
        qDebug() << "Failed to compute embedding:" << clip.lastError();
        return 1;
    }
    
    auto embedding = result.value();
    qDebug() << "✅ Embedding computed!";
    qDebug() << "   Dimensions:" << embedding.size();
    
    // Check normalization
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    qDebug() << "   L2 Norm:" << norm;
    
    if (embedding.size() == 512 && std::abs(norm - 1.0f) < 0.01f) {
        qDebug() << "\n🎉 SUCCESS! CLIP is working correctly!";
        return 0;
    } else {
        qDebug() << "\n❌ FAILED: Invalid embedding";
        return 1;
    }
}
