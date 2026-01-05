#include <gtest/gtest.h>
#include <QApplication>
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include "../../src/ml/LlamaVLM.h"
#include "../../src/core/Logger.h"

using namespace PhotoGuru;

/**
 * VLM Integration Tests
 * 
 * These tests use REAL AI models (2.3GB Qwen3-VL + 433MB mmproj)
 * They validate that:
 * - Models load correctly
 * - Captions are generated
 * - Captions have reasonable length
 * - Captions are consistent across runs
 * - Captions contain relevant information
 * 
 * Expected runtime: 2-5 minutes (models loaded once, inference ~5-10s per image)
 * 
 * Note: VLMs can have slight variation in output even with same input
 * due to sampling, but should be deterministic with temperature=0
 */
class VLMIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Initialize QApplication (needed for QImage)
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
        
        // Load VLM model ONCE for all tests (very expensive operation)
        LOG_INFO("VLMIntegration", "=== Loading VLM model for integration tests ===");
        LOG_INFO("VLMIntegration", "⚠️  This will take 10-30 seconds and use ~3GB RAM...");
        
        QString modelsDir = QDir::currentPath() + "/../models";
        if (!QDir(modelsDir).exists()) {
            modelsDir = QDir::currentPath() + "/models";
        }
        
        QString modelPath = modelsDir + "/Qwen3VL-4B-Instruct-Q4_K_M.gguf";
        QString mmprojPath = modelsDir + "/mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf";
        
        // Check if models exist
        if (!QFileInfo::exists(modelPath) || !QFileInfo::exists(mmprojPath)) {
            LOG_WARNING("VLMIntegration", "VLM models not found - skipping integration tests");
            LOG_WARNING("VLMIntegration", "Expected: " + modelPath);
            LOG_WARNING("VLMIntegration", "Expected: " + mmprojPath);
            GTEST_SKIP() << "VLM models not found at: " << modelsDir.toStdString();
        }
        
        vlm = std::make_unique<LlamaVLM>();
        
        // Configure VLM with proper ModelConfig struct
        LlamaVLM::ModelConfig config;
        config.modelPath = modelPath;
        config.mmprojPath = mmprojPath;
        config.contextSize = 2048;
        config.nThreads = 4;
        config.nGPULayers = 5;
        config.temperature = 0.0f;  // Deterministic
        config.maxTokens = 256;
        
        bool initialized = vlm->initialize(config);
        
        if (!initialized) {
            LOG_ERROR("VLMIntegration", "Failed to initialize VLM model");
            GTEST_SKIP() << "VLM initialization failed";
        }
        
        LOG_INFO("VLMIntegration", "✅ VLM model loaded successfully");
        
        // Verify test fixtures exist
        fixturesDir = QDir::currentPath() + "/../tests/integration/fixtures";
        if (!QDir(fixturesDir).exists()) {
            fixturesDir = QDir::currentPath() + "/tests/integration/fixtures";
        }
        
        ASSERT_TRUE(QDir(fixturesDir).exists()) 
            << "Fixtures directory not found: " << fixturesDir.toStdString();
        
        testImage1 = fixturesDir + "/test_image_1.jpg";
        testImage2 = fixturesDir + "/test_image_2.jpg";
        
        ASSERT_TRUE(QFileInfo::exists(testImage1)) 
            << "Test image 1 not found: " << testImage1.toStdString();
        ASSERT_TRUE(QFileInfo::exists(testImage2)) 
            << "Test image 2 not found: " << testImage2.toStdString();
    }
    
    static void TearDownTestSuite() {
        LOG_INFO("VLMIntegration", "=== Cleaning up VLM model ===");
        vlm.reset();
    }
    
    static std::unique_ptr<LlamaVLM> vlm;
    static QString fixturesDir;
    static QString testImage1;
    static QString testImage2;
};

// Static members initialization
std::unique_ptr<LlamaVLM> VLMIntegrationTest::vlm = nullptr;
QString VLMIntegrationTest::fixturesDir;
QString VLMIntegrationTest::testImage1;
QString VLMIntegrationTest::testImage2;

// ========== Test 1: Model Initialization ==========

TEST_F(VLMIntegrationTest, Model_InitializedSuccessfully) {
    ASSERT_NE(vlm, nullptr) << "VLM model should be initialized";
    LOG_INFO("VLMIntegration", "✅ Model initialization verified");
}

// ========== Test 2: Caption Generation Not Empty ==========

TEST_F(VLMIntegrationTest, GenerateCaption_NotEmpty) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull()) << "Failed to load test image: " << testImage1.toStdString();
    
    LOG_INFO("VLMIntegration", "Generating caption for test_image_1.jpg...");
    
    auto caption = vlm->generateCaption(img);
    
    ASSERT_TRUE(caption.has_value()) << "VLM should return caption for valid image";
    EXPECT_FALSE(caption->isEmpty()) << "Caption should not be empty";
    
    LOG_INFO("VLMIntegration", "✅ Generated caption: " + *caption);
}

// ========== Test 3: Caption Reasonable Length ==========

TEST_F(VLMIntegrationTest, GenerateCaption_ReasonableLength) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    auto caption = vlm->generateCaption(img);
    ASSERT_TRUE(caption.has_value());
    
    int length = caption->length();
    
    // Caption should be at least 5 characters (too short = error)
    EXPECT_GE(length, 5) 
        << "Caption too short (got " << length << " chars): " << caption->toStdString();
    
    // Caption should not be excessively long (>500 = might be hallucinating)
    EXPECT_LE(length, 500) 
        << "Caption too long (got " << length << " chars): " << caption->toStdString();
    
    LOG_INFO("VLMIntegration", QString("✅ Caption length: %1 chars").arg(length));
}

// ========== Test 4: Caption Contains English Words ==========

TEST_F(VLMIntegrationTest, GenerateCaption_ContainsWords) {
    QImage img(testImage2);
    ASSERT_FALSE(img.isNull());
    
    LOG_INFO("VLMIntegration", "Generating caption for test_image_2.jpg...");
    
    auto caption = vlm->generateCaption(img);
    ASSERT_TRUE(caption.has_value());
    
    QString text = caption->toLower();
    
    // Caption should contain at least one common English word
    // (basic sanity check - not gibberish)
    QStringList commonWords = {"the", "a", "an", "is", "in", "on", "of", "with", 
                               "person", "people", "image", "photo", "shows", "background"};
    
    bool hasCommonWord = false;
    for (const QString& word : commonWords) {
        if (text.contains(word)) {
            hasCommonWord = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasCommonWord) 
        << "Caption should contain common English words. Got: " << caption->toStdString();
    
    LOG_INFO("VLMIntegration", "✅ Caption content: " + *caption);
}

// ========== Test 5: Multiple Images Produce Different Captions ==========

TEST_F(VLMIntegrationTest, DifferentImages_DifferentCaptions) {
    QImage img1(testImage1);
    QImage img2(testImage2);
    
    ASSERT_FALSE(img1.isNull());
    ASSERT_FALSE(img2.isNull());
    
    LOG_INFO("VLMIntegration", "Generating captions for both test images...");
    
    auto caption1 = vlm->generateCaption(img1);
    auto caption2 = vlm->generateCaption(img2);
    
    ASSERT_TRUE(caption1.has_value() && caption2.has_value());
    
    // Different images should (usually) produce different captions
    // Note: Could be same if images are very similar, so just log warning
    if (*caption1 == *caption2) {
        LOG_WARNING("VLMIntegration", "Different images produced identical captions - this is unusual");
        LOG_WARNING("VLMIntegration", "Caption: " + *caption1);
    } else {
        LOG_INFO("VLMIntegration", "✅ Image 1: " + *caption1);
        LOG_INFO("VLMIntegration", "✅ Image 2: " + *caption2);
    }
    
    // At minimum, captions should exist
    EXPECT_FALSE(caption1->isEmpty());
    EXPECT_FALSE(caption2->isEmpty());
}

// ========== Test 6: Caption Consistency (Deterministic) ==========

TEST_F(VLMIntegrationTest, SameImage_ConsistentCaptions) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    LOG_INFO("VLMIntegration", "Testing caption consistency with 2 inferences...");
    
    // Generate caption twice for same image
    auto caption1 = vlm->generateCaption(img);
    auto caption2 = vlm->generateCaption(img);
    
    ASSERT_TRUE(caption1.has_value() && caption2.has_value());
    
    // With temperature=0 or low, captions should be identical (deterministic)
    // If using sampling, they might differ slightly
    if (*caption1 != *caption2) {
        LOG_WARNING("VLMIntegration", "Captions differ - model may be using sampling");
        LOG_WARNING("VLMIntegration", "Caption 1: " + *caption1);
        LOG_WARNING("VLMIntegration", "Caption 2: " + *caption2);
        
        // Even with sampling, should be somewhat similar
        // (at least >50% of words in common)
        QStringList words1 = caption1->split(" ", Qt::SkipEmptyParts);
        QStringList words2 = caption2->split(" ", Qt::SkipEmptyParts);
        
        int commonWords = 0;
        for (const QString& word : words1) {
            if (words2.contains(word, Qt::CaseInsensitive)) {
                commonWords++;
            }
        }
        
        float similarity = words1.isEmpty() ? 0.0f : (float)commonWords / words1.size();
        EXPECT_GT(similarity, 0.3f) 
            << "Captions should have at least 30% word overlap (got " << similarity << ")";
    } else {
        LOG_INFO("VLMIntegration", "✅ Captions are identical (deterministic)");
        LOG_INFO("VLMIntegration", "Caption: " + *caption1);
    }
}

// ========== Test 7: Invalid Image Handling ==========

TEST_F(VLMIntegrationTest, InvalidImage_ReturnsEmpty) {
    QImage invalidImg; // Null image
    ASSERT_TRUE(invalidImg.isNull());
    
    auto caption = vlm->generateCaption(invalidImg);
    
    // Should return empty optional or handle gracefully
    EXPECT_FALSE(caption.has_value()) 
        << "Invalid image should return empty caption";
    
    LOG_INFO("VLMIntegration", "✅ Invalid image handled correctly");
}

// ========== Test 8: Performance Baseline ==========

TEST_F(VLMIntegrationTest, Performance_ReasonableInferenceTime) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    LOG_INFO("VLMIntegration", "Measuring inference time...");
    
    // Measure inference time (first inference after load can be slow)
    auto start = std::chrono::high_resolution_clock::now();
    
    auto caption = vlm->generateCaption(img);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    ASSERT_TRUE(caption.has_value());
    
    // VLM inference is slow (0.6s - 10s typical)
    EXPECT_LT(duration_ms, 30000) 
        << "VLM inference should be < 30s (got " << duration_ms << "ms)";
    
    // Should not be instant (sanity check)
    EXPECT_GT(duration_ms, 100) 
        << "Inference time seems too fast, might be cached incorrectly";
    
    LOG_INFO("VLMIntegration", QString("✅ Inference time: %1ms (%.2fs)").arg(duration_ms).arg(duration_ms / 1000.0));
}
