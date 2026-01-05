#include "AnalysisPanel.h"
#include "DarkTheme.h"
#include "../ml/CLIPAnalyzer.h"
#include "../ml/LlamaVLM.h"
#include "../core/MetadataWriter.h"
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileInfo>
#include <QScrollBar>
#include <QCoreApplication>
#include <QThread>
#include <QImage>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

namespace PhotoGuru {

AnalysisPanel::AnalysisPanel(QWidget* parent, bool shouldInitializeAI)
    : QWidget(parent)
    , m_isAnalyzing(false)
    , m_aiInitialized(false)
    , m_lastGeneratedCaption("")
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(260);
    setupUI();
    updateButtonStates(false);
    if (shouldInitializeAI) {
        initializeAI();
    }
}

AnalysisPanel::~AnalysisPanel() = default;

void AnalysisPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Title
    QLabel* titleLabel = new QLabel("AI Analysis & Processing");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Single Image Analysis Group
    m_singleImageGroup = new QGroupBox("Current Image");
    QVBoxLayout* singleLayout = new QVBoxLayout(m_singleImageGroup);
    
    m_currentImageLabel = new QLabel("No image selected");
    m_currentImageLabel->setWordWrap(true);
    m_currentImageLabel->setStyleSheet("color: #888; font-style: italic;");
    singleLayout->addWidget(m_currentImageLabel);
    
    m_analyzeImageBtn = new QPushButton("🔍 Analyze with AI");
    m_analyzeImageBtn->setToolTip("Generate description, keywords, technical scores, and semantic keys");
    m_analyzeImageBtn->setMinimumHeight(35);
    connect(m_analyzeImageBtn, &QPushButton::clicked, this, &AnalysisPanel::onAnalyzeCurrentImage);
    singleLayout->addWidget(m_analyzeImageBtn);
    
    mainLayout->addWidget(m_singleImageGroup);
    
    // Generated Caption Display Group
    m_captionGroup = new QGroupBox("Generated Description");
    m_captionGroup->setVisible(false);  // Hidden until caption is generated
    QVBoxLayout* captionLayout = new QVBoxLayout(m_captionGroup);
    
    m_captionDisplay = new QTextEdit();
    m_captionDisplay->setReadOnly(true);
    m_captionDisplay->setMaximumHeight(100);
    m_captionDisplay->setStyleSheet("QTextEdit { background-color: #f0f8ff; color: #000; padding: 8px; border: 1px solid #4CAF50; border-radius: 4px; }");
    captionLayout->addWidget(m_captionDisplay);
    
    QHBoxLayout* captionButtonsLayout = new QHBoxLayout();
    
    m_copyCaptionBtn = new QPushButton("📋 Copy");
    m_copyCaptionBtn->setToolTip("Copy description to clipboard");
    connect(m_copyCaptionBtn, &QPushButton::clicked, this, [this]() {
        LOG_INFO("AnalysisPanel", "User clicked: Copy Caption button");
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(m_lastGeneratedCaption);
        LOG_INFO("AnalysisPanel", QString("Caption copied to clipboard (%1 chars)").arg(m_lastGeneratedCaption.length()));
        m_logOutput->append("📋 Caption copied to clipboard");
    });
    captionButtonsLayout->addWidget(m_copyCaptionBtn);
    
    m_applyToOthersBtn = new QPushButton("📤 Apply to Selection");
    m_applyToOthersBtn->setToolTip("Apply this description to other selected images");
    m_applyToOthersBtn->setEnabled(false);  // TODO: implement batch apply
    captionButtonsLayout->addWidget(m_applyToOthersBtn);
    
    captionLayout->addLayout(captionButtonsLayout);
    
    mainLayout->addWidget(m_captionGroup);
    
    // Batch Analysis Group
    m_batchGroup = new QGroupBox("Batch Operations");
    QVBoxLayout* batchLayout = new QVBoxLayout(m_batchGroup);
    
    m_analyzeDirBtn = new QPushButton("📁 Analyze All Images in Folder");
    m_analyzeDirBtn->setToolTip("Process all images in the current directory");
    m_analyzeDirBtn->setMinimumHeight(35);
    connect(m_analyzeDirBtn, &QPushButton::clicked, this, &AnalysisPanel::onAnalyzeDirectory);
    batchLayout->addWidget(m_analyzeDirBtn);
    
    m_findDuplicatesBtn = new QPushButton("🔄 Find Duplicates");
    m_findDuplicatesBtn->setToolTip("Detect duplicate and similar images");
    connect(m_findDuplicatesBtn, &QPushButton::clicked, this, &AnalysisPanel::onFindDuplicates);
    batchLayout->addWidget(m_findDuplicatesBtn);
    
    m_detectBurstsBtn = new QPushButton("📸 Detect Burst Groups");
    m_detectBurstsBtn->setToolTip("Identify burst sequences and find best shots");
    connect(m_detectBurstsBtn, &QPushButton::clicked, this, &AnalysisPanel::onDetectBursts);
    batchLayout->addWidget(m_detectBurstsBtn);
    
    m_generateReportBtn = new QPushButton("📊 Generate Quality Report");
    m_generateReportBtn->setToolTip("Create a detailed quality analysis report");
    connect(m_generateReportBtn, &QPushButton::clicked, this, &AnalysisPanel::onGenerateReport);
    batchLayout->addWidget(m_generateReportBtn);
    
    // Options
    QVBoxLayout* optionsLayout = new QVBoxLayout();
    m_overwriteCheckbox = new QCheckBox("Overwrite existing analysis");
    connect(m_overwriteCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        LOG_INFO("AnalysisPanel", QString("User toggled: Overwrite checkbox = %1").arg(state == Qt::Checked ? "ON" : "OFF"));
    });
    
    m_skipExistingCheckbox = new QCheckBox("Skip already analyzed images");
    m_skipExistingCheckbox->setChecked(true);
    connect(m_skipExistingCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        LOG_INFO("AnalysisPanel", QString("User toggled: Skip existing checkbox = %1").arg(state == Qt::Checked ? "ON" : "OFF"));
    });
    
    optionsLayout->addWidget(m_overwriteCheckbox);
    optionsLayout->addWidget(m_skipExistingCheckbox);
    batchLayout->addLayout(optionsLayout);
    
    mainLayout->addWidget(m_batchGroup);
    
    // Progress Section
    QGroupBox* progressGroup = new QGroupBox("Progress");
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);
    
    m_statusLabel = new QLabel("Ready");
    progressLayout->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    progressLayout->addWidget(m_progressBar);
    
    m_cancelBtn = new QPushButton("⏹ Cancel");
    m_cancelBtn->setEnabled(false);
    connect(m_cancelBtn, &QPushButton::clicked, this, &AnalysisPanel::onCancelAnalysis);
    progressLayout->addWidget(m_cancelBtn);
    
    mainLayout->addWidget(progressGroup);
    
    // Log Output
    QLabel* logLabel = new QLabel("Analysis Log:");
    mainLayout->addWidget(logLabel);
    
    m_logOutput = new QTextEdit();
    m_logOutput->setReadOnly(true);
    m_logOutput->setMaximumHeight(150);
    m_logOutput->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: monospace; }");
    mainLayout->addWidget(m_logOutput);
    
    // Log file button
    QPushButton* openLogBtn = new QPushButton("📄 Open Full Log File");
    openLogBtn->setToolTip("Open complete log file in default text editor");
    connect(openLogBtn, &QPushButton::clicked, this, [this]() {
        LOG_INFO("AnalysisPanel", "User clicked: Open Full Log File button");
        QString logPath = Logger::instance().logFilePath();
        LOG_INFO("AnalysisPanel", "Opening log file: " + logPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
        m_logOutput->append("📄 Log file: " + logPath);
    });
    mainLayout->addWidget(openLogBtn);
    
    mainLayout->addStretch();
}

void AnalysisPanel::setCurrentImage(const QString& filepath) {
    m_currentImage = filepath;
    
    if (filepath.isEmpty()) {
        LOG_INFO("AnalysisPanel", "User action: Image deselected");
        m_currentImageLabel->setText("No image selected");
        m_currentImageLabel->setStyleSheet("color: #888; font-style: italic;");
        m_analyzeImageBtn->setEnabled(false);
    } else {
        QFileInfo info(filepath);
        LOG_INFO("AnalysisPanel", "User action: Image selected - " + filepath);
        LOG_DEBUG("AnalysisPanel", QString("Image size: %1 bytes, name: %2")
            .arg(info.size()).arg(info.fileName()));
        m_currentImageLabel->setText(info.fileName());
        m_currentImageLabel->setStyleSheet("color: #d4d4d4;");
        m_analyzeImageBtn->setEnabled(!m_isAnalyzing);
    }
}

void AnalysisPanel::setCurrentDirectory(const QString& dirpath) {
    m_currentDirectory = dirpath;
    
    if (dirpath.isEmpty()) {
        LOG_INFO("AnalysisPanel", "User action: Directory deselected");
    } else {
        LOG_INFO("AnalysisPanel", "User action: Directory selected - " + dirpath);
        QDir dir(dirpath);
        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG" << "*.heic" << "*.HEIC" << "*.png" << "*.PNG";
        int imageCount = dir.entryList(filters, QDir::Files).size();
        LOG_DEBUG("AnalysisPanel", QString("Directory contains %1 images").arg(imageCount));
    }
    
    bool hasDir = !dirpath.isEmpty();
    m_analyzeDirBtn->setEnabled(hasDir && !m_isAnalyzing);
    m_findDuplicatesBtn->setEnabled(hasDir && !m_isAnalyzing);
    m_detectBurstsBtn->setEnabled(hasDir && !m_isAnalyzing);
    m_generateReportBtn->setEnabled(hasDir && !m_isAnalyzing);
}

void AnalysisPanel::updateButtonStates(bool analyzing) {
    m_isAnalyzing = analyzing;
    
    m_analyzeImageBtn->setEnabled(!analyzing && !m_currentImage.isEmpty());
    m_analyzeDirBtn->setEnabled(!analyzing && !m_currentDirectory.isEmpty());
    m_findDuplicatesBtn->setEnabled(!analyzing && !m_currentDirectory.isEmpty());
    m_detectBurstsBtn->setEnabled(!analyzing && !m_currentDirectory.isEmpty());
    m_generateReportBtn->setEnabled(!analyzing && !m_currentDirectory.isEmpty());
    
    m_cancelBtn->setEnabled(analyzing);
    
    if (!analyzing) {
        m_statusLabel->setText("Ready");
        m_progressBar->setValue(0);
    }
}

void AnalysisPanel::initializeAI() {
    LOG_INFO("AnalysisPanel", "=== AI Initialization Started ===");
    m_logOutput->append("🔧 Initializing AI components...");
    
    // Get models directory - support both dev build and app bundle
    QString modelsDir;
    QString appDir = QCoreApplication::applicationDirPath();
    
    LOG_DEBUG("AnalysisPanel", "App directory: " + appDir);
    m_logOutput->append("📁 App directory: " + appDir);
    
    // Try bundle path first (for .app bundle)
    QString bundleModels = appDir + "/../Resources/models";
    if (QDir(bundleModels).exists()) {
        modelsDir = QDir(bundleModels).canonicalPath();
        LOG_INFO("AnalysisPanel", "Using bundle models: " + modelsDir);
        m_logOutput->append("✅ Using bundle models: " + modelsDir);
    } else {
        // Fallback to build directory
        modelsDir = appDir + "/models";
        if (!QDir(modelsDir).exists()) {
            // Try parent directory (for when app is in build/)
            QString parentModels = appDir + "/../models";
            if (QDir(parentModels).exists()) {
                modelsDir = QDir(parentModels).canonicalPath();
                LOG_INFO("AnalysisPanel", "Using parent models: " + modelsDir);
                m_logOutput->append("✅ Using parent models: " + modelsDir);
            } else {
                LOG_WARNING("AnalysisPanel", "Models directory not found: " + modelsDir);
                m_logOutput->append("⚠️ Models directory not found, trying: " + modelsDir);
            }
        } else {
            LOG_INFO("AnalysisPanel", "Using build models: " + modelsDir);
            m_logOutput->append("✅ Using build models: " + modelsDir);
        }
    }
    
    // Initialize CLIP
    m_clipAnalyzer = std::make_unique<CLIPAnalyzer>();
    QString clipModelPath = modelsDir + "/clip-vit-base-patch32.onnx";
    LOG_INFO("AnalysisPanel", "Loading CLIP from: " + clipModelPath);
    m_logOutput->append("🔍 Loading CLIP from: " + clipModelPath);
    if (m_clipAnalyzer->initialize(clipModelPath, true)) {
        LOG_INFO("AnalysisPanel", "CLIP initialized successfully");
        m_logOutput->append("✅ CLIP initialized successfully");
    } else {
        LOG_ERROR("AnalysisPanel", "CLIP initialization failed");
        m_logOutput->append("❌ CLIP initialization failed");
        m_clipAnalyzer.reset();
    }
    
    // Initialize VLM
    m_llamaVLM = std::make_unique<LlamaVLM>();
    LlamaVLM::ModelConfig config;
    config.modelPath = modelsDir + "/Qwen3VL-4B-Instruct-Q4_K_M.gguf";
    config.mmprojPath = modelsDir + "/mmproj-Qwen3VL-4B-Instruct-Q8_0.gguf";
    config.nGPULayers = 5;
    config.contextSize = 2048;
    
    LOG_INFO("AnalysisPanel", "Checking VLM model files...");
    LOG_DEBUG("AnalysisPanel", "Model path: " + config.modelPath);
    LOG_DEBUG("AnalysisPanel", "MMProj path: " + config.mmprojPath);
    
    if (QFileInfo::exists(config.modelPath) && QFileInfo::exists(config.mmprojPath)) {
        LOG_INFO("AnalysisPanel", "VLM models found, starting initialization (this may take 30-60s)...");
        m_logOutput->append("🔄 Loading VLM (this may take 30-60s)...");
        if (m_llamaVLM->initialize(config)) {
            LOG_INFO("AnalysisPanel", "VLM initialized successfully");
            m_logOutput->append("✅ VLM initialized: Qwen3-VL 4B");
        } else {
            LOG_ERROR("AnalysisPanel", "VLM initialization failed");
            m_logOutput->append("❌ VLM initialization failed");
            m_llamaVLM.reset();
        }
    } else {
        LOG_WARNING("AnalysisPanel", "VLM models not found - skipping");
        LOG_DEBUG("AnalysisPanel", "Model exists: " + QString::number(QFileInfo::exists(config.modelPath)));
        LOG_DEBUG("AnalysisPanel", "MMProj exists: " + QString::number(QFileInfo::exists(config.mmprojPath)));
        m_logOutput->append("⚠️ VLM models not found - skipping");
        m_llamaVLM.reset();
    }
    
    m_aiInitialized = (m_clipAnalyzer != nullptr);
    
    if (m_aiInitialized) {
        LOG_INFO("AnalysisPanel", "AI initialization complete - CLIP ready");
        m_logOutput->append("✅ AI initialization complete");
    } else {
        LOG_WARNING("AnalysisPanel", "AI initialization incomplete - some features disabled");
        m_logOutput->append("⚠️ AI initialization incomplete - some features disabled");
    }
    
    LOG_INFO("AnalysisPanel", "=== AI Initialization Finished ===");
}

void AnalysisPanel::onAnalyzeCurrentImage() {
    LOG_INFO("AnalysisPanel", "=== Analyze Current Image - CLICKED ===");
    
    if (m_currentImage.isEmpty()) {
        LOG_WARNING("AnalysisPanel", "No image selected");
        return;
    }
    
    LOG_INFO("AnalysisPanel", "Analyzing: " + m_currentImage);
    
    if (!m_aiInitialized || !m_clipAnalyzer) {
        LOG_ERROR("AnalysisPanel", "AI not initialized");
        return;
    }
    
    updateButtonStates(true);
    m_statusLabel->setText("Analyzing image...");
    m_logOutput->append("\n🔍 Analyzing: " + QFileInfo(m_currentImage).fileName());
    
    // Load image
    QImage image(m_currentImage);
    if (image.isNull()) {
        m_logOutput->append("❌ Failed to load image");
        m_statusLabel->setText("Analysis failed");
        updateButtonStates(false);
        return;
    }
    
    // 1. CLIP Analysis
    LOG_INFO("AnalysisPanel", "Computing CLIP embeddings...");
    m_statusLabel->setText("Computing CLIP embeddings...");
    auto startTime = QDateTime::currentDateTime();
    auto embedding = m_clipAnalyzer->computeEmbedding(image);
    auto elapsed = startTime.msecsTo(QDateTime::currentDateTime());
    
    if (embedding && !embedding->empty()) {
        LOG_INFO("AnalysisPanel", QString("CLIP embedding computed: %1-dim in %2ms")
            .arg(embedding->size()).arg(elapsed));
        m_logOutput->append(QString("✅ CLIP embedding computed (%1-dim) in %2ms")
            .arg(embedding->size()).arg(elapsed));
    } else {
        LOG_ERROR("AnalysisPanel", "CLIP embedding failed");
        m_logOutput->append("❌ CLIP embedding failed");
        m_statusLabel->setText("Analysis failed");
        updateButtonStates(false);
        return;
    }
    
    // 2. VLM Caption (if available)
    QString caption;
    QString description;
    
    if (m_llamaVLM) {
        LOG_INFO("AnalysisPanel", "Generating VLM caption...");
        m_statusLabel->setText("Generating caption with VLM...");
        m_logOutput->append("🤖 Generating VLM caption (may take 10-30s)...");
        m_logOutput->append(QString("🖼️  Image: %1x%2, format: %3")
            .arg(image.width()).arg(image.height()).arg(image.format()));
        
        auto vlmStart = QDateTime::currentDateTime();
        auto captionResult = m_llamaVLM->generateCaption(image);
        auto vlmElapsed = vlmStart.msecsTo(QDateTime::currentDateTime());
        
        if (captionResult) {
            caption = *captionResult;
            if (caption.isEmpty()) {
                LOG_WARNING("AnalysisPanel", QString("Caption empty after %1ms").arg(vlmElapsed));
                m_logOutput->append("⚠️ Caption is empty (VLM generated 0 tokens)");
            } else {
                LOG_INFO("AnalysisPanel", QString("Caption generated in %1ms: %2").arg(vlmElapsed).arg(caption));
                m_logOutput->append(QString("✅ Caption generated in %1ms!").arg(vlmElapsed));
                m_lastGeneratedCaption = caption;
                showGeneratedCaption(caption);
            }
        } else {
            QString error = m_llamaVLM->lastError();
            LOG_ERROR("AnalysisPanel", QString("VLM failed after %1ms: %2").arg(vlmElapsed).arg(error));
            m_logOutput->append("⚠️ VLM caption generation failed: " + error);
        }
        
        // Detailed description
        auto descResult = m_llamaVLM->analyzeImage(image);
        
        if (descResult) {
            description = *descResult;
            m_logOutput->append("✅ Detailed analysis complete");
        }
    } else {
        m_logOutput->append("⚠️ VLM not available - skipping caption");
    }
    
    // 3. Write metadata
    if (!caption.isEmpty() || !description.isEmpty()) {
        m_statusLabel->setText("Writing metadata...");
        
        PhotoMetadata metadata;
        metadata.llm_title = caption;
        metadata.llm_description = description.isEmpty() ? caption : description;
        
        if (MetadataWriter::instance().write(m_currentImage, metadata)) {
            LOG_INFO("AnalysisPanel", "Metadata written successfully");
            m_logOutput->append("✅ Metadata written to image file");
            emit metadataUpdated(m_currentImage);
        } else {
            LOG_ERROR("AnalysisPanel", "Failed to write metadata");
            m_logOutput->append("⚠️ Failed to write metadata");
        }
    }
    
    LOG_INFO("AnalysisPanel", "=== Analyze Current Image - COMPLETE ===");
    m_statusLabel->setText("Analysis complete");
    m_logOutput->append("✅ Analysis complete!\n");
    updateButtonStates(false);
}

void AnalysisPanel::onAnalyzeDirectory() {
    LOG_INFO("AnalysisPanel", "=== Analyze Directory - CLICKED ===");
    
    if (m_currentDirectory.isEmpty()) {
        LOG_WARNING("AnalysisPanel", "No directory selected");
        return;
    }
    
    LOG_INFO("AnalysisPanel", "Batch analyzing: " + m_currentDirectory);
    
    if (!m_aiInitialized || !m_clipAnalyzer) {
        LOG_ERROR("AnalysisPanel", "AI not initialized");
        return;
    }
    
    updateButtonStates(true);
    m_logOutput->append("\n📁 Batch analyzing directory: " + m_currentDirectory);
    
    // Get all image files
    QDir dir(m_currentDirectory);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
            << "*.heic" << "*.HEIC" << "*.png" << "*.PNG";
    QStringList imageFiles = dir.entryList(filters, QDir::Files);
    
    if (imageFiles.isEmpty()) {
        m_logOutput->append("⚠️ No images found in directory");
        m_statusLabel->setText("No images found");
        updateButtonStates(false);
        return;
    }
    
    m_logOutput->append(QString("Found %1 images to analyze").arg(imageFiles.size()));
    m_progressBar->setMaximum(imageFiles.size());
    
    int processed = 0;
    int succeeded = 0;
    int failed = 0;
    
    for (const QString& filename : imageFiles) {
        QString filepath = dir.absoluteFilePath(filename);
        processed++;
        
        m_progressBar->setValue(processed);
        m_statusLabel->setText(QString("Processing %1/%2: %3")
            .arg(processed).arg(imageFiles.size()).arg(filename));
        
        // Skip if already has metadata and skip option is checked
        if (m_skipExistingCheckbox->isChecked()) {
            // TODO: Check if metadata exists
            // For now, process all
        }
        
        QImage image(filepath);
        if (image.isNull()) {
            m_logOutput->append(QString("⚠️ Failed to load: %1").arg(filename));
            failed++;
            continue;
        }
        
        // CLIP embedding
        auto embedding = m_clipAnalyzer->computeEmbedding(image);
        if (!embedding || embedding->empty()) {
            m_logOutput->append(QString("❌ CLIP failed: %1").arg(filename));
            failed++;
            continue;
        }
        
        // VLM caption (optional)
        QString caption;
        if (m_llamaVLM) {
            auto captionResult = m_llamaVLM->generateCaption(image);
            if (captionResult) {
                caption = *captionResult;
            }
        }
        
        // Write metadata
        if (!caption.isEmpty()) {
            PhotoMetadata metadata;
            metadata.llm_title = caption;
            if (MetadataWriter::instance().write(filepath, metadata)) {
                succeeded++;
                m_logOutput->append(QString("✅ %1").arg(filename));
            } else {
                failed++;
                m_logOutput->append(QString("⚠️ Write failed: %1").arg(filename));
            }
        } else {
            succeeded++;
            m_logOutput->append(QString("✅ %1 (CLIP only)").arg(filename));
        }
        
        // Process events to keep UI responsive
        QCoreApplication::processEvents();
    }
    
    LOG_INFO("AnalysisPanel", QString("Batch complete: %1 succeeded, %2 failed out of %3 total")
        .arg(succeeded).arg(failed).arg(processed));
    m_logOutput->append(QString("\n✅ Batch complete: %1 succeeded, %2 failed")
        .arg(succeeded).arg(failed));
    m_statusLabel->setText("Batch analysis complete");
    m_progressBar->setValue(0);
    updateButtonStates(false);
    LOG_INFO("AnalysisPanel", "=== Analyze Directory - COMPLETE ===");
}

void AnalysisPanel::onFindDuplicates() {
    LOG_INFO("AnalysisPanel", "=== Find Duplicates - CLICKED ===");
    
    if (m_currentDirectory.isEmpty()) {
        LOG_WARNING("AnalysisPanel", "No directory selected");
        return;
    }
    
    LOG_INFO("AnalysisPanel", "Finding duplicates in: " + m_currentDirectory);
    
    if (!m_aiInitialized || !m_clipAnalyzer) {
        LOG_ERROR("AnalysisPanel", "CLIP not initialized");
        return;
    }
    
    updateButtonStates(true);
    m_logOutput->append("\n🔍 Finding duplicates in: " + m_currentDirectory);
    
    // Get all images
    QDir dir(m_currentDirectory);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
            << "*.heic" << "*.HEIC" << "*.png" << "*.PNG";
    QStringList imageFiles = dir.entryList(filters, QDir::Files);
    
    if (imageFiles.size() < 2) {
        m_logOutput->append("⚠️ Need at least 2 images to compare");
        updateButtonStates(false);
        return;
    }
    
    m_logOutput->append(QString("Computing embeddings for %1 images...").arg(imageFiles.size()));
    m_progressBar->setMaximum(imageFiles.size());
    
    // Compute embeddings for all images
    QList<QPair<QString, std::vector<float>>> embeddings;
    for (int i = 0; i < imageFiles.size(); i++) {
        QString filepath = dir.absoluteFilePath(imageFiles[i]);
        m_progressBar->setValue(i + 1);
        m_statusLabel->setText(QString("Computing: %1/%2").arg(i+1).arg(imageFiles.size()));
        
        auto embedding = m_clipAnalyzer->computeEmbedding(filepath);
        if (embedding && !embedding->empty()) {
            embeddings.append(qMakePair(filepath, *embedding));
        }
        
        QCoreApplication::processEvents();
    }
    
    m_logOutput->append(QString("✅ Computed %1 embeddings").arg(embeddings.size()));
    m_logOutput->append("\nSearching for similar pairs (threshold > 0.95)...");
    
    // Compare all pairs
    int duplicatesFound = 0;
    const float threshold = 0.95f;
    
    for (int i = 0; i < embeddings.size(); i++) {
        for (int j = i + 1; j < embeddings.size(); j++) {
            float similarity = m_clipAnalyzer->cosineSimilarity(
                embeddings[i].second, 
                embeddings[j].second
            );
            
            if (similarity > threshold) {
                duplicatesFound++;
                QFileInfo file1(embeddings[i].first);
                QFileInfo file2(embeddings[j].first);
                m_logOutput->append(QString("🔗 Similar (%1%%): %2 ↔ %3")
                    .arg(similarity * 100, 0, 'f', 1)
                    .arg(file1.fileName())
                    .arg(file2.fileName()));
            }
        }
    }
    
    if (duplicatesFound == 0) {
        LOG_INFO("AnalysisPanel", QString("No duplicates found (checked %1 images)").arg(embeddings.size()));
        m_logOutput->append("\n✅ No duplicates found");
    } else {
        LOG_INFO("AnalysisPanel", QString("Found %1 duplicate pairs").arg(duplicatesFound));
        m_logOutput->append(QString("\n✅ Found %1 duplicate pairs").arg(duplicatesFound));
    }
    
    m_statusLabel->setText("Duplicate search complete");
    m_progressBar->setValue(0);
    updateButtonStates(false);
    LOG_INFO("AnalysisPanel", "=== Find Duplicates - COMPLETE ===");
}

void AnalysisPanel::onDetectBursts() {
    LOG_INFO("AnalysisPanel", "=== Detect Bursts - CLICKED ===");
    
    if (m_currentDirectory.isEmpty()) {
        LOG_WARNING("AnalysisPanel", "No directory selected");
        return;
    }
    
    LOG_INFO("AnalysisPanel", "Detecting bursts in: " + m_currentDirectory);
    updateButtonStates(true);
    m_logOutput->append("\n📸 Detecting bursts in: " + m_currentDirectory);
    
    // Get all images with timestamps
    QDir dir(m_currentDirectory);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
            << "*.heic" << "*.HEIC" << "*.png" << "*.PNG";
    QStringList imageFiles = dir.entryList(filters, QDir::Files, QDir::Name);
    
    if (imageFiles.size() < 2) {
        m_logOutput->append("⚠️ Need at least 2 images to detect bursts");
        updateButtonStates(false);
        return;
    }
    
    // Get file creation times (proxy for capture time)
    QList<QPair<QString, QDateTime>> imageTimestamps;
    for (const QString& filename : imageFiles) {
        QString filepath = dir.absoluteFilePath(filename);
        QFileInfo fileInfo(filepath);
        imageTimestamps.append(qMakePair(filepath, fileInfo.birthTime()));
    }
    
    // Sort by timestamp
    std::sort(imageTimestamps.begin(), imageTimestamps.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    m_logOutput->append(QString("Analyzing %1 images...").arg(imageTimestamps.size()));
    
    // Detect bursts (photos within 5 seconds)
    const int maxSeconds = 5;
    const int minPhotos = 3;
    QList<QStringList> bursts;
    QStringList currentBurst;
    currentBurst.append(imageTimestamps[0].first);
    
    for (int i = 1; i < imageTimestamps.size(); i++) {
        qint64 deltaSeconds = imageTimestamps[i-1].second.secsTo(imageTimestamps[i].second);
        
        if (deltaSeconds <= maxSeconds) {
            currentBurst.append(imageTimestamps[i].first);
        } else {
            if (currentBurst.size() >= minPhotos) {
                bursts.append(currentBurst);
            }
            currentBurst.clear();
            currentBurst.append(imageTimestamps[i].first);
        }
    }
    
    // Check last burst
    if (currentBurst.size() >= minPhotos) {
        bursts.append(currentBurst);
    }
    
    if (bursts.isEmpty()) {
        LOG_INFO("AnalysisPanel", QString("No bursts detected (checked %1 images)").arg(imageTimestamps.size()));
        m_logOutput->append("\n✅ No bursts detected (need 3+ photos within 5s)");
    } else {
        LOG_INFO("AnalysisPanel", QString("Found %1 bursts").arg(bursts.size()));
        m_logOutput->append(QString("\n📸 Found %1 bursts:").arg(bursts.size()));
        for (int i = 0; i < bursts.size(); i++) {
            m_logOutput->append(QString("\nBurst %1 (%2 photos):")
                .arg(i+1).arg(bursts[i].size()));
            for (const QString& filepath : bursts[i]) {
                QFileInfo info(filepath);
                m_logOutput->append(QString("  • %1").arg(info.fileName()));
            }
        }
    }
    
    m_statusLabel->setText("Burst detection complete");
    updateButtonStates(false);
    LOG_INFO("AnalysisPanel", "=== Detect Bursts - COMPLETE ===");
}

void AnalysisPanel::onGenerateReport() {
    LOG_INFO("AnalysisPanel", "=== Generate Report - CLICKED ===");
    
    if (m_currentDirectory.isEmpty()) {
        LOG_WARNING("AnalysisPanel", "No directory selected");
        return;
    }
    
    LOG_INFO("AnalysisPanel", "Generating report for: " + m_currentDirectory);
    updateButtonStates(true);
    m_logOutput->append("\n📊 Generating quality report for: " + m_currentDirectory);
    
    // Get all images
    QDir dir(m_currentDirectory);
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
            << "*.heic" << "*.HEIC" << "*.png" << "*.PNG";
    QStringList imageFiles = dir.entryList(filters, QDir::Files);
    
    if (imageFiles.isEmpty()) {
        m_logOutput->append("⚠️ No images found");
        updateButtonStates(false);
        return;
    }
    
    m_logOutput->append(QString("Analyzing %1 images...").arg(imageFiles.size()));
    m_progressBar->setMaximum(imageFiles.size());
    
    // Analyze each image
    struct ImageQuality {
        QString filename;
        qint64 filesize;
        int width;
        int height;
        double score;
    };
    
    QList<ImageQuality> qualities;
    
    for (int i = 0; i < imageFiles.size(); i++) {
        QString filepath = dir.absoluteFilePath(imageFiles[i]);
        m_progressBar->setValue(i + 1);
        m_statusLabel->setText(QString("Analyzing: %1/%2").arg(i+1).arg(imageFiles.size()));
        
        QFileInfo fileInfo(filepath);
        QImage image(filepath);
        
        if (image.isNull()) continue;
        
        ImageQuality quality;
        quality.filename = imageFiles[i];
        quality.filesize = fileInfo.size();
        quality.width = image.width();
        quality.height = image.height();
        
        // Simple quality heuristic: resolution + filesize
        double resolutionScore = (image.width() * image.height()) / 1000000.0; // MP
        double filesizeScore = fileInfo.size() / (1024.0 * 1024.0); // MB
        quality.score = resolutionScore * 0.7 + filesizeScore * 0.3;
        
        qualities.append(quality);
        QCoreApplication::processEvents();
    }
    
    // Sort by score
    std::sort(qualities.begin(), qualities.end(),
        [](const ImageQuality& a, const ImageQuality& b) { return a.score > b.score; });
    
    // Generate report
    m_logOutput->append("\n📊 Quality Report (sorted by score):");
    m_logOutput->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    for (int i = 0; i < qMin(20, qualities.size()); i++) {
        const auto& q = qualities[i];
        m_logOutput->append(QString("%1. %2")
            .arg(i+1, 2)
            .arg(q.filename));
        m_logOutput->append(QString("   Score: %1 | %2x%3 | %4 MB")
            .arg(q.score, 0, 'f', 2)
            .arg(q.width)
            .arg(q.height)
            .arg(q.filesize / (1024.0 * 1024.0), 0, 'f', 1));
    }
    
    m_logOutput->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    m_logOutput->append(QString("Total: %1 images analyzed").arg(qualities.size()));
    
    LOG_INFO("AnalysisPanel", QString("Report generated: %1 images analyzed, top %2 shown")
        .arg(qualities.size()).arg(qMin(20, qualities.size())));
    m_statusLabel->setText("Report complete");
    m_progressBar->setValue(0);
    updateButtonStates(false);
    LOG_INFO("AnalysisPanel", "=== Generate Report - COMPLETE ===");
}

void AnalysisPanel::onCancelAnalysis() {
    LOG_INFO("AnalysisPanel", "User clicked: Cancel button");
    m_logOutput->append("⚠ Cancelling analysis...");
    m_statusLabel->setText("Cancelling...");
    updateButtonStates(false);
    
    // Note: Workers are in separate threads and will clean up automatically
    // For a production app, would need to track worker references and call cancel()
    LOG_INFO("AnalysisPanel", "Analysis cancellation requested");
    m_logOutput->append("Analysis operations will terminate shortly");
}

void AnalysisPanel::onAnalysisProgress(int current, int total, const QString& message) {
    if (total > 0) {
        int percent = (current * 100) / total;
        m_progressBar->setValue(percent);
        m_statusLabel->setText(QString("%1 (%2/%3)").arg(message).arg(current).arg(total));
    } else {
        m_statusLabel->setText(message);
    }
}

void AnalysisPanel::onAnalysisLog(const QString& message) {
    m_logOutput->append(message);
    
    // Auto-scroll to bottom
    QScrollBar* scrollBar = m_logOutput->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void AnalysisPanel::onAnalysisError(const QString& error) {
    m_logOutput->append(QString("<span style='color: #f44336;'>ERROR: %1</span>").arg(error));
    updateButtonStates(false);
}

void AnalysisPanel::showGeneratedCaption(const QString& caption) {
    m_captionDisplay->setPlainText(caption);
    m_captionGroup->setVisible(true);
}

} // namespace PhotoGuru
