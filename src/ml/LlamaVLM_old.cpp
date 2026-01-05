#include "LlamaVLM.h"
#include "llama.h"
#include "mtmd.h"
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
        llama_free_model(m_model);
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
        
        m_ctx = llama_new_context_with_model(m_model, ctx_params);
        
        if (!m_ctx) {
            m_lastError = "Failed to create context";
            qWarning() << "[LlamaVLM]" << m_lastError;
            llama_free_model(m_model);
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
            llama_free_model(m_model);
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
        return std::nullopt;
    }
    
    qDebug() << "[LlamaVLM] Running inference with prompt:" << prompt;
    
    try {
        // Encode image
        if (!encodeImage(image)) {
            return std::nullopt;
        }
        
        // Tokenize prompt
        std::string promptStr = prompt.toStdString();
        std::vector<llama_token> tokens;
        tokens.resize(promptStr.size() + 256);
        
        int n_tokens = llama_tokenize(
            m_model,
            promptStr.c_str(),
            promptStr.size(),
            tokens.data(),
            tokens.size(),
            true,  // add_bos
            false  // special
        );
        
        if (n_tokens < 0) {
            m_lastError = "Failed to tokenize prompt";
            return std::nullopt;
        }
        
        tokens.resize(n_tokens);
        qDebug() << "[LlamaVLM] Tokenized" << n_tokens << "tokens";
        
        // Create batch for processing
        llama_batch batch = llama_batch_init(n_tokens, 0, 1);
        
        // Add tokens to batch
        for (int i = 0; i < n_tokens; i++) {
            llama_batch_add(batch, tokens[i], i, {0}, false);
        }
        batch.logits[batch.n_tokens - 1] = true; // Only compute logits for last token
        
        // Decode the prompt
        if (llama_decode(m_ctx, batch) != 0) {
            llama_batch_free(batch);
            m_lastError = "Failed to decode prompt";
            return std::nullopt;
        }
        
        // Generate tokens
        QString response;
        int n_generated = 0;
        const int max_gen = m_config.maxTokens;
        
        while (n_generated < max_gen) {
            // Sample next token
            auto* logits = llama_get_logits_ith(m_ctx, batch.n_tokens - 1);
            if (!logits) break;
            
            const int n_vocab = llama_n_vocab(m_model);
            std::vector<llama_token_data> candidates;
            candidates.reserve(n_vocab);
            
            for (int i = 0; i < n_vocab; i++) {
                candidates.push_back({i, logits[i], 0.0f});
            }
            
            llama_token_data_array candidates_p = {
                candidates.data(),
                candidates.size(),
                false
            };
            
            // Apply temperature and sample
            llama_token new_token = llama_sample_token_greedy(m_ctx, &candidates_p);
            
            // Check for EOS
            if (llama_token_is_eog(m_model, new_token)) {
                break;
            }
            
            // Convert token to text
            char buf[256];
            int n = llama_token_to_piece(m_model, new_token, buf, sizeof(buf), 0, false);
            if (n > 0) {
                response.append(QString::fromUtf8(buf, n));
            }
            
            // Prepare next iteration
            llama_batch_clear(batch);
            llama_batch_add(batch, new_token, batch.n_tokens, {0}, true);
            
            if (llama_decode(m_ctx, batch) != 0) {
                break;
            }
            
            n_generated++;
        }
        
        llama_batch_free(batch);
        qDebug() << "[LlamaVLM] Generated" << n_generated << "tokens";
        
        return response.trimmed();
        
    } catch (const std::exception& e) {
        m_lastError = QString("Inference error: %1").arg(e.what());
        qWarning() << "[LlamaVLM]" << m_lastError;
        return std::nullopt;
    }
}

bool LlamaVLM::encodeImage(const QImage& image) {
    if (!m_clipCtx) {
        m_lastError = "Vision projector not loaded";
        return false;
    }
    
    QImage processedImage = image;
    
    // Resize if too large (prevents OOM on Mac M4)
    const int MAX_DIM = 512;
    if (image.width() > MAX_DIM || image.height() > MAX_DIM) {
        qDebug() << "[LlamaVLM] Resizing image from" << image.size() << "to max" << MAX_DIM;
        processedImage = image.scaled(MAX_DIM, MAX_DIM, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Convert QImage to temporary file (llava expects file path)
    QString tempPath = "/tmp/photoguru_temp_image.jpg";
    if (!processedImage.save(tempPath, "JPG", 95)) {
        m_lastError = "Failed to save temporary image";
        return false;
    }
    
    // Load and encode image with llava
    auto img_embed = llava_image_embed_make_with_filename(m_clipCtx, tempPath.toStdString().c_str());
    if (!img_embed) {
        m_lastError = "Failed to encode image with llava";
        return false;
    }
    
    // Evaluate image embedding (add to context)
    if (!llava_eval_image_embed(m_ctx, img_embed, 1, nullptr)) {
        llava_image_embed_free(img_embed);
        m_lastError = "Failed to evaluate image embedding";
        return false;
    }
    
    llava_image_embed_free(img_embed);
    qDebug() << "[LlamaVLM] Image encoded successfully";
    return true;
}

std::string LlamaVLM::sampleTokens(int maxTokens) {
    // TODO: Implement token sampling loop
    return "";
}

} // namespace PhotoGuru
