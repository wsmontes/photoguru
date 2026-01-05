#include <gtest/gtest.h>
#include "ml/LlamaVLM.h"
#include <QImage>
#include <QDebug>

using namespace PhotoGuru;

class LlamaVLMTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if models exist
        modelPath = "models/Qwen3VL-4B-Instruct-Q4_K_M.gguf";
        mmprojPath = "models/mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf";
        testImagePath = "Test_10/IMG_0001.HEIC";
    }
    
    QString modelPath;
    QString mmprojPath;
    QString testImagePath;
};

TEST_F(LlamaVLMTest, DISABLED_InitializeModel) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = modelPath;
    config.mmprojPath = mmprojPath;
    config.nGPULayers = 5; // Optimized for Mac M4
    config.maxTokens = 50;
    
    bool success = vlm.initialize(config);
    EXPECT_TRUE(success);
    
    if (!success) {
        qDebug() << "Initialization failed:" << vlm.lastError();
    }
}

TEST_F(LlamaVLMTest, DISABLED_GenerateCaptionFromRealImage) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = modelPath;
    config.mmprojPath = mmprojPath;
    config.nGPULayers = 5;
    config.maxTokens = 50;
    
    ASSERT_TRUE(vlm.initialize(config));
    
    // Load real image
    QImage image(testImagePath);
    ASSERT_FALSE(image.isNull()) << "Failed to load test image";
    
    qDebug() << "Image loaded:" << image.size() << "format:" << image.format();
    
    // Generate caption
    auto result = vlm.generateCaption(image);
    
    ASSERT_TRUE(result.has_value());
    QString caption = result.value();
    
    qDebug() << "Generated caption:" << caption;
    
    // Validate caption
    EXPECT_FALSE(caption.isEmpty());
    EXPECT_GT(caption.length(), 10); // At least some meaningful text
}

TEST_F(LlamaVLMTest, DISABLED_AnswerQuestionAboutImage) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = modelPath;
    config.mmprojPath = mmprojPath;
    config.nGPULayers = 5;
    config.maxTokens = 30;
    
    ASSERT_TRUE(vlm.initialize(config));
    
    QImage image(testImagePath);
    ASSERT_FALSE(image.isNull());
    
    // Ask a question
    auto result = vlm.answerQuestion(image, "What is the person doing?");
    
    ASSERT_TRUE(result.has_value());
    QString answer = result.value();
    
    qDebug() << "Question: What is the person doing?";
    qDebug() << "Answer:" << answer;
    
    EXPECT_FALSE(answer.isEmpty());
}

TEST_F(LlamaVLMTest, DISABLED_DetailedAnalysis) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = modelPath;
    config.mmprojPath = mmprojPath;
    config.nGPULayers = 5;
    config.maxTokens = 100;
    
    ASSERT_TRUE(vlm.initialize(config));
    
    QImage image(testImagePath);
    ASSERT_FALSE(image.isNull());
    
    // Get detailed analysis
    auto result = vlm.analyzeImage(image, true); // with keywords
    
    ASSERT_TRUE(result.has_value());
    QString analysis = result.value();
    
    qDebug() << "Detailed analysis:" << analysis;
    
    EXPECT_FALSE(analysis.isEmpty());
    EXPECT_GT(analysis.length(), 50); // Should be detailed
}

TEST_F(LlamaVLMTest, DISABLED_AutomaticImageResizing) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = modelPath;
    config.mmprojPath = mmprojPath;
    config.nGPULayers = 5;
    config.maxTokens = 50;
    
    ASSERT_TRUE(vlm.initialize(config));
    
    // Load full resolution image (4032x3024)
    QImage largeImage(testImagePath);
    ASSERT_FALSE(largeImage.isNull());
    
    qDebug() << "Large image size:" << largeImage.size();
    
    // Should automatically resize to 512px max
    auto result = vlm.generateCaption(largeImage);
    
    ASSERT_TRUE(result.has_value());
    qDebug() << "Caption from large image:" << result.value();
    
    // Verify no OOM error
    EXPECT_FALSE(result.value().isEmpty());
}

TEST_F(LlamaVLMTest, ErrorHandlingMissingModel) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = "nonexistent_model.gguf";
    config.mmprojPath = mmprojPath;
    
    bool success = vlm.initialize(config);
    EXPECT_FALSE(success);
    EXPECT_FALSE(vlm.lastError().isEmpty());
    
    qDebug() << "Expected error:" << vlm.lastError();
}

TEST_F(LlamaVLMTest, ErrorHandlingNullImage) {
    LlamaVLM vlm;
    
    LlamaVLM::ModelConfig config;
    config.modelPath = modelPath;
    config.mmprojPath = mmprojPath;
    config.nGPULayers = 5;
    
    ASSERT_TRUE(vlm.initialize(config));
    
    QImage nullImage;
    auto result = vlm.generateCaption(nullImage);
    
    // Should handle gracefully
    EXPECT_FALSE(result.has_value());
}
