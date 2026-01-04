#include "PythonAnalysisWorker.h"
#include "PythonBridge.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>

namespace PhotoGuru {

PythonAnalysisWorker::PythonAnalysisWorker(QObject* parent)
    : QObject(parent)
    , m_cancelled(false)
    , m_workerThread(nullptr)
{
}

PythonAnalysisWorker::~PythonAnalysisWorker() {
    cancel();
}

void PythonAnalysisWorker::cancel() {
    m_cancelled = true;
    emit logMessage("Cancellation requested...");
}

void PythonAnalysisWorker::analyzeImage(const QString& imagePath, bool overwrite) {
    m_cancelled = false;
    
    try {
        emit logMessage(QString("Analyzing: %1").arg(QFileInfo(imagePath).fileName()));
        emit progress(0, 1, "Starting analysis...");
        
        // Get path to agent_v2.py and venv python
        QFileInfo imageInfo(imagePath);
        QString agentPath = QCoreApplication::applicationDirPath() + "/../../../python/agent_v2.py";
        QString venvPython = QCoreApplication::applicationDirPath() + "/../../../.venv/bin/python3";
        
        // Check if venv exists
        if (!QFileInfo::exists(venvPython)) {
            venvPython = "python3";  // Fall back to system python
        }
        
        // Build command: python agent_v2.py write <image_path>
        QProcess process;
        QStringList args;
        args << agentPath << "info" << imagePath;
        
        emit logMessage(QString("Running: %1 %2 info \"%3\"").arg(venvPython).arg(agentPath).arg(imagePath));
        
        process.start(venvPython, args);
        if (!process.waitForFinished(60000)) {  // 60 second timeout
            emit error("Analysis timed out");
            emit finished();
            return;
        }
        
        int exitCode = process.exitCode();
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        
        if (exitCode == 0) {
            emit progress(100, 100, "Complete");
            emit logMessage(QString("✓ Analysis complete"));
            if (!output.isEmpty()) {
                emit logMessage(output);
            }
            emit imageAnalyzed(imagePath);
        } else {
            emit error(QString("Analysis failed (exit code %1): %2").arg(exitCode).arg(errorOutput));
        }
        
        emit finished();
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Error: %1").arg(e.what());
        qWarning() << errorMsg;
        emit error(errorMsg);
        emit finished();
    }
}

void PythonAnalysisWorker::analyzeDirectory(const QString& dirPath, bool skipExisting) {
    m_cancelled = false;
    
    try {
        emit logMessage(QString("Starting batch analysis of directory: %1").arg(dirPath));
        emit progress(0, 100, "Initializing...");
        
        // Get path to agent_v2.py and venv python
        QString agentPath = QCoreApplication::applicationDirPath() + "/../../../python/agent_v2.py";
        QString venvPython = QCoreApplication::applicationDirPath() + "/../../../.venv/bin/python3";
        
        if (!QFileInfo::exists(venvPython)) {
            venvPython = "python3";
        }
        
        // Build command: python agent_v2.py write <directory>
        QProcess process;
        QStringList args;
        args << agentPath << "write" << dirPath;
        
        if (skipExisting) {
            args << "--skip-existing";
        }
        
        emit logMessage(QString("Running: %1 %2 write \"%3\"").arg(venvPython).arg(agentPath).arg(dirPath));
        emit progress(10, 100, "Running batch analysis...");
        
        process.start(venvPython, args);
        
        // Stream output as it comes
        connect(&process, &QProcess::readyReadStandardOutput, [&]() {
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            emit logMessage(output);
        });
        
        if (!process.waitForFinished(300000)) {  // 5 minute timeout
            process.kill();
            emit error("Batch analysis timed out");
            emit finished();
            return;
        }
        
        int exitCode = process.exitCode();
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        
        if (exitCode == 0) {
            emit progress(100, 100, "Complete");
            emit logMessage("\n✓ Batch analysis complete");
        } else {
            emit error(QString("Batch analysis failed (exit code %1): %2").arg(exitCode).arg(errorOutput));
        }
        
        emit finished();
        
    } catch (const std::exception& e) {
        emit error(QString("Error: %1").arg(e.what()));
        emit finished();
    }
}

void PythonAnalysisWorker::findDuplicates(const QString& dirPath, int threshold) {
    m_cancelled = false;
    
    try {
        emit logMessage(QString("Searching for duplicates in: %1").arg(dirPath));
        emit progress(0, 100, "Analyzing...");
        
        QString agentPath = QCoreApplication::applicationDirPath() + "/../../../python/agent_v2.py";
        QString venvPython = QCoreApplication::applicationDirPath() + "/../../../.venv/bin/python3";
        
        if (!QFileInfo::exists(venvPython)) {
            venvPython = "python3";
        }
        
        QProcess process;
        QStringList args;
        args << agentPath << "duplicates" << dirPath << "--threshold" << QString::number(threshold);
        
        emit logMessage(QString("Running: %1 %2 duplicates \"%3\" --threshold %4").arg(venvPython).arg(agentPath).arg(dirPath).arg(threshold));
        
        process.start(venvPython, args);
        if (!process.waitForFinished(120000)) {
            process.kill();
            emit error("Duplicate detection timed out");
            emit finished();
            return;
        }
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        
        if (process.exitCode() == 0) {
            emit progress(100, 100, "Complete");
            emit logMessage(output);
            emit logMessage("✓ Duplicate detection complete");
        } else {
            emit error(QString("Duplicate detection failed: %1").arg(errorOutput));
        }
        
        emit finished();
        
    } catch (const std::exception& e) {
        emit error(QString("Error: %1").arg(e.what()));
        emit finished();
    }
}

void PythonAnalysisWorker::detectBursts(const QString& dirPath, int maxSeconds, int minPhotos) {
    m_cancelled = false;
    
    try {
        emit logMessage(QString("Detecting burst sequences in: %1").arg(dirPath));
        emit progress(0, 100, "Analyzing...");
        
        QString agentPath = QCoreApplication::applicationDirPath() + "/../../../python/agent_v2.py";
        QString venvPython = QCoreApplication::applicationDirPath() + "/../../../.venv/bin/python3";
        
        if (!QFileInfo::exists(venvPython)) {
            venvPython = "python3";
        }
        
        QProcess process;
        QStringList args;
        args << agentPath << "bursts" << dirPath 
             << "--max-seconds" << QString::number(maxSeconds)
             << "--min-photos" << QString::number(minPhotos);
        
        emit logMessage(QString("Running: %1 %2 bursts \"%3\"").arg(venvPython).arg(agentPath).arg(dirPath));
        
        process.start(venvPython, args);
        if (!process.waitForFinished(120000)) {
            process.kill();
            emit error("Burst detection timed out");
            emit finished();
            return;
        }
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        
        if (process.exitCode() == 0) {
            emit progress(100, 100, "Complete");
            emit logMessage(output);
            emit logMessage("✓ Burst detection complete");
        } else {
            emit error(QString("Burst detection failed: %1").arg(errorOutput));
        }
        
        emit finished();
        
    } catch (const std::exception& e) {
        emit error(QString("Error: %1").arg(e.what()));
        emit finished();
    }
}

void PythonAnalysisWorker::generateQualityReport(const QString& dirPath, const QString& sortBy) {
    m_cancelled = false;
    
    try {
        emit logMessage(QString("Generating quality report for: %1").arg(dirPath));
        emit progress(0, 100, "Analyzing...");
        
        QString agentPath = QCoreApplication::applicationDirPath() + "/../../../python/agent_v2.py";
        QString venvPython = QCoreApplication::applicationDirPath() + "/../../../.venv/bin/python3";
        
        if (!QFileInfo::exists(venvPython)) {
            venvPython = "python3";
        }
        
        QProcess process;
        QStringList args;
        args << agentPath << "quality" << dirPath << "--sort-by" << "overall";
        
        emit logMessage(QString("Running: %1 %2 quality \"%3\"").arg(venvPython).arg(agentPath).arg(dirPath));
        
        process.start(venvPython, args);
        if (!process.waitForFinished(120000)) {
            process.kill();
            emit error("Quality report generation timed out");
            emit finished();
            return;
        }
        
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        
        if (process.exitCode() == 0) {
            emit progress(100, 100, "Complete");
            emit logMessage(output);
            emit logMessage("✓ Quality report generated");
        } else {
            emit error(QString("Quality report failed: %1").arg(errorOutput));
        }
        
        emit finished();
        
    } catch (const std::exception& e) {
        emit error(QString("Error: %1").arg(e.what()));
        emit finished();
    }
}

} // namespace PhotoGuru
