#include "CLIPAnalyzer.h"
#include <QImage>
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace PhotoGuru {

CLIPAnalyzer::CLIPAnalyzer() 
    : m_visionModel(std::make_unique<ONNXInference>())
{
}

CLIPAnalyzer::~CLIPAnalyzer() = default;

bool CLIPAnalyzer::initialize(const QString& visionModelPath, bool useGPU) {
    qDebug() << "[CLIP] Initializing with model:" << visionModelPath;
    
    if (!m_visionModel->loadModel(visionModelPath, useGPU)) {
        m_lastError = "Failed to load vision model: " + m_visionModel->lastError();
        qWarning() << "[CLIP]" << m_lastError;
        return false;
    }
    
    // Set model info
    m_modelInfo.embeddingDim = 512; // CLIP ViT-B/32
    m_modelInfo.inputSize = 224;
    m_modelInfo.gpuAccelerated = useGPU;
    m_modelInfo.modelVersion = "ViT-B/32";
    
    auto outputShape = m_visionModel->getOutputShape();
    if (!outputShape.empty() && outputShape.back() != 512) {
        qWarning() << "[CLIP] Unexpected output dimension:" << outputShape.back();
        qDebug() << "[CLIP] Expected 512, adjusting...";
        m_modelInfo.embeddingDim = outputShape.back();
    }
    
    m_initialized = true;
    qDebug() << "[CLIP] Initialized successfully";
    qDebug() << "[CLIP] Embedding dimension:" << m_modelInfo.embeddingDim;
    qDebug() << "[CLIP] Input size:" << m_modelInfo.inputSize << "x" << m_modelInfo.inputSize;
    
    return true;
}

std::optional<std::vector<float>> CLIPAnalyzer::computeEmbedding(const QImage& image) {
    if (!m_initialized) {
        m_lastError = "CLIP analyzer not initialized";
        return std::nullopt;
    }
    
    if (image.isNull()) {
        m_lastError = "Invalid image";
        return std::nullopt;
    }
    
    // CLIP preprocessing: mean=[0.48145466, 0.4578275, 0.40821073], std=[0.26862954, 0.26130258, 0.27577711]
    std::vector<float> mean = {0.48145466f, 0.4578275f, 0.40821073f};
    std::vector<float> std = {0.26862954f, 0.26130258f, 0.27577711f};
    
    // Preprocess image
    auto inputTensor = m_visionModel->preprocessImage(image, mean, std);
    if (inputTensor.empty()) {
        m_lastError = "Failed to preprocess image";
        return std::nullopt;
    }
    
    // Run inference
    auto output = m_visionModel->runInference(inputTensor);
    if (!output.has_value()) {
        m_lastError = "Inference failed: " + m_visionModel->lastError();
        return std::nullopt;
    }
    
    // Normalize embedding to unit length
    std::vector<float> embedding = output.value();
    normalizeEmbedding(embedding);
    
    qDebug() << "[CLIP] Computed embedding: dim=" << embedding.size() 
             << ", norm=" << std::sqrt(std::inner_product(
                 embedding.begin(), embedding.end(), embedding.begin(), 0.0f));
    
    return embedding;
}

std::vector<float> CLIPAnalyzer::computeEmbedding(const cv::Mat& image) {
    qDebug() << "[CLIP] Computing embedding from cv::Mat:" << image.cols << "x" << image.rows 
             << "channels=" << image.channels();
    
    // Convert cv::Mat to QImage
    QImage qImage;
    
    if (image.empty()) {
        m_lastError = "Empty image";
        qDebug() << "[CLIP] Error:" << m_lastError;
        return {};
    }
    
    if (image.channels() == 3) {
        // BGR to RGB
        cv::Mat rgb;
        cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);
        qImage = QImage(rgb.data, rgb.cols, rgb.rows, 
                       rgb.step, QImage::Format_RGB888).copy();
    } else if (image.channels() == 1) {
        qImage = QImage(image.data, image.cols, image.rows,
                       image.step, QImage::Format_Grayscale8).copy();
    } else {
        m_lastError = "Unsupported image format: " + QString::number(image.channels()) + " channels";
        qDebug() << "[CLIP] Error:" << m_lastError;
        return {};
    }
    
    qDebug() << "[CLIP] Converted to QImage:" << qImage.width() << "x" << qImage.height();
    
    auto result = computeEmbedding(qImage);
    if (result.has_value()) {
        qDebug() << "[CLIP] Embedding computed successfully from cv::Mat";
        return result.value();
    }
    qDebug() << "[CLIP] Failed to compute embedding:" << m_lastError;
    return {};
}

std::optional<std::vector<float>> CLIPAnalyzer::computeEmbedding(const QString& imagePath) {
    QImage image(imagePath);
    if (image.isNull()) {
        m_lastError = "Failed to load image: " + imagePath;
        return std::nullopt;
    }
    
    return computeEmbedding(image);
}

float CLIPAnalyzer::cosineSimilarity(
    const std::vector<float>& emb1,
    const std::vector<float>& emb2
) const {
    if (emb1.size() != emb2.size()) {
        qWarning() << "[CLIP] Embedding size mismatch:" << emb1.size() << "vs" << emb2.size();
        return 0.0f;
    }
    
    // Compute dot product (embeddings should already be normalized)
    float dotProduct = std::inner_product(emb1.begin(), emb1.end(), emb2.begin(), 0.0f);
    
    return dotProduct;
}

std::vector<int> CLIPAnalyzer::findMostSimilar(
    const std::vector<float>& queryEmbedding,
    const std::vector<std::vector<float>>& databaseEmbeddings,
    int k
) const {
    if (databaseEmbeddings.empty()) {
        return {};
    }
    
    // Calculate similarities
    std::vector<std::pair<float, int>> similarities;
    similarities.reserve(databaseEmbeddings.size());
    
    for (size_t i = 0; i < databaseEmbeddings.size(); ++i) {
        float sim = cosineSimilarity(queryEmbedding, databaseEmbeddings[i]);
        similarities.emplace_back(sim, static_cast<int>(i));
    }
    
    // Sort by similarity (descending)
    std::sort(similarities.begin(), similarities.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top k indices
    int numResults = std::min(k, static_cast<int>(similarities.size()));
    std::vector<int> result;
    result.reserve(numResults);
    
    for (int i = 0; i < numResults; ++i) {
        result.push_back(similarities[i].second);
    }
    
    return result;
}

std::vector<std::pair<QString, float>> CLIPAnalyzer::zeroShotClassification(
    const std::vector<float>& imageEmbedding,
    const std::vector<std::pair<QString, std::vector<float>>>& textEmbeddings
) const {
    std::vector<std::pair<QString, float>> results;
    results.reserve(textEmbeddings.size());
    
    for (const auto& [label, textEmb] : textEmbeddings) {
        float sim = cosineSimilarity(imageEmbedding, textEmb);
        results.emplace_back(label, sim);
    }
    
    // Sort by confidence (descending)
    std::sort(results.begin(), results.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return results;
}

void CLIPAnalyzer::normalizeEmbedding(std::vector<float>& embedding) const {
    float norm = std::sqrt(std::inner_product(
        embedding.begin(), embedding.end(), embedding.begin(), 0.0f));
    
    if (norm > 1e-6f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
}

QString CLIPAnalyzer::lastError() const {
    return m_lastError;
}

} // namespace PhotoGuru

