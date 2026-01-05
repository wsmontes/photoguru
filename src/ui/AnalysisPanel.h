#ifndef ANALYSISPANEL_H
#define ANALYSISPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QString>
#include <QVBoxLayout>
#include <memory>

namespace PhotoGuru {

class CLIPAnalyzer;
class LlamaVLM;
class MetadataWriter;

class AnalysisPanel : public QWidget {
    Q_OBJECT

public:
    explicit AnalysisPanel(QWidget* parent = nullptr);
    ~AnalysisPanel();
    
    void setCurrentImage(const QString& filepath);
    void setCurrentDirectory(const QString& dirpath);

signals:
    void analysisStarted();
    void analysisCompleted();
    void metadataUpdated(const QString& filepath);
    void directoryAnalysisCompleted();

public slots:
    void onAnalyzeCurrentImage();
    void onAnalyzeDirectory();
    void onFindDuplicates();
    void onDetectBursts();
    void onGenerateReport();
    void onCancelAnalysis();

private slots:
    void onAnalysisProgress(int current, int total, const QString& message);
    void onAnalysisLog(const QString& message);
    void onAnalysisError(const QString& error);

private:
    void setupUI();
    void updateButtonStates(bool analyzing);
    void initializeAI();
    void showGeneratedCaption(const QString& caption);
    
    // Current context
    QString m_currentImage;
    QString m_currentDirectory;
    bool m_isAnalyzing;
    QString m_lastGeneratedCaption;  // Store last caption for reuse
    
    // AI Components
    std::unique_ptr<CLIPAnalyzer> m_clipAnalyzer;
    std::unique_ptr<LlamaVLM> m_llamaVLM;
    bool m_aiInitialized;
    
    // UI Components - Single Image Analysis
    QGroupBox* m_singleImageGroup;
    QPushButton* m_analyzeImageBtn;
    QLabel* m_currentImageLabel;
    
    // Generated Caption Display
    QGroupBox* m_captionGroup;
    QTextEdit* m_captionDisplay;
    QPushButton* m_copyCaptionBtn;
    QPushButton* m_applyToOthersBtn;
    
    // UI Components - Batch Analysis
    QGroupBox* m_batchGroup;
    QPushButton* m_analyzeDirBtn;
    QPushButton* m_findDuplicatesBtn;
    QPushButton* m_detectBurstsBtn;
    QPushButton* m_generateReportBtn;
    QCheckBox* m_overwriteCheckbox;
    QCheckBox* m_skipExistingCheckbox;
    
    // Progress
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_cancelBtn;
    
    // Log output
    QTextEdit* m_logOutput;
};

} // namespace PhotoGuru

#endif // ANALYSISPANEL_H
