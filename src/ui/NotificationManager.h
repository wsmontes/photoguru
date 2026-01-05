#pragma once

#include "NotificationToast.h"
#include <QObject>
#include <QQueue>
#include <QTimer>

namespace PhotoGuru {

class NotificationManager : public QObject {
    Q_OBJECT
    
public:
    static NotificationManager& instance();
    
    void setParentWidget(QWidget* parent);
    
    // Modern toast notifications (non-blocking)
    void showInfo(const QString& message, int durationMs = 3000);
    void showSuccess(const QString& message, int durationMs = 3000);
    void showWarning(const QString& message, int durationMs = 4000);
    void showError(const QString& message, int durationMs = 5000);
    
    // For questions, return false by default (safe option)
    bool askQuestion(const QString& title, const QString& message);
    
private:
    NotificationManager();
    ~NotificationManager() = default;
    
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;
    
    void showToast(const QString& message, ToastType type, int durationMs);
    void processQueue();
    
    QWidget* m_parentWidget;
    NotificationToast* m_currentToast;
    QQueue<std::tuple<QString, ToastType, int>> m_queue;
    QTimer* m_queueTimer;
    bool m_isShowing;
};

} // namespace PhotoGuru
