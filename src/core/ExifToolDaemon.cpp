#include "ExifToolDaemon.h"
#include <QFileInfo>
#include <QDebug>
#include <QThread>

namespace PhotoGuru {

ExifToolDaemon& ExifToolDaemon::instance() {
    static ExifToolDaemon daemon;
    return daemon;
}

ExifToolDaemon::ExifToolDaemon() {
    // Auto-start on first use
}

ExifToolDaemon::~ExifToolDaemon() {
    stop();
}

QString ExifToolDaemon::findExifToolPath() const {
    QStringList locations = {
        "/opt/homebrew/bin/exiftool",
        "/usr/local/bin/exiftool",
        "/usr/bin/exiftool"
    };
    
    for (const QString& path : locations) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    
    return "exiftool"; // Try PATH
}

bool ExifToolDaemon::start() {
    QMutexLocker locker(&m_mutex);
    
    if (m_running) {
        return true;
    }
    
    QString exifToolPath = findExifToolPath();
    
    m_process = new QProcess();
    m_process->setProgram(exifToolPath);
    // NÃO merge channels - precisamos ler stdout separadamente para evitar deadlock
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    
    // Stay-open mode: process stays alive, reads commands from stdin
    QStringList args = {
        "-stay_open", "True",
        "-@", "-"              // Read args from stdin
    };
    
    m_process->setArguments(args);
    m_process->start();
    
    if (!m_process->waitForStarted(3000)) {
        qCritical() << "Failed to start ExifTool daemon at:" << exifToolPath;
        delete m_process;
        m_process = nullptr;
        return false;
    }
    
    m_running = true;
    qDebug() << "ExifTool daemon started (stay-open mode)";
    
    return true;
}

void ExifToolDaemon::stop() {
    QMutexLocker locker(&m_mutex);
    
    if (!m_running || !m_process) {
        return;
    }
    
    qDebug() << "Stopping ExifTool daemon...";
    
    // Send exit command
    m_process->write("-stay_open\nFalse\n");
    m_process->closeWriteChannel();
    
    // Wait for graceful exit
    if (!m_process->waitForFinished(2000)) {
        qWarning() << "ExifTool did not exit gracefully, terminating...";
        m_process->terminate();
        
        // Force kill if terminate doesn't work
        if (!m_process->waitForFinished(1000)) {
            qWarning() << "Force killing ExifTool process";
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }
    
    delete m_process;
    m_process = nullptr;
    m_running = false;
    
    qDebug() << "ExifTool daemon stopped";
}

bool ExifToolDaemon::isRunning() const {
    QMutexLocker locker(&m_mutex);
    return m_running;
}

QString ExifToolDaemon::executeCommand(const QStringList& args) {
    // Lock ONCE for entire operation
    QMutexLocker locker(&m_mutex);
    
    // Start if needed (already holding lock)
    if (!m_running) {
        locker.unlock();  // Temporarily unlock for start()
        if (!const_cast<ExifToolDaemon*>(this)->start()) {
            qWarning() << "[ExifToolDaemon] Failed to start";
            return QString();
        }
        locker.relock();  // Re-acquire lock
    }
    
    if (!m_process) {
        return QString();
    }
    
    // Write each argument on separate line
    for (const QString& arg : args) {
        m_process->write(arg.toUtf8());
        m_process->write("\n");
    }
    // Send execute marker
    m_process->write("-execute\n");
    m_process->waitForBytesWritten(3000);
    
    // Read response (mutex still held)
    return readResponse();
}

QStringList ExifToolDaemon::executeBatch(const QStringList& commands) {
    // Lock ONCE for entire batch operation
    QMutexLocker locker(&m_mutex);
    
    // Start if needed (already holding lock)
    if (!m_running) {
        locker.unlock();  // Temporarily unlock for start()
        if (!const_cast<ExifToolDaemon*>(this)->start()) {
            qWarning() << "Failed to start ExifTool daemon";
            return QStringList();
        }
        locker.relock();  // Re-acquire lock
    }
    
    if (!m_process) {
        return QStringList();
    }
    
    QStringList results;
    
    // Execute all commands
    for (const QString& cmd : commands) {
        m_process->write(cmd.toUtf8() + "\n");
        m_process->write(EXECUTE_MARKER);
        m_process->waitForBytesWritten(1000);
        
        QString response = readResponse();
        results.append(response);
    }
    
    return results;
}

QString ExifToolDaemon::readResponse() {
    // Read until we see {ready} marker
    QByteArray buffer;
    
    const int timeout = 10000; // 10 seconds  
    const int pollInterval = 50; // 50ms
    int elapsed = 0;
    
    while (elapsed < timeout) {
        // Espera dados ficarem disponíveis
        if (m_process->waitForReadyRead(pollInterval)) {
            // Lê TODOS os dados disponíveis imediatamente
            QByteArray data = m_process->readAllStandardOutput();
            buffer.append(data);
            
            // Check for ready marker
            if (buffer.contains(READY_MARKER)) {
                // Extract response (everything before {ready})
                int markerPos = buffer.indexOf(READY_MARKER);
                QString output = QString::fromUtf8(buffer.left(markerPos));
                return output.trimmed();
            }
        }
        
        elapsed += pollInterval;
    }
    
    // Timeout - loga erro e retorna o que temos
    qWarning() << "[ExifToolDaemon] Response timeout after" << timeout << "ms";
    qWarning() << "[ExifToolDaemon] Buffer contents:" << buffer;
    
    return QString::fromUtf8(buffer).trimmed();
}

} // namespace PhotoGuru
