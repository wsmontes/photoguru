#include "ONNXInference.h"
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <QImage>
#include <QDebug>
#include <cmath>

namespace PhotoGuru {

// ONNX Runtime environment (singleton)
static std::unique_ptr<Ort::Env> g_ort_env = nullptr;
static std::mutex g_env_mutex;

ONNXInference::ONNXInference() {
    initializeEnvironment();
}

ONNXInference::~ONNXInference() = default;

ONNXInference::ONNXInference(ONNXInference&&) noexcept = default;
ONNXInference& ONNXInference::operator=(ONNXInference&&) noexcept = default;

void ONNXInference::initializeEnvironment() {
    std::lock_guard<std::mutex> lock(g_env_mutex);
    if (!g_ort_env) {
        try {
            g_ort_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "PhotoGuru");
            qDebug() << "[ONNX] Runtime environment initialized";
        } catch (const Ort::Exception& e) {
            qWarning() << "[ONNX] Failed to initialize environment:" << e.what();
        }
    }
}

bool ONNXInference::loadModel(const QString& modelPath, bool useGPU) {
    if (!g_ort_env) {
        m_lastError = "ONNX Runtime environment not initialized";
        return false;
    }
    
    try {
        // Create session options
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(4);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        
        // Try to use CoreML on macOS for GPU acceleration
        if (useGPU) {
#ifdef __APPLE__
            try {
                // CoreML provider - simplified approach without explicit options
                uint32_t coreml_flags = 0;
                sessionOptions.AppendExecutionProvider("CoreML", {{"CoreMLFlags", std::to_string(coreml_flags)}});
                qDebug() << "[ONNX] Using CoreML execution provider (GPU accelerated)";
            } catch (const Ort::Exception& e) {
                qWarning() << "[ONNX] CoreML provider not available, using CPU:" << e.what();
            }
#else
            // Try CUDA on other platforms
            try {
                sessionOptions.AppendExecutionProvider("CUDA", {});
                qDebug() << "[ONNX] Using CUDA execution provider";
            } catch (const Ort::Exception&) {
                qDebug() << "[ONNX] CUDA not available, using CPU";
            }
#endif
        }
        
        // Load the model
        std::string modelPathStr = modelPath.toStdString();
        m_session = std::make_unique<Ort::Session>(*g_ort_env, modelPathStr.c_str(), sessionOptions);
        
        // Store session options
        m_sessionOptions = std::make_unique<Ort::SessionOptions>(std::move(sessionOptions));
        
        // Get input info
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_input_nodes = m_session->GetInputCount();
        if (num_input_nodes > 0) {
            Ort::TypeInfo input_type_info = m_session->GetInputTypeInfo(0);
            auto tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
            m_inputShape = tensor_info.GetShape();
            
            // Handle fully dynamic shapes (CLIP models often have all -1)
            // Assume standard CLIP input: [batch=1, channels=3, height=224, width=224]
            bool has_dynamic_dims = false;
            for (auto dim : m_inputShape) {
                if (dim < 0) {
                    has_dynamic_dims = true;
                    break;
                }
            }
            
            if (has_dynamic_dims || m_inputShape.empty()) {
                qDebug() << "[ONNX] Model has dynamic input shape, using CLIP defaults [1, 3, 224, 224]";
                m_inputShape = {1, 3, 224, 224};
            } else if (m_inputShape.size() > 0 && m_inputShape[0] < 0) {
                // Only batch size is dynamic
                m_inputShape[0] = 1;
            }
            
            qDebug() << "[ONNX] Model loaded:" << modelPath;
            qDebug() << "[ONNX] Input shape: [" 
                     << (m_inputShape.size() > 0 ? m_inputShape[0] : 0) << ","
                     << (m_inputShape.size() > 1 ? m_inputShape[1] : 0) << ","
                     << (m_inputShape.size() > 2 ? m_inputShape[2] : 0) << ","
                     << (m_inputShape.size() > 3 ? m_inputShape[3] : 0) << "]";
        }
        
        // Get output info
        size_t num_output_nodes = m_session->GetOutputCount();
        if (num_output_nodes > 0) {
            Ort::TypeInfo output_type_info = m_session->GetOutputTypeInfo(0);
            auto tensor_info = output_type_info.GetTensorTypeAndShapeInfo();
            m_outputShape = tensor_info.GetShape();
            
            qDebug() << "[ONNX] Output shape: [" 
                     << (m_outputShape.size() > 0 ? m_outputShape[0] : 0) << ","
                     << (m_outputShape.size() > 1 ? m_outputShape[1] : 0) << "]";
        }
        
        // Create memory info for CPU
        m_memoryInfo = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)
        );
        
        m_loaded = true;
        return true;
        
    } catch (const Ort::Exception& e) {
        m_lastError = QString("Failed to load model: %1").arg(e.what());
        qWarning() << "[ONNX]" << m_lastError;
        return false;
    }
}

std::vector<float> ONNXInference::preprocessImage(
    const QImage& image,
    const std::vector<float>& mean,
    const std::vector<float>& std
) const {
    if (m_inputShape.size() < 4) {
        qWarning() << "[ONNX] Invalid input shape";
        return {};
    }
    
    // Get target dimensions (assume NCHW format)
    int target_height = m_inputShape[2];
    int target_width = m_inputShape[3];
    int channels = m_inputShape[1];
    
    // Resize image
    QImage resized = image.scaled(target_width, target_height, 
                                  Qt::IgnoreAspectRatio, 
                                  Qt::SmoothTransformation);
    
    // Convert to RGB if needed
    if (resized.format() != QImage::Format_RGB888) {
        resized = resized.convertToFormat(QImage::Format_RGB888);
    }
    
    // Allocate tensor (NCHW format)
    std::vector<float> tensor(channels * target_height * target_width);
    
    // Convert HWC (QImage) to CHW (ONNX) and normalize
    for (int h = 0; h < target_height; ++h) {
        const uchar* line = resized.constScanLine(h);
        for (int w = 0; w < target_width; ++w) {
            for (int c = 0; c < channels; ++c) {
                float pixel = line[w * channels + c] / 255.0f;
                
                // Normalize with mean and std
                if (c < static_cast<int>(mean.size()) && c < static_cast<int>(std.size())) {
                    pixel = (pixel - mean[c]) / std[c];
                }
                
                // CHW format: channel * height * width + height * width + width
                int idx = c * target_height * target_width + h * target_width + w;
                tensor[idx] = pixel;
            }
        }
    }
    
    return tensor;
}

std::optional<std::vector<float>> ONNXInference::runInference(
    const std::vector<float>& inputTensor
) {
    if (!m_loaded || !m_session || !m_memoryInfo) {
        m_lastError = "Model not loaded";
        return std::nullopt;
    }
    
    try {
        // Debug: print shapes
        qDebug() << "[ONNX] Input tensor size:" << inputTensor.size();
        qDebug() << "[ONNX] Expected shape:" << m_inputShape[0] << "x" << m_inputShape[1] 
                 << "x" << m_inputShape[2] << "x" << m_inputShape[3];
        
        size_t expected_size = 1;
        for (auto dim : m_inputShape) {
            expected_size *= dim;
        }
        qDebug() << "[ONNX] Expected size:" << expected_size;
        
        if (inputTensor.size() != expected_size) {
            m_lastError = QString("Tensor size mismatch: got %1, expected %2")
                          .arg(inputTensor.size()).arg(expected_size);
            qWarning() << "[ONNX]" << m_lastError;
            return std::nullopt;
        }
        
        qDebug() << "[ONNX] Creating tensor...";
        
        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr input_name_ptr = m_session->GetInputNameAllocated(0, allocator);
        Ort::AllocatedStringPtr output_name_ptr = m_session->GetOutputNameAllocated(0, allocator);
        
        const char* input_names[] = {input_name_ptr.get()};
        const char* output_names[] = {output_name_ptr.get()};
        
        // Create a mutable copy of input tensor for ONNX Runtime
        std::vector<float> mutable_input(inputTensor.begin(), inputTensor.end());
        
        // Create input tensor with proper shape
        std::vector<int64_t> input_shape_int64(m_inputShape.begin(), m_inputShape.end());
        
        auto input_tensor = Ort::Value::CreateTensor<float>(
            *m_memoryInfo,
            mutable_input.data(),
            mutable_input.size(),
            input_shape_int64.data(),
            input_shape_int64.size()
        );
        
        qDebug() << "[ONNX] Running inference with input tensor size:" << mutable_input.size();
        qDebug() << "[ONNX] Input names:" << input_names[0];
        qDebug() << "[ONNX] Output names:" << output_names[0];
        
        // Run inference
        qDebug() << "[ONNX] Calling session->Run()...";
        auto output_tensors = m_session->Run(
            Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1,
            output_names, 1
        );
        qDebug() << "[ONNX] Inference completed successfully";
        
        // Extract output
        qDebug() << "[ONNX] Extracting output tensors...";
        if (output_tensors.size() == 0) {
            m_lastError = "No output tensors from model";
            qWarning() << "[ONNX]" << m_lastError;
            return std::nullopt;
        }
        
        qDebug() << "[ONNX] Number of output tensors:" << output_tensors.size();
        
        // Get tensor info
        Ort::TensorTypeAndShapeInfo type_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        size_t output_size = type_info.GetElementCount();
        auto shape = type_info.GetShape();
        
        qDebug() << "[ONNX] Output element count:" << output_size;
        qDebug() << "[ONNX] Output shape dimensions:" << shape.size();
        for (size_t i = 0; i < shape.size(); ++i) {
            qDebug() << "[ONNX]   Dim" << i << ":" << shape[i];
        }
        
        // Get pointer to output data
        qDebug() << "[ONNX] Getting output data pointer...";
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        if (!output_data) {
            m_lastError = "Failed to get output data pointer";
            qWarning() << "[ONNX]" << m_lastError;
            return std::nullopt;
        }
        
        qDebug() << "[ONNX] Creating result vector of size:" << output_size;
        std::vector<float> result;
        result.reserve(output_size);
        for (size_t i = 0; i < output_size; ++i) {
            result.push_back(output_data[i]);
        }
        
        qDebug() << "[ONNX] Successfully created result vector with" << result.size() << "elements";
        return result;
        
    } catch (const Ort::Exception& e) {
        m_lastError = QString("ONNX Runtime exception: %1").arg(e.what());
        qWarning() << "[ONNX]" << m_lastError;
        return std::nullopt;
    } catch (const std::exception& e) {
        m_lastError = QString("Standard exception: %1").arg(e.what());
        qWarning() << "[ONNX]" << m_lastError;
        return std::nullopt;
    } catch (...) {
        m_lastError = "Unknown exception during inference";
        qWarning() << "[ONNX]" << m_lastError;
        return std::nullopt;
    }
}

} // namespace PhotoGuru
