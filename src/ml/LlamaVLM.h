#pragma once

#include <QString>
#include <QImage>
#include <memory>
#include <optional>
#include <string>

// Forward declarations
struct llama_model;
struct llama_context;
struct mtmd_context;

namespace PhotoGuru {

/**
 * @brief Wrapper for llama.cpp Vision-Language Model
 * 
 * Integrates Qwen3-VL for multimodal image understanding:
 * - Image captioning
 * - Visual question answering
 * - Detailed scene description
 * 
 * Uses llama.cpp for inference with Metal (GPU) acceleration.
 */
class LlamaVLM {
public:
    struct ModelConfig {
        QString modelPath;      // Path to main .gguf file
        QString mmprojPath;     // Path to mmproj .gguf file
        int contextSize = 2048; // Context window size
        int nThreads = 4;       // Number of CPU threads
        int nGPULayers = 5;     // GPU layers (5 optimal for Mac M4)
        float temperature = 0.7f;
        int maxTokens = 512;    // Max tokens to generate
    };
    
    explicit LlamaVLM();
    ~LlamaVLM();
    
    // Disable copy, allow move
    LlamaVLM(const LlamaVLM&) = delete;
    LlamaVLM& operator=(const LlamaVLM&) = delete;
    LlamaVLM(LlamaVLM&&) noexcept;
    LlamaVLM& operator=(LlamaVLM&&) noexcept;
    
    /**
     * @brief Initialize model and vision projector
     * @param config Model configuration
     * @return true if successful
     */
    bool initialize(const ModelConfig& config);
    
    /**
     * @brief Check if model is loaded and ready
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * @brief Generate image caption
     * @param image Input image
     * @return Generated caption or nullopt on error
     */
    std::optional<QString> generateCaption(const QImage& image);
    
    /**
     * @brief Ask a question about an image
     * @param image Input image
     * @param question Question text
     * @return Answer or nullopt on error
     */
    std::optional<QString> answerQuestion(const QImage& image, const QString& question);
    
    /**
     * @brief Generate detailed description with keywords
     * @param image Input image
     * @param includeKeywords Extract keywords separately
     * @return Description or nullopt on error
     */
    std::optional<QString> analyzeImage(const QImage& image, bool includeKeywords = true);
    
    /**
     * @brief Get last error message
     */
    QString lastError() const { return m_lastError; }
    
    /**
     * @brief Get model configuration
     */
    const ModelConfig& config() const { return m_config; }
    
private:
    /**
     * @brief Run inference with image and text prompt
     */
    std::optional<QString> runInference(const QImage& image, const QString& prompt);
    
    /**
     * @brief Encode image through vision projector
     */
    bool encodeImage(const QImage& image);
    
    /**
     * @brief Sample tokens from model
     */
    std::string sampleTokens(int maxTokens);
    
    bool m_initialized = false;
    ModelConfig m_config;
    QString m_lastError;
    
    // llama.cpp handles
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    mtmd_context* m_mtmdCtx = nullptr;
};

} // namespace PhotoGuru
