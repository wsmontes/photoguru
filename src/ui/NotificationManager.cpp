#include "NotificationManager.h"
#include <QDebug>

namespace PhotoGuru {

NotificationManager& NotificationManager::instance() {
    static NotificationManager instance;
    return instance;
}

NotificationManager::NotificationManager()
    : QObject(nullptr)
    , m_parentWidget(nullptr)
    , m_currentToast(nullptr)
    , m_isShowing(false)
{
    m_queueTimer = new QTimer(this);
    m_queueTimer->setInterval(100);
    connect(m_queueTimer, &QTimer::timeout, this, &NotificationManager::processQueue);
}

void NotificationManager::setParentWidget(QWidget* parent) {
    m_parentWidget = parent;
    if (m_currentToast) {
        delete m_currentToast;
        m_currentToast = nullptr;
    }
}

void NotificationManager::showInfo(const QString& message, int durationMs) {
    showToast(message, ToastType::Info, durationMs);
}

void NotificationManager::showSuccess(const QString& message, int durationMs) {
    showToast(message, ToastType::Success, durationMs);
}

void NotificationManager::showWarning(const QString& message, int durationMs) {
    showToast(message, ToastType::Warning, durationMs);
}

void NotificationManager::showError(const QString& message, int durationMs) {
    showToast(message, ToastType::Error, durationMs);
}

bool NotificationManager::askQuestion(const QString& title, const QString& message) {
    // For questions that need user confirmation, show warning and return false (safe option)
    // In production, could show a toast with Yes/No buttons
    showWarning(QString("%1: %2 (Operation cancelled)").arg(title).arg(message), 4000);
    return false;
}

void NotificationManager::showToast(const QString& message, ToastType type, int durationMs) {
    // Skip notifications in test mode
    if (qEnvironmentVariableIsSet("PHOTOGURU_TESTING")) {
        qDebug() << "[Toast]" << message;
        return;
    }
    
    if (!m_parentWidget) {
        qWarning() << "NotificationManager: No parent widget set";
        return;
    }
    
    // Queue the notification
    m_queue.enqueue(std::make_tuple(message, type, durationMs));
    
    // Start processing queue if not already showing
    if (!m_isShowing) {
        processQueue();
    }
}

void NotificationManager::processQueue() {
    if (m_queue.isEmpty()) {
        m_queueTimer->stop();
        m_isShowing = false;
        return;
    }
    
    if (m_isShowing) {
        return; // Wait for current toast to finish
    }
    
    auto [message, type, duration] = m_queue.dequeue();
    
    if (!m_currentToast) {
        m_currentToast = new NotificationToast(m_parentWidget);
    }
    
    m_isShowing = true;
    m_currentToast->show(message, type, duration);
    
    // Schedule next notification
    QTimer::singleShot(duration + 500, this, [this]() {
        m_isShowing = false;
        if (!m_queue.isEmpty()) {
            processQueue();
        }
    });
}

} // namespace PhotoGuru
