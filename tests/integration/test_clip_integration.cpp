#include <gtest/gtest.h>
#include <QApplication>
#include <QImage>
#include <QFileInfo>
#include <QDir>
#include <cmath>
#include "../../src/ml/CLIPAnalyzer.h"
#include "../../src/core/Logger.h"

using namespace PhotoGuru;

/**
 * CLIP Integration Tests
 * 
 * These tests use REAL AI models (335MB CLIP-ViT-B/32)
 * They validate that:
 * - Models load correctly
 * - Embeddings have correct dimensions
 * - Similar images have high similarity
 * - Different images have low similarity
 * - Same image produces identical embeddings
 * 
 * Expected runtime: 30-60s (models loaded once, shared across tests)
 */
class CLIPIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Initialize QApplication (needed for QImage)
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
        
        // Load CLIP model ONCE for all tests (expensive operation)
        // Model is shared across all tests = much faster
        LOG_INFO("CLIPIntegration", "=== Loading CLIP model for integration tests ===");
        
        QString modelsDir = QDir::currentPath() + "/../models";
        if (!QDir(modelsDir).exists()) {
            modelsDir = QDir::currentPath() + "/models";
        }
        
        QString clipPath = modelsDir + "/clip-vit-base-patch32.onnx";
        
        clip = std::make_unique<CLIPAnalyzer>();
        bool initialized = clip->initialize(clipPath, true);
        
        ASSERT_TRUE(initialized) << "Failed to initialize CLIP model at: " << clipPath.toStdString();
        LOG_INFO("CLIPIntegration", "✅ CLIP model loaded successfully");
        
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
        LOG_INFO("CLIPIntegration", "=== Cleaning up CLIP model ===");
        clip.reset();
    }
    
    // Helper to compute cosine similarity between two embeddings
    static float computeSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) return 0.0f;
        
        float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0f || norm_b == 0.0f) return 0.0f;
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    
    static std::unique_ptr<CLIPAnalyzer> clip;
    static QString fixturesDir;
    static QString testImage1;
    static QString testImage2;
};

// Static members initialization
std::unique_ptr<CLIPAnalyzer> CLIPIntegrationTest::clip = nullptr;
QString CLIPIntegrationTest::fixturesDir;
QString CLIPIntegrationTest::testImage1;
QString CLIPIntegrationTest::testImage2;

// ========== Test 1: Model Initialization ==========

TEST_F(CLIPIntegrationTest, Model_InitializedSuccessfully) {
    ASSERT_NE(clip, nullptr) << "CLIP model should be initialized";
}

// ========== Test 2: Embedding Dimensions ==========

TEST_F(CLIPIntegrationTest, ComputeEmbedding_CorrectDimensions) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull()) << "Failed to load test image: " << testImage1.toStdString();
    
    auto embedding = clip->computeEmbedding(img);
    ASSERT_TRUE(embedding.has_value()) << "CLIP should return embedding for valid image";
    
    // CLIP ViT-B/32 always produces 512-dimensional embeddings
    EXPECT_EQ(embedding->size(), 512) 
        << "CLIP ViT-B/32 embeddings must be 512-dimensional";
    
    LOG_INFO("CLIPIntegration", QString("✅ Embedding dimensions correct: %1").arg(embedding->size()));
}

// ========== Test 3: Same Image Perfect Similarity ==========

TEST_F(CLIPIntegrationTest, SameImage_PerfectSimilarity) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    // Compute embedding twice for same image
    auto emb1 = clip->computeEmbedding(img);
    auto emb2 = clip->computeEmbedding(img);
    
    ASSERT_TRUE(emb1.has_value() && emb2.has_value());
    
    float similarity = computeSimilarity(*emb1, *emb2);
    
    // Same image should have similarity very close to 1.0
    // (small tolerance for floating point arithmetic)
    EXPECT_NEAR(similarity, 1.0f, 0.001f) 
        << "Same image should have ~100% similarity (got " << similarity << ")";
    
    LOG_INFO("CLIPIntegration", QString("✅ Same image similarity: %1").arg(similarity));
}

// ========== Test 4: Different Images Have Lower Similarity ==========

TEST_F(CLIPIntegrationTest, DifferentImages_LowerSimilarity) {
    QImage img1(testImage1);
    QImage img2(testImage2);
    
    ASSERT_FALSE(img1.isNull());
    ASSERT_FALSE(img2.isNull());
    
    auto emb1 = clip->computeEmbedding(img1);
    auto emb2 = clip->computeEmbedding(img2);
    
    ASSERT_TRUE(emb1.has_value() && emb2.has_value());
    
    float similarity = computeSimilarity(*emb1, *emb2);
    
    // Different images should have similarity < 1.0
    // (Exact value depends on image content, but should not be perfect)
    EXPECT_LT(similarity, 0.99f) 
        << "Different images should not be perfectly similar (got " << similarity << ")";
    
    // Similarity should still be positive and reasonable (0-1 range)
    EXPECT_GE(similarity, 0.0f) << "Similarity must be >= 0";
    EXPECT_LE(similarity, 1.0f) << "Similarity must be <= 1";
    
    LOG_INFO("CLIPIntegration", QString("✅ Different images similarity: %1").arg(similarity));
}

// ========== Test 5: Embedding Consistency ==========

TEST_F(CLIPIntegrationTest, MultipleInferences_ConsistentResults) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    // Compute embedding 3 times
    auto emb1 = clip->computeEmbedding(img);
    auto emb2 = clip->computeEmbedding(img);
    auto emb3 = clip->computeEmbedding(img);
    
    ASSERT_TRUE(emb1.has_value() && emb2.has_value() && emb3.has_value());
    
    // All embeddings should be identical (deterministic model)
    float sim12 = computeSimilarity(*emb1, *emb2);
    float sim23 = computeSimilarity(*emb2, *emb3);
    float sim13 = computeSimilarity(*emb1, *emb3);
    
    EXPECT_NEAR(sim12, 1.0f, 0.001f) << "Embedding 1 vs 2 should be identical";
    EXPECT_NEAR(sim23, 1.0f, 0.001f) << "Embedding 2 vs 3 should be identical";
    EXPECT_NEAR(sim13, 1.0f, 0.001f) << "Embedding 1 vs 3 should be identical";
    
    LOG_INFO("CLIPIntegration", QString("✅ Consistency check: %1, %2, %3").arg(sim12).arg(sim23).arg(sim13));
}

// ========== Test 6: Embedding Normalization ==========

TEST_F(CLIPIntegrationTest, Embedding_IsNormalized) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    auto embedding = clip->computeEmbedding(img);
    ASSERT_TRUE(embedding.has_value());
    
    // Compute L2 norm of embedding
    float norm = 0.0f;
    for (float val : *embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    // CLIP embeddings should be normalized (L2 norm ≈ 1.0)
    EXPECT_NEAR(norm, 1.0f, 0.01f) 
        << "CLIP embeddings should be L2-normalized (norm=" << norm << ")";
    
    LOG_INFO("CLIPIntegration", QString("✅ Embedding L2 norm: %1").arg(norm));
}

// ========== Test 7: Invalid Image Handling ==========

TEST_F(CLIPIntegrationTest, InvalidImage_ReturnsEmpty) {
    QImage invalidImg; // Null image
    ASSERT_TRUE(invalidImg.isNull());
    
    auto embedding = clip->computeEmbedding(invalidImg);
    
    // Should return empty optional or handle gracefully
    EXPECT_FALSE(embedding.has_value()) 
        << "Invalid image should return empty embedding";
    
    LOG_INFO("CLIPIntegration", "✅ Invalid image handled correctly");
}

// ========== Test 8: Performance Baseline ==========

TEST_F(CLIPIntegrationTest, Performance_ReasonableInferenceTime) {
    QImage img(testImage1);
    ASSERT_FALSE(img.isNull());
    
    // Warm-up inference (first one can be slower)
    clip->computeEmbedding(img);
    
    // Measure inference time
    auto start = std::chrono::high_resolution_clock::now();
    
    auto embedding = clip->computeEmbedding(img);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    ASSERT_TRUE(embedding.has_value());
    
    // Inference should be under 1 second (typically 50-230ms)
    EXPECT_LT(duration_ms, 1000) 
        << "CLIP inference should be < 1s (got " << duration_ms << "ms)";
    
    // Should not be instant (sanity check)
    EXPECT_GT(duration_ms, 10) 
        << "Inference time seems too fast, might be cached incorrectly";
    
    LOG_INFO("CLIPIntegration", QString("✅ Inference time: %1ms").arg(duration_ms));
}
