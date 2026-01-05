#pragma once

#include "ONNXInference.h"
#include <QString>
#include <QImage>
#include <vector>
#include <optional>
#include <opencv2/opencv.hpp>

namespace PhotoGuru {

/**
 * @brief CLIP (Contrastive Language-Image Pre-training) analyzer
 * 
 * Uses ONNX Runtime to run CLIP vision model locally:
 * - Generates 512-dimensional image embeddings
 * - Enables semantic image search
 * - Zero-shot image classification
 * - Image similarity comparison
 * 
 * Model: CLIP ViT-B/32 (OpenAI)
 * Size: ~170MB ONNX
 * Performance: ~200ms per image (CoreML accelerated)
 * 
 * Usage:
 *   CLIPAnalyzer clip;
 *   clip.initialize("models/clip_vision.onnx");
 *   auto embedding = clip.computeEmbedding(image);
 *   float similarity = clip.cosineSimilarity(emb1, emb2);
 */
class CLIPAnalyzer {
public:
    CLIPAnalyzer();
    ~CLIPAnalyzer();
    
    /**
     * @brief Initialize CLIP model
     * @param visionModelPath Path to clip_vision.onnx
     * @param useGPU Use CoreML/GPU acceleration
     * @return true if initialized successfully
     */
    bool initialize(const QString& visionModelPath, bool useGPU = true);
    
    /**
     * @brief Load CLIP model (alternative name for initialize)
     */
    bool loadModel(const QString& modelPath, bool useGPU = true) {
        return initialize(modelPath, useGPU);
    }
    
    /**
     * @brief Check if model is ready
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * @brief Check if model is loaded (alternative name)
     */
    bool isModelLoaded() const { return m_initialized; }
    
    /**
     * @brief Compute CLIP embedding for image
     * @param image Input image (any size, will be resized to 224x224)
     * @return 512-dimensional embedding vector, normalized to unit length
     */
    std::optional<std::vector<float>> computeEmbedding(const QImage& image);
    
    /**
     * @brief Compute CLIP embedding for cv::Mat image
     * @param image OpenCV Mat image (any size, will be resized to 224x224)
     * @return 512-dimensional embedding vector, normalized to unit length
     */
    std::vector<float> computeEmbedding(const cv::Mat& image);
    
    /**
     * @brief Compute CLIP embedding for image file
     * @param imagePath Path to image file
     */
    std::optional<std::vector<float>> computeEmbedding(const QString& imagePath);
    
    /**
     * @brief Calculate cosine similarity between two embeddings
     * @param emb1 First embedding (must be normalized)
     * @param emb2 Second embedding (must be normalized)
     * @return Similarity score [-1, 1], where 1 = identical, 0 = orthogonal, -1 = opposite
     */
    float cosineSimilarity(
        const std::vector<float>& emb1,
        const std::vector<float>& emb2
    ) const;
    
    /**
     * @brief Find most similar images from a set
     * @param queryEmbedding Query image embedding
     * @param databaseEmbeddings List of candidate embeddings
     * @param k Number of results to return
     * @return Indices sorted by similarity (descending)
     */
    std::vector<int> findMostSimilar(
        const std::vector<float>& queryEmbedding,
        const std::vector<std::vector<float>>& databaseEmbeddings,
        int k = 10
    ) const;
    
    /**
     * @brief Zero-shot classification using precomputed text embeddings
     * @param imageEmbedding Image embedding
     * @param textEmbeddings List of (label, embedding) pairs
     * @return Sorted list of (label, score) by confidence
     */
    std::vector<std::pair<QString, float>> zeroShotClassification(
        const std::vector<float>& imageEmbedding,
        const std::vector<std::pair<QString, std::vector<float>>>& textEmbeddings
    ) const;
    
    /**
     * @brief Get last error message
     */
    QString lastError() const;
    
    /**
     * @brief Get model info
     */
    struct ModelInfo {
        int embeddingDim = 512;
        int inputSize = 224;
        QString modelVersion;
        bool gpuAccelerated = false;
    };
    ModelInfo getModelInfo() const { return m_modelInfo; }

private:
    std::unique_ptr<ONNXInference> m_visionModel;
    bool m_initialized = false;
    QString m_lastError;
    ModelInfo m_modelInfo;
    
    // Normalize embedding to unit length
    void normalizeEmbedding(std::vector<float>& embedding) const;
};

} // namespace PhotoGuru
