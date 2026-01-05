#pragma once

#include <QString>
#include <QImage>
#include <vector>
#include <memory>
#include <optional>

// Forward declare ONNX Runtime types to avoid header pollution
namespace Ort {
    struct Env;
    struct Session;
    struct SessionOptions;
    struct MemoryInfo;
}

namespace PhotoGuru {

/**
 * @brief Base class for ONNX Runtime inference
 * 
 * Provides common functionality for loading and running ONNX models:
 * - Model loading with CoreML/Metal acceleration on macOS
 * - Image preprocessing (resize, normalize, CHW format)
 * - Tensor management
 * - Thread-safe execution
 */
class ONNXInference {
public:
    explicit ONNXInference();
    virtual ~ONNXInference();
    
    // Disable copy, allow move
    ONNXInference(const ONNXInference&) = delete;
    ONNXInference& operator=(const ONNXInference&) = delete;
    ONNXInference(ONNXInference&&) noexcept;
    ONNXInference& operator=(ONNXInference&&) noexcept;
    
    /**
     * @brief Shutdown ONNX Runtime environment (call before program exit)
     * This should be called from MainWindow destructor to prevent crash
     */
    static void shutdownEnvironment();
    
    /**
     * @brief Load ONNX model from file
     * @param modelPath Absolute path to .onnx file
     * @param useGPU Try to use GPU acceleration (CoreML on Mac, CUDA on others)
     * @return true if loaded successfully
     */
    bool loadModel(const QString& modelPath, bool useGPU = true);
    
    /**
     * @brief Check if model is loaded and ready
     */
    bool isLoaded() const { return m_loaded; }
    
    /**
     * @brief Get model input dimensions [batch, channels, height, width]
     */
    std::vector<int64_t> getInputShape() const { return m_inputShape; }
    
    /**
     * @brief Get model output dimensions
     */
    std::vector<int64_t> getOutputShape() const { return m_outputShape; }
    
    /**
     * @brief Preprocess image to model input tensor
     * 
     * Common preprocessing:
     * - Resize to input size
     * - Convert to float32
     * - Normalize (mean/std)
     * - Convert HWC -> CHW format
     * 
     * @param image Input QImage
     * @param mean Channel means for normalization
     * @param std Channel standard deviations
     * @return Tensor data in CHW format
     */
    std::vector<float> preprocessImage(
        const QImage& image,
        const std::vector<float>& mean = {0.485f, 0.456f, 0.406f},
        const std::vector<float>& std = {0.229f, 0.224f, 0.225f}
    ) const;
    
    /**
     * @brief Run inference with preprocessed tensor
     * @param inputTensor Input data (already preprocessed)
     * @return Output tensor or empty if error
     */
    std::optional<std::vector<float>> runInference(
        const std::vector<float>& inputTensor
    );
    
    /**
     * @brief Get error message from last operation
     */
    QString lastError() const { return m_lastError; }
    
    /**
     * @brief Check if model is loaded
     */
    bool isModelLoaded() const { return m_session != nullptr; }

protected:

private:
    // ONNX Runtime components
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;
    
    // Model metadata
    std::vector<int64_t> m_inputShape;
    std::vector<int64_t> m_outputShape;
    bool m_loaded = false;
    QString m_lastError;
    
    // Initialize ONNX Runtime environment (called once)
    void initializeEnvironment();
};

} // namespace PhotoGuru
