#pragma once

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

namespace PhotoGuru {

enum class ToastType {
    Info,
    Success,
    Warning,
    Error
};

class NotificationToast : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ windowOpacity WRITE setWindowOpacity)
    
public:
    explicit NotificationToast(QWidget* parent = nullptr);
    
    void show(const QString& message, ToastType type = ToastType::Info, int durationMs = 3000);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private slots:
    void fadeOut();
    
private:
    void setupUI();
    void updateStyle(ToastType type);
    
    QLabel* m_iconLabel;
    QLabel* m_messageLabel;
    QPropertyAnimation* m_fadeAnimation;
    QTimer* m_timer;
    ToastType m_currentType;
};

} // namespace PhotoGuru
