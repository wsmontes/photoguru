#include "Logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QMutexLocker>
#include <iostream>

namespace PhotoGuru {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_minLevel(DEBUG) {
    // Create logs directory
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    
    // Open log file
    QString logPath = logDir + "/photoguru.log";
    m_logFile.setFileName(logPath);
    
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        log(INFO, "Logger", "=== PhotoGuru Started ===");
        log(INFO, "Logger", "Log file: " + logPath);
    } else {
        std::cerr << "Failed to open log file: " << logPath.toStdString() << std::endl;
    }
}

Logger::~Logger() {
    if (m_logFile.isOpen()) {
        log(INFO, "Logger", "=== PhotoGuru Shutdown ===");
        m_stream.flush();
        m_logFile.close();
    }
}

void Logger::log(Level level, const QString& category, const QString& message) {
    if (level < m_minLevel) return;
    
    QMutexLocker locker(&m_mutex);
    
    rotateLogIfNeeded();
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString logLine = QString("[%1] [%2] [%3] %4\n")
        .arg(timestamp)
        .arg(levelStr)
        .arg(category)
        .arg(message);
    
    m_stream << logLine;
    m_stream.flush();
    
    // Also print to console for development
    std::cout << logLine.toStdString();
}

void Logger::debug(const QString& category, const QString& message) {
    log(DEBUG, category, message);
}

void Logger::info(const QString& category, const QString& message) {
    log(INFO, category, message);
}

void Logger::warning(const QString& category, const QString& message) {
    log(WARNING, category, message);
}

void Logger::error(const QString& category, const QString& message) {
    log(ERROR, category, message);
}

void Logger::rotateLogIfNeeded() {
    if (m_logFile.size() > MAX_LOG_SIZE) {
        m_stream.flush();
        m_logFile.close();
        
        // Rename old log
        QString oldPath = m_logFile.fileName();
        QString backupPath = oldPath + ".old";
        QFile::remove(backupPath);
        QFile::rename(oldPath, backupPath);
        
        // Open new log
        m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        m_stream.setDevice(&m_logFile);
        log(INFO, "Logger", "Log rotated (previous log saved as .old)");
    }
}

QString Logger::levelToString(Level level) const {
    switch (level) {
        case DEBUG:   return "DEBUG  ";
        case INFO:    return "INFO   ";
        case WARNING: return "WARNING";
        case ERROR:   return "ERROR  ";
        default:      return "UNKNOWN";
    }
}

} // namespace PhotoGuru
