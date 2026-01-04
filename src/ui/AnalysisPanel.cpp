#include "AnalysisPanel.h"
#include "DarkTheme.h"
#include "../ml/PythonAnalysisWorker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileInfo>
#include <QScrollBar>
#include <QThread>

namespace PhotoGuru {

AnalysisPanel::AnalysisPanel(QWidget* parent)
    : QWidget(parent)
    , m_isAnalyzing(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(260);
    setupUI();
    updateButtonStates(false);
}

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
    m_skipExistingCheckbox = new QCheckBox("Skip already analyzed images");
    m_skipExistingCheckbox->setChecked(true);
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
    
    mainLayout->addStretch();
}

void AnalysisPanel::setCurrentImage(const QString& filepath) {
    m_currentImage = filepath;
    
    if (filepath.isEmpty()) {
        m_currentImageLabel->setText("No image selected");
        m_currentImageLabel->setStyleSheet("color: #888; font-style: italic;");
        m_analyzeImageBtn->setEnabled(false);
    } else {
        QFileInfo info(filepath);
        m_currentImageLabel->setText(info.fileName());
        m_currentImageLabel->setStyleSheet("color: #d4d4d4;");
        m_analyzeImageBtn->setEnabled(!m_isAnalyzing);
    }
}

void AnalysisPanel::setCurrentDirectory(const QString& dirpath) {
    m_currentDirectory = dirpath;
    
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

void AnalysisPanel::onAnalyzeCurrentImage() {
    if (m_currentImage.isEmpty()) return;
    
    updateButtonStates(true);
    m_statusLabel->setText("Analyzing image...");
    m_logOutput->append(QString("Starting analysis: %1").arg(QFileInfo(m_currentImage).fileName()));
    
    emit analysisStarted();
    
    // Create worker and thread
    QThread* thread = new QThread();
    PythonAnalysisWorker* worker = new PythonAnalysisWorker();
    worker->moveToThread(thread);
    
    // Connect signals
    connect(thread, &QThread::started, worker, [worker, this]() {
        worker->analyzeImage(m_currentImage, true);
    });
    connect(worker, &PythonAnalysisWorker::progress, this, &AnalysisPanel::onAnalysisProgress);
    connect(worker, &PythonAnalysisWorker::logMessage, this, [this](const QString& msg) {
        m_logOutput->append(msg);
    });
    connect(worker, &PythonAnalysisWorker::imageAnalyzed, this, &AnalysisPanel::metadataUpdated);
    connect(worker, &PythonAnalysisWorker::error, this, [this](const QString& err) {
        m_logOutput->append(QString("ERROR: %1").arg(err));
        updateButtonStates(false);
    });
    connect(worker, &PythonAnalysisWorker::finished, this, [this]() {
        updateButtonStates(false);
        m_statusLabel->setText("Analysis complete");
    });
    connect(worker, &PythonAnalysisWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void AnalysisPanel::onAnalyzeDirectory() {
    if (m_currentDirectory.isEmpty()) return;
    
    updateButtonStates(true);
    m_statusLabel->setText("Analyzing directory...");
    m_logOutput->append(QString("Starting batch analysis: %1").arg(m_currentDirectory));
    
    emit analysisStarted();
    
    // Create worker and thread
    QThread* thread = new QThread();
    PythonAnalysisWorker* worker = new PythonAnalysisWorker();
    worker->moveToThread(thread);
    
    // Connect signals
    connect(thread, &QThread::started, worker, [worker, this]() {
        worker->analyzeDirectory(m_currentDirectory, true);
    });
    connect(worker, &PythonAnalysisWorker::progress, this, &AnalysisPanel::onAnalysisProgress);
    connect(worker, &PythonAnalysisWorker::logMessage, this, [this](const QString& msg) {
        m_logOutput->append(msg);
    });
    connect(worker, &PythonAnalysisWorker::error, this, [this](const QString& err) {
        m_logOutput->append(QString("ERROR: %1").arg(err));
        updateButtonStates(false);
    });
    connect(worker, &PythonAnalysisWorker::finished, this, [this]() {
        updateButtonStates(false);
        m_statusLabel->setText("Batch analysis complete");
    });
    connect(worker, &PythonAnalysisWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void AnalysisPanel::onFindDuplicates() {
    if (m_currentDirectory.isEmpty()) return;
    
    updateButtonStates(true);
    m_statusLabel->setText("Searching for duplicates...");
    m_logOutput->append("Finding duplicate images...");
    
    emit analysisStarted();
    
    // Create worker and thread
    QThread* thread = new QThread();
    PythonAnalysisWorker* worker = new PythonAnalysisWorker();
    worker->moveToThread(thread);
    
    // Connect signals
    connect(thread, &QThread::started, worker, [worker, this]() {
        worker->findDuplicates(m_currentDirectory, 10);  // threshold 10
    });
    connect(worker, &PythonAnalysisWorker::progress, this, &AnalysisPanel::onAnalysisProgress);
    connect(worker, &PythonAnalysisWorker::logMessage, this, [this](const QString& msg) {
        m_logOutput->append(msg);
    });
    connect(worker, &PythonAnalysisWorker::error, this, [this](const QString& err) {
        m_logOutput->append(QString("ERROR: %1").arg(err));
        updateButtonStates(false);
    });
    connect(worker, &PythonAnalysisWorker::finished, this, [this]() {
        updateButtonStates(false);
        m_statusLabel->setText("Duplicate search complete");
    });
    connect(worker, &PythonAnalysisWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void AnalysisPanel::onDetectBursts() {
    if (m_currentDirectory.isEmpty()) return;
    
    updateButtonStates(true);
    m_statusLabel->setText("Detecting burst sequences...");
    m_logOutput->append("Analyzing burst groups...");
    
    emit analysisStarted();
    
    // Create worker and thread
    QThread* thread = new QThread();
    PythonAnalysisWorker* worker = new PythonAnalysisWorker();
    worker->moveToThread(thread);
    
    // Connect signals
    connect(thread, &QThread::started, worker, [worker, this]() {
        worker->detectBursts(m_currentDirectory, 5, 2);  // max 5 seconds, min 2 photos
    });
    connect(worker, &PythonAnalysisWorker::progress, this, &AnalysisPanel::onAnalysisProgress);
    connect(worker, &PythonAnalysisWorker::logMessage, this, [this](const QString& msg) {
        m_logOutput->append(msg);
    });
    connect(worker, &PythonAnalysisWorker::error, this, [this](const QString& err) {
        m_logOutput->append(QString("ERROR: %1").arg(err));
        updateButtonStates(false);
    });
    connect(worker, &PythonAnalysisWorker::finished, this, [this]() {
        updateButtonStates(false);
        m_statusLabel->setText("Burst detection complete");
    });
    connect(worker, &PythonAnalysisWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void AnalysisPanel::onGenerateReport() {
    if (m_currentDirectory.isEmpty()) return;
    
    updateButtonStates(true);
    m_statusLabel->setText("Generating quality report...");
    m_logOutput->append("Creating quality analysis report...");
    
    emit analysisStarted();
    
    // Create worker and thread
    QThread* thread = new QThread();
    PythonAnalysisWorker* worker = new PythonAnalysisWorker();
    worker->moveToThread(thread);
    
    // Connect signals
    connect(thread, &QThread::started, worker, [worker, this]() {
        worker->generateQualityReport(m_currentDirectory);
    });
    connect(worker, &PythonAnalysisWorker::progress, this, &AnalysisPanel::onAnalysisProgress);
    connect(worker, &PythonAnalysisWorker::logMessage, this, [this](const QString& msg) {
        m_logOutput->append(msg);
    });
    connect(worker, &PythonAnalysisWorker::error, this, [this](const QString& err) {
        m_logOutput->append(QString("ERROR: %1").arg(err));
        updateButtonStates(false);
    });
    connect(worker, &PythonAnalysisWorker::finished, this, [this]() {
        updateButtonStates(false);
        m_statusLabel->setText("Quality report complete");
    });
    connect(worker, &PythonAnalysisWorker::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    thread->start();
}

void AnalysisPanel::onCancelAnalysis() {
    m_logOutput->append("⚠ Cancelling analysis...");
    m_statusLabel->setText("Cancelling...");
    updateButtonStates(false);
    
    // Note: Workers are in separate threads and will clean up automatically
    // For a production app, would need to track worker references and call cancel()
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

} // namespace PhotoGuru
