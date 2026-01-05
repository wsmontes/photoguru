#include "NotificationToast.h"
#include <QPainter>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QApplication>
#include <QScreen>

namespace PhotoGuru {

NotificationToast::NotificationToast(QWidget* parent)
    : QWidget(parent)
    , m_currentType(ToastType::Info)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    
    setupUI();
    
    m_fadeAnimation = new QPropertyAnimation(this, "opacity", this);
    m_fadeAnimation->setDuration(300);
    
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &NotificationToast::fadeOut);
    
    setFixedHeight(60);
    setMinimumWidth(300);
}

void NotificationToast::setupUI() {
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);
    
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setStyleSheet("font-size: 20px;");
    layout->addWidget(m_iconLabel);
    
    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet("color: white; font-size: 14px;");
    layout->addWidget(m_messageLabel, 1);
    
    // Drop shadow effect for modern look
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setXOffset(0);
    shadow->setYOffset(4);
    shadow->setColor(QColor(0, 0, 0, 80));
    setGraphicsEffect(shadow);
}

void NotificationToast::show(const QString& message, ToastType type, int durationMs) {
    m_currentType = type;
    m_messageLabel->setText(message);
    updateStyle(type);
    
    // Position at bottom-right of parent or screen
    if (parentWidget()) {
        QPoint bottomRight = parentWidget()->rect().bottomRight();
        move(bottomRight.x() - width() - 20, bottomRight.y() - height() - 20);
    } else {
        QScreen* screen = QApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.width() - width() - 20, screenGeometry.height() - height() - 80);
    }
    
    // Fade in
    setWindowOpacity(0.0);
    QWidget::show();
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
    
    // Auto-hide after duration
    m_timer->start(durationMs);
}

void NotificationToast::fadeOut() {
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
    m_fadeAnimation->start();
}

void NotificationToast::updateStyle(ToastType type) {
    QString bgColor, icon;
    
    switch (type) {
        case ToastType::Success:
            bgColor = "#10b981"; // Green
            icon = "✓";
            break;
        case ToastType::Warning:
            bgColor = "#f59e0b"; // Orange
            icon = "⚠";
            break;
        case ToastType::Error:
            bgColor = "#ef4444"; // Red
            icon = "✕";
            break;
        case ToastType::Info:
        default:
            bgColor = "#3b82f6"; // Blue
            icon = "ℹ";
            break;
    }
    
    m_iconLabel->setText(icon);
    
    setStyleSheet(QString(R"(
        NotificationToast {
            background-color: %1;
            border-radius: 8px;
        }
    )").arg(bgColor));
}

void NotificationToast::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QString bgColor;
    switch (m_currentType) {
        case ToastType::Success: bgColor = "#10b981"; break;
        case ToastType::Warning: bgColor = "#f59e0b"; break;
        case ToastType::Error: bgColor = "#ef4444"; break;
        case ToastType::Info:
        default: bgColor = "#3b82f6"; break;
    }
    
    painter.setBrush(QColor(bgColor));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 8, 8);
}

} // namespace PhotoGuru
