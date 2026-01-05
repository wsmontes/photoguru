#include <gtest/gtest.h>
#include "ml/ONNXInference.h"
#include <QImage>
#include <QDir>
#include <QDebug>
#include <QColor>

using namespace PhotoGuru;

class ONNXBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure model directory exists
        QDir modelDir("/Users/wagnermontes/Documents/GitHub/photoguru/models");
        ASSERT_TRUE(modelDir.exists());
    }
};

TEST_F(ONNXBasicTest, LoadMobileNetV2) {
    ONNXInference inference;
    bool success = inference.loadModel("/Users/wagnermontes/Documents/GitHub/photoguru/models/mobilenetv2-12.onnx");
    
    if (!success) {
        qDebug() << "Load error:" << inference.lastError();
    }
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(inference.isModelLoaded());
}

TEST_F(ONNXBasicTest, DISABLED_MobileNetV2Inference) {
    ONNXInference inference;
    ASSERT_TRUE(inference.loadModel("/Users/wagnermontes/Documents/GitHub/photoguru/models/mobilenetv2-12.onnx"));
    
    // Create test image (224x224 RGB)
    QImage testImage(224, 224, QImage::Format_RGB888);
    testImage.fill(QColor(128, 128, 128));  // Gray image
    
    // ImageNet normalization
    std::vector<float> mean = {0.485f, 0.456f, 0.406f};
    std::vector<float> std = {0.229f, 0.224f, 0.225f};
    
    auto inputTensor = inference.preprocessImage(testImage, mean, std);
    ASSERT_FALSE(inputTensor.empty());
    
    qDebug() << "[TEST] Running inference...";
    auto output = inference.runInference(inputTensor);
    
    if (!output.has_value()) {
        qDebug() << "[TEST] Inference failed:" << inference.lastError();
    }
    
    ASSERT_TRUE(output.has_value());
    EXPECT_GT(output.value().size(), 0);
    
    qDebug() << "[TEST] Output size:" << output.value().size();
}

TEST_F(ONNXBasicTest, DISABLED_LoadCLIPXenova) {
    ONNXInference inference;
    bool success = inference.loadModel("/Users/wagnermontes/Documents/GitHub/photoguru/models/clip-visual-xenova.onnx");
    
    if (!success) {
        qDebug() << "Load error:" << inference.lastError();
    }
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(inference.isModelLoaded());
}

TEST_F(ONNXBasicTest, DISABLED_CLIPXenovaInference) {
    ONNXInference inference;
    ASSERT_TRUE(inference.loadModel("/Users/wagnermontes/Documents/GitHub/photoguru/models/clip-visual-xenova.onnx"));
    
    // Create test image (224x224 RGB)
    QImage testImage(224, 224, QImage::Format_RGB888);
    testImage.fill(QColor(128, 128, 128));  // Gray image
    
    // CLIP normalization
    std::vector<float> mean = {0.48145466f, 0.4578275f, 0.40821073f};
    std::vector<float> std = {0.26862954f, 0.26130258f, 0.27577711f};
    
    auto inputTensor = inference.preprocessImage(testImage, mean, std);
    ASSERT_FALSE(inputTensor.empty());
    
    qDebug() << "[TEST] Running CLIP inference...";
    auto output = inference.runInference(inputTensor);
    
    if (!output.has_value()) {
        qDebug() << "[TEST] Inference failed:" << inference.lastError();
    }
    
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(output.value().size(), 512);  // CLIP embeddings are 512-dimensional
    
    qDebug() << "[TEST] CLIP output size:" << output.value().size();
}
