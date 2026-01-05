#include "LlamaVLM.h"
#include "llama.h"
#include "mtmd.h"
#include "mtmd-helper.h"
#include "common.h"
#include <QImage>
#include <QDebug>
#include <vector>

namespace PhotoGuru {

LlamaVLM::LlamaVLM() = default;

LlamaVLM::~LlamaVLM() {
    if (m_ctx) {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }
    if (m_mtmdCtx) {
        mtmd_free(m_mtmdCtx);
        m_mtmdCtx = nullptr;
    }
    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }
    
    // Backend cleanup
    llama_backend_free();
}

LlamaVLM::LlamaVLM(LlamaVLM&&) noexcept = default;
LlamaVLM& LlamaVLM::operator=(LlamaVLM&&) noexcept = default;

bool LlamaVLM::initialize(const ModelConfig& config) {
    if (m_initialized) {
        m_lastError = "Already initialized";
        return false;
    }
    
    m_config = config;
    
    try {
        // Initialize llama backend
        llama_backend_init();
        qDebug() << "[LlamaVLM] Backend initialized";
        
        // Load main model
        llama_model_params model_params = llama_model_default_params();
        model_params.n_gpu_layers = config.nGPULayers; // Optimized for Mac M4: 5 layers
        
        std::string modelPath = config.modelPath.toStdString();
        m_model = llama_load_model_from_file(modelPath.c_str(), model_params);
        
        if (!m_model) {
            m_lastError = QString("Failed to load model: %1").arg(config.modelPath);
            qWarning() << "[LlamaVLM]" << m_lastError;
            return false;
        }
        
        qDebug() << "[LlamaVLM] Model loaded:" << config.modelPath;
        
        // Create context
        llama_context_params ctx_params = llama_context_default_params();
        ctx_params.n_ctx = config.contextSize;
        ctx_params.n_threads = config.nThreads;
        ctx_params.n_threads_batch = config.nThreads;
        
        m_ctx = llama_init_from_model(m_model, ctx_params);
        
        if (!m_ctx) {
            m_lastError = "Failed to create context";
            qWarning() << "[LlamaVLM]" << m_lastError;
            llama_model_free(m_model);
            m_model = nullptr;
            return false;
        }
        
        qDebug() << "[LlamaVLM] Context created with" << config.contextSize << "tokens";
        
        // Load vision projector (mmproj) using mtmd
        std::string mmprojPath = config.mmprojPath.toStdString();
        mtmd_context_params mtmd_params = mtmd_context_params_default();
        mtmd_params.use_gpu = (config.nGPULayers > 0);
        mtmd_params.n_threads = config.nThreads;
        
        m_mtmdCtx = mtmd_init_from_file(mmprojPath.c_str(), m_model, mtmd_params);
        
        if (!m_mtmdCtx) {
            m_lastError = QString("Failed to load mmproj: %1").arg(config.mmprojPath);
            qWarning() << "[LlamaVLM]" << m_lastError;
            llama_free(m_ctx);
            llama_model_free(m_model);
            m_ctx = nullptr;
            m_model = nullptr;
            return false;
        }
        
        qDebug() << "[LlamaVLM] Vision projector loaded:" << config.mmprojPath;
        
        m_initialized = true;
        qDebug() << "[LlamaVLM] ✅ Initialization complete";
        return true;
        
    } catch (const std::exception& e) {
        m_lastError = QString("Initialization error: %1").arg(e.what());
        qWarning() << "[LlamaVLM]" << m_lastError;
        return false;
    }
}

std::optional<QString> LlamaVLM::generateCaption(const QImage& image) {
    return runInference(image, "Describe this image in one sentence.");
}

std::optional<QString> LlamaVLM::answerQuestion(const QImage& image, const QString& question) {
    QString prompt = QString("Question: %1\nAnswer:").arg(question);
    return runInference(image, prompt);
}

std::optional<QString> LlamaVLM::analyzeImage(const QImage& image, bool includeKeywords) {
    QString prompt = includeKeywords 
        ? "Provide a detailed description of this image including key objects, colors, composition, and mood. Also list 5-10 relevant keywords."
        : "Provide a detailed description of this image including key objects, colors, composition, and mood.";
    
    return runInference(image, prompt);
}

std::optional<QString> LlamaVLM::runInference(const QImage& image, const QString& prompt) {
    if (!m_initialized) {
        m_lastError = "Model not initialized";
        qWarning() << "[LlamaVLM] ERROR:" << m_lastError;
        return std::nullopt;
    }
    
    if (image.isNull()) {
        m_lastError = "Invalid image";
        qWarning() << "[LlamaVLM] ERROR:" << m_lastError;
        return std::nullopt;
    }
    
    qDebug() << "[LlamaVLM] Running inference with prompt:" << prompt;
    qDebug() << "[LlamaVLM] Image size:" << image.size() << "format:" << image.format();
    
    try {
        // Resize image if too large (prevents OOM on Mac M4)
        QImage processedImage = image;
        const int MAX_DIM = 512;
        if (image.width() > MAX_DIM || image.height() > MAX_DIM) {
            qDebug() << "[LlamaVLM] Resizing image from" << image.size() << "to max" << MAX_DIM;
            processedImage = image.scaled(MAX_DIM, MAX_DIM, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        
        // Convert QImage to RGB format for mtmd_bitmap
        QImage rgbImage = processedImage.convertToFormat(QImage::Format_RGB888);
        
        // Create mtmd_bitmap from image data
        mtmd_bitmap * bitmap = mtmd_bitmap_init(
            rgbImage.width(),
            rgbImage.height(),
            rgbImage.constBits()
        );
        
        if (!bitmap) {
            m_lastError = "Failed to create bitmap";
            qWarning() << "[LlamaVLM] ERROR:" << m_lastError;
            return std::nullopt;
        }
        
        qDebug() << "[LlamaVLM] Bitmap created:" << rgbImage.width() << "x" << rgbImage.height();
        
        // Create prompt with image marker
        QString fullPrompt = QString("<__media__> ") + prompt;
        std::string promptStr = fullPrompt.toStdString();
        
        qDebug() << "[LlamaVLM] Full prompt:" << fullPrompt;
        
        // Prepare input text
        mtmd_input_text input_text;
        input_text.text = promptStr.c_str();
        input_text.add_special = true;
        input_text.parse_special = true;
        
        // Tokenize prompt with image
        mtmd_input_chunks * chunks = mtmd_input_chunks_init();
        const mtmd_bitmap * bitmaps[] = {bitmap};
        
        qDebug() << "[LlamaVLM] Starting tokenization...";
        int32_t tokenize_result = mtmd_tokenize(m_mtmdCtx, chunks, &input_text, bitmaps, 1);
        
        if (tokenize_result != 0) {
            m_lastError = QString("Tokenization failed with code: %1").arg(tokenize_result);
            qWarning() << "[LlamaVLM] ERROR:" << m_lastError;
            mtmd_bitmap_free(bitmap);
            mtmd_input_chunks_free(chunks);
            return std::nullopt;
        }
        
        qDebug() << "[LlamaVLM] Tokenized" << mtmd_input_chunks_size(chunks) << "chunks";
        
        // CRITICAL: Clear memory/KV cache before new inference to prevent context overflow
        llama_memory_t mem = llama_get_memory(m_ctx);
        llama_memory_clear(mem, true);  // true = also clear data tensors
        qDebug() << "[LlamaVLM] Memory cache cleared for new inference";
        
        // Use mtmd_helper to evaluate all chunks (handles both text and image)
        llama_pos n_past = 0;
        llama_pos new_n_past = 0;
        
        int32_t eval_result = mtmd_helper_eval_chunks(
            m_mtmdCtx,
            m_ctx,
            chunks,
            n_past,           // starting position
            0,                // sequence id
            m_config.contextSize,  // batch size
            true,             // get logits for last token
            &new_n_past
        );
        
        if (eval_result != 0) {
            m_lastError = QString("Failed to evaluate chunks: error code %1").arg(eval_result);
            qWarning() << "[LlamaVLM] ERROR:" << m_lastError;
            mtmd_bitmap_free(bitmap);
            mtmd_input_chunks_free(chunks);
            return std::nullopt;
        }
        
        n_past = new_n_past;
        qDebug() << "[LlamaVLM] Chunks evaluated, n_past:" << n_past;
        
        qDebug() << "[LlamaVLM] Prompt processed, generating response...";
        
        // Create sampler for greedy decoding
        llama_sampler * sampler = llama_sampler_init_greedy();
        const llama_vocab * vocab = llama_model_get_vocab(m_model);
        
        // Create batch for generation
        llama_batch batch = llama_batch_init(m_config.contextSize, 0, 1);
        
        qDebug() << "[LlamaVLM] Starting token generation (max:" << m_config.maxTokens << ")";
        
        // Generate tokens
        QString response;
        int n_generated = 0;
        const int max_gen = m_config.maxTokens;
        
        while (n_generated < max_gen) {
            // Sample next token using modern API - use -1 for last token in context
            llama_token new_token = llama_sampler_sample(sampler, m_ctx, -1);
            
            qDebug() << "[LlamaVLM] Generated token #" << n_generated << ":" << new_token;
            
            // Check for EOS using vocab
            if (llama_vocab_is_eog(vocab, new_token)) {
                qDebug() << "[LlamaVLM] EOS token encountered, stopping generation";
                break;
            }
            
            // Convert token to text using vocab
            char buf[256];
            int n = llama_token_to_piece(vocab, new_token, buf, sizeof(buf), 0, false);
            if (n > 0) {
                response.append(QString::fromUtf8(buf, n));
            }
            
            // Prepare next iteration
            common_batch_clear(batch);
            common_batch_add(batch, new_token, n_past++, {0}, true);
            
            if (llama_decode(m_ctx, batch) != 0) {
                qWarning() << "[LlamaVLM] Decode failed during generation at token" << n_generated;
                break;
            }
            
            n_generated++;
        }
        
        llama_sampler_free(sampler);
        llama_batch_free(batch);
        mtmd_bitmap_free(bitmap);
        mtmd_input_chunks_free(chunks);
        
        qDebug() << "[LlamaVLM] Generated" << n_generated << "tokens";
        
        return response.trimmed();
        
    } catch (const std::exception& e) {
        m_lastError = QString("Inference error: %1").arg(e.what());
        qWarning() << "[LlamaVLM]" << m_lastError;
        return std::nullopt;
    }
}

bool LlamaVLM::encodeImage(const QImage& image) {
    // This method is no longer used with mtmd API
    // Image encoding is now handled in runInference via mtmd_tokenize
    qDebug() << "[LlamaVLM] encodeImage() is deprecated with mtmd API";
    return true;
}

std::string LlamaVLM::sampleTokens(int maxTokens) {
    // This method is no longer used - sampling is done in runInference
    return "";
}

} // namespace PhotoGuru
