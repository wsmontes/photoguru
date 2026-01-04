#include "PythonBridge.h"

// Fix Python.h/Qt conflict with "slots" keyword
#ifdef slots
#undef slots
#endif

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Restore Qt slots after pybind11 includes
#define slots Q_SLOTS

#include <QDebug>
#include <QFileInfo>

namespace py = pybind11;

namespace PhotoGuru {

PythonBridge& PythonBridge::instance() {
    static PythonBridge bridge;
    return bridge;
}

PythonBridge::~PythonBridge() {
    shutdown();
}

bool PythonBridge::initialize(const QString& agentScriptPath) {
    if (m_initialized) {
        qWarning() << "PythonBridge already initialized";
        return true;
    }
    
    try {
        // Set up Python path to use venv if it exists
        QFileInfo scriptInfo(agentScriptPath);
        QString scriptDir = scriptInfo.absolutePath();
        QString venvPath = scriptDir + "/.venv";
        
        // Set VIRTUAL_ENV to use venv
        if (QFileInfo::exists(venvPath)) {
            qDebug() << "Found venv at:" << venvPath;
            qputenv("VIRTUAL_ENV", venvPath.toUtf8());
            
            // Update PATH to use venv bin
            QString venvBin = venvPath + "/bin";
            QByteArray currentPath = qgetenv("PATH");
            QString newPath = venvBin + ":" + QString::fromUtf8(currentPath);
            qputenv("PATH", newPath.toUtf8());
        }
        
        qDebug() << "Initializing Python interpreter...";
        m_interpreter = std::make_unique<py::scoped_interpreter>();
        
        // Add agent script directory to sys.path
        py::module_ sys = py::module_::import("sys");
        py::list path = sys.attr("path");
        path.insert(0, scriptDir.toStdString());
        
        qDebug() << "Python version:" << QString::fromStdString(py::str(sys.attr("version")));
        qDebug() << "Python executable:" << QString::fromStdString(py::str(sys.attr("executable")));
        
        qDebug() << "Loading agent_v2 module from:" << scriptDir;
        
        // Import agent module
        QString moduleName = scriptInfo.baseName();  // "agent_v2"
        m_agentModule = std::make_unique<py::module_>(
            py::module_::import(moduleName.toStdString().c_str())
        );
        
        qDebug() << "Python bridge initialized successfully";
        m_initialized = true;
        return true;
        
    } catch (const py::error_already_set& e) {
        qCritical() << "Python initialization error:" << e.what();
        emit error(QString("Python error: %1").arg(e.what()));
        return false;
    } catch (const std::exception& e) {
        qCritical() << "Exception during Python initialization:" << e.what();
        emit error(QString("Exception: %1").arg(e.what()));
        return false;
    }
}

void PythonBridge::shutdown() {
    if (!m_initialized) return;
    
    qDebug() << "Shutting down Python interpreter...";
    m_agentModule.reset();
    m_interpreter.reset();
    m_initialized = false;
}

PythonBridge::ClipResult PythonBridge::runClipAnalysis(const QString& imagePath) {
    ClipResult result;
    
    if (!m_initialized) {
        qWarning() << "PythonBridge not initialized";
        return result;
    }
    
    try {
        // Create CLIPAnalyzer instance
        py::object clip_analyzer_class = m_agentModule->attr("CLIPAnalyzer");
        py::object analyzer = clip_analyzer_class();
        
        // Call analyze_image(image_path)
        py::object clip_result = analyzer.attr("analyze_image")(
            imagePath.toStdString()
        );
        
        // Extract embedding (numpy array -> std::vector)
        if (py::hasattr(clip_result, "embedding")) {
            py::object embedding_obj = clip_result.attr("embedding");
            py::array_t<float> embedding = embedding_obj.cast<py::array_t<float>>();
            auto buf = embedding.request();
            float* ptr = static_cast<float*>(buf.ptr);
            result.embedding = std::vector<float>(ptr, ptr + buf.size);
        }
        
        // Extract features (stored as JSON string)
        if (py::hasattr(clip_result, "features")) {
            py::object features = clip_result.attr("features");
            result.features << QString::fromStdString(py::str(features));
        }
        
        result.success = true;
        
    } catch (const py::error_already_set& e) {
        qWarning() << "Python CLIP analysis error:" << e.what();
        emit error(QString("CLIP error: %1").arg(e.what()));
    }
    
    return result;
}

PythonBridge::LlmResult PythonBridge::runLlmAnalysis(const QString& imagePath,
                                                     const QJsonObject& context) {
    LlmResult result;
    
    if (!m_initialized) {
        qWarning() << "PythonBridge not initialized";
        return result;
    }
    
    try {
        emit analysisProgress("Running LLM analysis...", 50);
        
        // Convert QJsonObject to Python dict
        py::dict py_context;
        if (!context.isEmpty()) {
            for (auto it = context.begin(); it != context.end(); ++it) {
                QString key = it.key();
                QJsonValue val = it.value();
                
                if (val.isString()) {
                    py_context[key.toUtf8().constData()] = val.toString().toStdString();
                } else if (val.isDouble()) {
                    py_context[key.toUtf8().constData()] = val.toDouble();
                }
            }
        }
        
        // Create PhotoContextAnalyzer and analyze single photo
        py::object analyzer_class = m_agentModule->attr("PhotoContextAnalyzer");
        py::object analyzer = analyzer_class();
        
        // Load image and analyze
        py::object pil_image_class = py::module_::import("PIL.Image");
        py::object img = pil_image_class.attr("open")(imagePath.toStdString());
        
        // For now, return placeholder - full integration needs PhotoMetadata creation
        py::dict llm_result;
        
        // Extract results
        if (py::hasattr(llm_result, "title")) {
            result.title = QString::fromStdString(
                py::str(llm_result.attr("title"))
            );
        }
        
        if (py::hasattr(llm_result, "description")) {
            result.description = QString::fromStdString(
                py::str(llm_result.attr("description"))
            );
        }
        
        if (py::hasattr(llm_result, "keywords")) {
            py::list keywords = llm_result.attr("keywords");
            for (const auto& kw : keywords) {
                result.keywords << QString::fromStdString(py::str(kw));
            }
        }
        
        if (py::hasattr(llm_result, "category")) {
            result.category = QString::fromStdString(
                py::str(llm_result.attr("category"))
            );
        }
        
        if (py::hasattr(llm_result, "scene")) {
            result.scene = QString::fromStdString(
                py::str(llm_result.attr("scene"))
            );
        }
        
        if (py::hasattr(llm_result, "mood")) {
            result.mood = QString::fromStdString(
                py::str(llm_result.attr("mood"))
            );
        }
        
        result.success = true;
        emit analysisProgress("LLM analysis complete", 100);
        
    } catch (const py::error_already_set& e) {
        qWarning() << "Python LLM analysis error:" << e.what();
        emit error(QString("LLM error: %1").arg(e.what()));
    }
    
    return result;
}

PythonBridge::SemanticKeyResult PythonBridge::generateSemanticKey(
    const std::vector<float>& embedding,
    const QString& role) {
    
    SemanticKeyResult result;
    
    if (!m_initialized) {
        qWarning() << "PythonBridge not initialized";
        return result;
    }
    
    try {
        // Convert vector to numpy array
        py::array_t<float> py_embedding(static_cast<py::ssize_t>(embedding.size()));
        auto buf = py_embedding.request();
        float* ptr = static_cast<float*>(buf.ptr);
        std::copy(embedding.begin(), embedding.end(), ptr);
        
        // Create SemanticKey
        py::object sk_class = m_agentModule->attr("SemanticKey");
        py::object key = sk_class(
            py_embedding,
            py::str(),  // auto-generate ID
            role.toStdString()
        );
        
        // Extract data
        result.key.key_id = QString::fromStdString(
            py::str(key.attr("key_id"))
        );
        result.key.role = QString::fromStdString(
            py::str(key.attr("role"))
        );
        
        result.success = true;
        
    } catch (const py::error_already_set& e) {
        qWarning() << "Python semantic key error:" << e.what();
    }
    
    return result;
}

std::vector<PythonBridge::SearchResult> PythonBridge::semanticSearch(
    const QString& query,
    const QStringList& imagePaths,
    double threshold) {
    
    std::vector<SearchResult> results;
    
    if (!m_initialized) {
        qWarning() << "PythonBridge not initialized";
        return results;
    }
    
    try {
        // Convert QStringList to Python list
        py::list py_paths;
        for (const QString& path : imagePaths) {
            py_paths.append(path.toStdString());
        }
        
        // Call semantic search
        py::object search_func = m_agentModule->attr("semantic_search");
        py::list search_results = search_func(
            query.toStdString(),
            py_paths,
            threshold
        );
        
        // Parse results
        for (const auto& item : search_results) {
            SearchResult sr;
            sr.filepath = QString::fromStdString(
                py::str(item.attr("filepath"))
            );
            sr.alignment_score = py::cast<double>(item.attr("score"));
            results.push_back(sr);
        }
        
    } catch (const py::error_already_set& e) {
        qWarning() << "Python semantic search error:" << e.what();
        emit error(QString("Search error: %1").arg(e.what()));
    }
    
    return results;
}

} // namespace PhotoGuru
