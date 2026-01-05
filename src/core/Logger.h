#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

namespace PhotoGuru {

class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    static Logger& instance();
    
    void log(Level level, const QString& category, const QString& message);
    void debug(const QString& category, const QString& message);
    void info(const QString& category, const QString& message);
    void warning(const QString& category, const QString& message);
    void error(const QString& category, const QString& message);
    
    QString logFilePath() const { return m_logFile.fileName(); }
    void setLogLevel(Level level) { m_minLevel = level; }
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void rotateLogIfNeeded();
    QString levelToString(Level level) const;
    
    QFile m_logFile;
    QTextStream m_stream;
    QMutex m_mutex;
    Level m_minLevel;
    static constexpr qint64 MAX_LOG_SIZE = 10 * 1024 * 1024; // 10MB
};

// Convenience macros
#define LOG_DEBUG(category, msg) PhotoGuru::Logger::instance().debug(category, msg)
#define LOG_INFO(category, msg) PhotoGuru::Logger::instance().info(category, msg)
#define LOG_WARNING(category, msg) PhotoGuru::Logger::instance().warning(category, msg)
#define LOG_ERROR(category, msg) PhotoGuru::Logger::instance().error(category, msg)

} // namespace PhotoGuru

#endif // LOGGER_H
