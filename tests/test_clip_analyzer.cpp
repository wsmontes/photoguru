#include <gtest/gtest.h>
#include "../src/ml/CLIPAnalyzer.h"
#include "../src/core/ImageLoader.h"
#include <opencv2/opencv.hpp>
#include <QDebug>

using namespace PhotoGuru;

class CLIPAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Model path (will need to be downloaded)
        modelPath = QString::fromStdString("../models/clip-vit-base-patch32.onnx");
    }

    QString modelPath;
};

TEST_F(CLIPAnalyzerTest, Constructor) {
    // Test that CLIPAnalyzer can be created
    CLIPAnalyzer analyzer;
    EXPECT_FALSE(analyzer.isModelLoaded());
}

TEST_F(CLIPAnalyzerTest, LoadModelFailsWithInvalidPath) {
    CLIPAnalyzer analyzer;
    
    // Try to load a non-existent model
    bool loaded = analyzer.loadModel("/invalid/path/model.onnx");
    
    EXPECT_FALSE(loaded);
    EXPECT_FALSE(analyzer.isModelLoaded());
}

TEST_F(CLIPAnalyzerTest, DISABLED_LoadModelSucceeds) {
    // This test is disabled until model is downloaded
    CLIPAnalyzer analyzer;
    
    bool loaded = analyzer.loadModel(modelPath);
    
    EXPECT_TRUE(loaded);
    EXPECT_TRUE(analyzer.isModelLoaded());
}

TEST_F(CLIPAnalyzerTest, DISABLED_ComputeEmbeddingProducesValidOutput) {
    // This test is disabled until model is downloaded
    CLIPAnalyzer analyzer;
    ASSERT_TRUE(analyzer.loadModel(modelPath));
    
    // Create a simple test image (224x224 RGB)
    cv::Mat testImage(224, 224, CV_8UC3, cv::Scalar(128, 128, 128));
    
    // Compute embedding
    std::vector<float> embedding = analyzer.computeEmbedding(testImage);
    
    // CLIP ViT-B/32 produces 512-dimensional embeddings
    EXPECT_EQ(embedding.size(), 512);
    
    // Check that embedding is normalized (L2 norm should be ~1.0)
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    EXPECT_NEAR(norm, 1.0f, 0.01f);
}

TEST_F(CLIPAnalyzerTest, DISABLED_ComputeEmbeddingFromRealHEIC) {
    CLIPAnalyzer analyzer;
    ASSERT_TRUE(analyzer.loadModel(modelPath));
    
    // Load real HEIC image from Test_10
    QString imagePath = "/Users/wagnermontes/Documents/GitHub/photoguru/Test_10/IMG_0001.HEIC";
    QImage image(imagePath);
    
    if (image.isNull()) {
        GTEST_SKIP() << "Could not load test image: " << imagePath.toStdString();
    }
    
    qDebug() << "[TEST] Loaded HEIC image:" << image.width() << "x" << image.height();
    
    auto embedding = analyzer.computeEmbedding(image);
    
    ASSERT_TRUE(embedding.has_value());
    EXPECT_EQ(embedding.value().size(), 512);
    
    // Check normalization
    float norm = 0.0f;
    for (float val : embedding.value()) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    EXPECT_NEAR(norm, 1.0f, 0.01f);
    
    qDebug() << "[TEST] ✅ CLIP embedding computed successfully from real HEIC image!";
    qDebug() << "[TEST] First 5 values:" << embedding.value()[0] << embedding.value()[1] 
             << embedding.value()[2] << embedding.value()[3] << embedding.value()[4];
}

TEST_F(CLIPAnalyzerTest, DISABLED_EmbeddingsAreDeterministic) {
    // This test is disabled until model is downloaded
    CLIPAnalyzer analyzer;
    ASSERT_TRUE(analyzer.loadModel(modelPath));
    
    // Create a test image
    cv::Mat testImage(224, 224, CV_8UC3, cv::Scalar(100, 150, 200));
    
    // Compute embedding twice
    std::vector<float> embedding1 = analyzer.computeEmbedding(testImage);
    std::vector<float> embedding2 = analyzer.computeEmbedding(testImage);
    
    // Should be identical
    ASSERT_EQ(embedding1.size(), embedding2.size());
    for (size_t i = 0; i < embedding1.size(); ++i) {
        EXPECT_FLOAT_EQ(embedding1[i], embedding2[i]);
    }
}

TEST_F(CLIPAnalyzerTest, DISABLED_DifferentImagesProduceDifferentEmbeddings) {
    // This test is disabled until model is downloaded
    CLIPAnalyzer analyzer;
    ASSERT_TRUE(analyzer.loadModel(modelPath));
    
    // Create two different test images
    cv::Mat image1(224, 224, CV_8UC3, cv::Scalar(100, 100, 100));
    cv::Mat image2(224, 224, CV_8UC3, cv::Scalar(200, 200, 200));
    
    // Compute embeddings
    std::vector<float> embedding1 = analyzer.computeEmbedding(image1);
    std::vector<float> embedding2 = analyzer.computeEmbedding(image2);
    
    // Calculate cosine similarity
    float similarity = analyzer.cosineSimilarity(embedding1, embedding2);
    
    // Should not be identical (similarity < 1.0)
    EXPECT_LT(similarity, 0.99f);
}

TEST_F(CLIPAnalyzerTest, CosineSimilarityIdentical) {
    CLIPAnalyzer analyzer;
    
    // Create identical vectors
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {1.0f, 0.0f, 0.0f};
    
    float similarity = analyzer.cosineSimilarity(vec1, vec2);
    
    EXPECT_FLOAT_EQ(similarity, 1.0f);
}

TEST_F(CLIPAnalyzerTest, CosineSimilarityOrthogonal) {
    CLIPAnalyzer analyzer;
    
    // Create orthogonal vectors
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {0.0f, 1.0f, 0.0f};
    
    float similarity = analyzer.cosineSimilarity(vec1, vec2);
    
    EXPECT_FLOAT_EQ(similarity, 0.0f);
}

TEST_F(CLIPAnalyzerTest, CosineSimilarityOpposite) {
    CLIPAnalyzer analyzer;
    
    // Create opposite vectors
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {-1.0f, 0.0f, 0.0f};
    
    float similarity = analyzer.cosineSimilarity(vec1, vec2);
    
    EXPECT_FLOAT_EQ(similarity, -1.0f);
}

TEST_F(CLIPAnalyzerTest, FindMostSimilarSingleResult) {
    CLIPAnalyzer analyzer;
    
    // Query embedding
    std::vector<float> query = {1.0f, 0.0f, 0.0f};
    
    // Database embeddings
    std::vector<std::vector<float>> database = {
        {0.9f, 0.1f, 0.0f},  // High similarity
        {0.0f, 1.0f, 0.0f},  // Low similarity
        {-1.0f, 0.0f, 0.0f}  // Opposite
    };
    
    // Find top 1
    std::vector<int> indices = analyzer.findMostSimilar(query, database, 1);
    
    ASSERT_EQ(indices.size(), 1);
    EXPECT_EQ(indices[0], 0);  // Index 0 should be most similar
}

TEST_F(CLIPAnalyzerTest, FindMostSimilarMultipleResults) {
    CLIPAnalyzer analyzer;
    
    // Query embedding
    std::vector<float> query = {1.0f, 0.0f, 0.0f};
    
    // Database embeddings
    std::vector<std::vector<float>> database = {
        {0.9f, 0.1f, 0.0f},   // Rank 1
        {0.8f, 0.2f, 0.0f},   // Rank 2
        {0.0f, 1.0f, 0.0f},   // Rank 4
        {0.7f, 0.3f, 0.0f}    // Rank 3
    };
    
    // Find top 3
    std::vector<int> indices = analyzer.findMostSimilar(query, database, 3);
    
    ASSERT_EQ(indices.size(), 3);
    EXPECT_EQ(indices[0], 0);  // Most similar
    EXPECT_EQ(indices[1], 1);  // Second
    EXPECT_EQ(indices[2], 3);  // Third
}

TEST_F(CLIPAnalyzerTest, FindMostSimilarHandlesEmptyDatabase) {
    CLIPAnalyzer analyzer;
    
    std::vector<float> query = {1.0f, 0.0f, 0.0f};
    std::vector<std::vector<float>> database;
    
    std::vector<int> indices = analyzer.findMostSimilar(query, database, 5);
    
    EXPECT_TRUE(indices.empty());
}

TEST_F(CLIPAnalyzerTest, FindMostSimilarHandlesKLargerThanDatabase) {
    CLIPAnalyzer analyzer;
    
    std::vector<float> query = {1.0f, 0.0f, 0.0f};
    std::vector<std::vector<float>> database = {
        {0.9f, 0.1f, 0.0f},
        {0.8f, 0.2f, 0.0f}
    };
    
    // Request more results than available
    std::vector<int> indices = analyzer.findMostSimilar(query, database, 10);
    
    // Should return all available
    EXPECT_EQ(indices.size(), 2);
}

TEST_F(CLIPAnalyzerTest, DISABLED_RealImageEmbedding) {
    // This test is disabled until model is downloaded
    CLIPAnalyzer analyzer;
    ASSERT_TRUE(analyzer.loadModel(modelPath));
    
    // Load a real test image
    auto qImage = ImageLoader::instance().load(QString("../Test_10/IMG_0001.HEIC"));
    ASSERT_TRUE(qImage.has_value());
    
    // Convert QImage to cv::Mat
    QImage img = qImage.value().convertToFormat(QImage::Format_RGB888);
    cv::Mat image(img.height(), img.width(), CV_8UC3, (void*)img.bits(), img.bytesPerLine());
    
    // Compute embedding
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<float> embedding = analyzer.computeEmbedding(image);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should produce valid embedding
    EXPECT_EQ(embedding.size(), 512);
    
    // Should be fast (<500ms with CoreML acceleration)
    qDebug() << "Embedding computation time:" << duration.count() << "ms";
    EXPECT_LT(duration.count(), 1000);  // Relaxed for CPU-only
}

TEST_F(CLIPAnalyzerTest, DISABLED_SemanticSearchPerformance) {
    // This test is disabled until model is downloaded
    CLIPAnalyzer analyzer;
    ASSERT_TRUE(analyzer.loadModel(modelPath));
    
    // Create a database of 1000 random embeddings
    std::vector<std::vector<float>> database;
    for (int i = 0; i < 1000; ++i) {
        std::vector<float> embedding(512);
        for (int j = 0; j < 512; ++j) {
            embedding[j] = static_cast<float>(rand()) / RAND_MAX;
        }
        database.push_back(embedding);
    }
    
    // Query embedding
    std::vector<float> query(512);
    for (int j = 0; j < 512; ++j) {
        query[j] = static_cast<float>(rand()) / RAND_MAX;
    }
    
    // Benchmark search
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<int> results = analyzer.findMostSimilar(query, database, 10);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(results.size(), 10);
    
    // Should be fast (<50ms for 1000 vectors)
    qDebug() << "Search time (1000 vectors):" << duration.count() << "ms";
    EXPECT_LT(duration.count(), 50);
}
