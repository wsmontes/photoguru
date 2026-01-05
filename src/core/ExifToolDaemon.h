#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QMutex>
#include <QWaitCondition>

namespace PhotoGuru {

/**
 * @brief Daemon ExifTool usando stay-open mode para performance
 * 
 * Mantém processo ExifTool vivo entre chamadas, eliminando fork/exec overhead.
 * Performance: 5-10x mais rápido que spawn por chamada.
 * Thread-safe com mutex interno.
 */
class ExifToolDaemon : public QObject {
    Q_OBJECT
    
public:
    static ExifToolDaemon& instance();
    
    // Lifecycle
    bool start();
    void stop();
    bool isRunning() const;
    
    // Commands (thread-safe)
    QString executeCommand(const QStringList& args);
    
    // Batch operations (mais eficiente)
    QStringList executeBatch(const QStringList& commands);
    
private:
    ExifToolDaemon();
    ~ExifToolDaemon();
    ExifToolDaemon(const ExifToolDaemon&) = delete;
    ExifToolDaemon& operator=(const ExifToolDaemon&) = delete;
    
    QString findExifToolPath() const;
    QString readResponse();
    
    QProcess* m_process = nullptr;
    mutable QMutex m_mutex;  // mutable para permitir lock() em métodos const
    bool m_running = false;
    
    static constexpr const char* EXECUTE_MARKER = "-execute\n";
    static constexpr const char* READY_MARKER = "{ready}";
};

} // namespace PhotoGuru
