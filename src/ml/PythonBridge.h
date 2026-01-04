#pragma once

#include "PhotoMetadata.h"
#include <QObject>
#include <QString>
#include <memory>

// Forward declare pybind11 types
namespace pybind11 {
    class scoped_interpreter;
    class module_;
}

namespace PhotoGuru {

/**
 * Bridge to Python ML backend (agent_v2.py)
 * Embeds Python interpreter and calls CLIP, SKP, and other ML functions
 */
class PythonBridge : public QObject {
    Q_OBJECT
    
public:
    static PythonBridge& instance();
    
    // Initialize Python interpreter and load agent_v2.py
    bool initialize(const QString& agentScriptPath);
    
    // Shutdown Python interpreter
    void shutdown();
    
    // Check if initialized
    bool isInitialized() const { return m_initialized; }
    
    // Run CLIP analysis on image
    struct ClipResult {
        std::vector<float> embedding;
        QStringList features;
        bool success = false;
    };
    ClipResult runClipAnalysis(const QString& imagePath);
    
    // Run full LLM analysis with context
    struct LlmResult {
        QString title;
        QString description;
        QStringList keywords;
        QString category;
        QString scene;
        QString mood;
        bool success = false;
    };
    LlmResult runLlmAnalysis(const QString& imagePath, 
                            const QJsonObject& context = {});
    
    // Generate semantic key for image
    struct SemanticKeyResult {
        SemanticKeyData key;
        bool success = false;
    };
    SemanticKeyResult generateSemanticKey(const std::vector<float>& embedding,
                                          const QString& role = "anchor");
    
    // Search images by semantic query
    struct SearchResult {
        QString filepath;
        double alignment_score;
    };
    std::vector<SearchResult> semanticSearch(const QString& query,
                                             const QStringList& imagePaths,
                                             double threshold = 0.5);
    
signals:
    void analysisProgress(const QString& message, int percent);
    void analysisComplete(bool success);
    void error(const QString& message);
    
private:
    PythonBridge() = default;
    ~PythonBridge();
    PythonBridge(const PythonBridge&) = delete;
    PythonBridge& operator=(const PythonBridge&) = delete;
    
    bool m_initialized = false;
    std::unique_ptr<pybind11::scoped_interpreter> m_interpreter;
    std::unique_ptr<pybind11::module_> m_agentModule;
};

} // namespace PhotoGuru
