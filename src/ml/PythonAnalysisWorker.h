#ifndef PYTHONANALYSISWORKER_H
#define PYTHONANALYSISWORKER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <atomic>

namespace PhotoGuru {

class PythonAnalysisWorker : public QObject {
    Q_OBJECT

public:
    explicit PythonAnalysisWorker(QObject* parent = nullptr);
    ~PythonAnalysisWorker();
    
    void analyzeImage(const QString& imagePath, bool overwrite = false);
    void analyzeDirectory(const QString& dirPath, bool skipExisting = true);
    void findDuplicates(const QString& dirPath, int threshold = 10);
    void detectBursts(const QString& dirPath, int maxSeconds = 5, int minPhotos = 2);
    void generateQualityReport(const QString& dirPath, const QString& sortBy = "overall");
    
    void cancel();

signals:
    void progress(int current, int total, const QString& message);
    void logMessage(const QString& message);
    void error(const QString& errorMessage);
    void finished();
    void imageAnalyzed(const QString& imagePath);

private:
    void initializePython();
    bool callPythonAnalysis(const QString& function, const QStringList& args);
    
    std::atomic<bool> m_cancelled;
    QThread* m_workerThread;
};

} // namespace PhotoGuru

#endif // PYTHONANALYSISWORKER_H
