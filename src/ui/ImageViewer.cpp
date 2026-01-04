#include "ImageViewer.h"
#include "../core/ImageLoader.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <cmath>

namespace PhotoGuru {

ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void ImageViewer::loadImage(const QString& filepath) {
    m_isLoading = true;
    update();
    
    auto imageOpt = ImageLoader::instance().load(filepath, QSize(4000, 4000));
    
    m_isLoading = false;
    
    if (!imageOpt) {
        m_image = QImage();
        m_filepath.clear();
        update();
        return;
    }
    
    m_image = *imageOpt;
    m_filepath = filepath;
    
    // Always fit new images to window
    m_autoFit = true;
    zoomToFit();
    
    update();
    emit imageLoaded(filepath);
}

void ImageViewer::clear() {
    m_image = QImage();
    m_filepath.clear();
    update();
}

void ImageViewer::zoomIn() {
    m_autoFit = false;
    setZoom(m_zoom * 1.25);
}

void ImageViewer::zoomOut() {
    m_autoFit = false;
    setZoom(m_zoom / 1.25);
}

void ImageViewer::zoomToFit() {
    if (m_image.isNull()) return;
    
    m_autoFit = true;
    
    double widthRatio = double(width()) / m_image.width();
    double heightRatio = double(height()) / m_image.height();
    m_zoom = std::min(widthRatio, heightRatio);
    
    centerImage();
    emit zoomChanged(m_zoom);
    update();
}

void ImageViewer::zoomActual() {
    m_autoFit = false;
    setZoom(1.0);
}

void ImageViewer::setZoom(double factor) {
    m_zoom = std::clamp(factor, 0.01, 20.0);
    updateTransform();
    emit zoomChanged(m_zoom);
    update();
}

void ImageViewer::updateTransform() {
    if (m_image.isNull()) return;
    centerImage();
}

void ImageViewer::centerImage() {
    if (m_image.isNull()) return;
    
    int scaledWidth = int(m_image.width() * m_zoom);
    int scaledHeight = int(m_image.height() * m_zoom);
    
    m_offset.setX((width() - scaledWidth) / 2);
    m_offset.setY((height() - scaledHeight) / 2);
}

void ImageViewer::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    if (!m_image.isNull() && m_autoFit) {
        zoomToFit();
    }
}

void ImageViewer::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(42, 42, 42));
    
    if (m_isLoading) {
        drawLoadingIndicator(painter);
        return;
    }
    
    if (m_image.isNull()) {
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(rect(), Qt::AlignCenter, "No image loaded\nPress Ctrl+O to open images");
        return;
    }
    
    // Draw image
    int scaledWidth = int(m_image.width() * m_zoom);
    int scaledHeight = int(m_image.height() * m_zoom);
    
    QRect targetRect(m_offset.x(), m_offset.y(), scaledWidth, scaledHeight);
    
    // Use smooth transform for zoom < 1.0
    if (m_zoom < 1.0) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
    }
    
    painter.drawImage(targetRect, m_image);
}

void ImageViewer::wheelEvent(QWheelEvent* event) {
    if (m_image.isNull()) return;
    
    m_autoFit = false;
    
    // Zoom factor
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    
    // Zoom towards mouse position
    QPoint mousePos = event->position().toPoint();
    QPoint imagePos = mousePos - m_offset;
    
    double oldZoom = m_zoom;
    m_zoom = std::clamp(m_zoom * factor, 0.01, 20.0);
    
    // Adjust offset to keep point under mouse stationary
    m_offset = mousePos - imagePos * (m_zoom / oldZoom);
    
    emit zoomChanged(m_zoom);
    update();
}

void ImageViewer::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_offset += delta;
        m_lastPanPos = event->pos();
        m_autoFit = false;
        update();
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void ImageViewer::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_Up:
            emit previousImageRequested();
            break;
        case Qt::Key_Right:
        case Qt::Key_Down:
        case Qt::Key_Space:
            emit nextImageRequested();
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            zoomIn();
            break;
        case Qt::Key_Minus:
        case Qt::Key_Underscore:
            zoomOut();
            break;
        case Qt::Key_0:
            if (event->modifiers() & Qt::ControlModifier) {
                zoomActual();
            }
            break;
        case Qt::Key_F:
            zoomToFit();
            break;
        case Qt::Key_Escape:
            emit escapePressed();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

void ImageViewer::nextImage() {
    emit nextImageRequested();
}

void ImageViewer::previousImage() {
    emit previousImageRequested();
}

void ImageViewer::drawLoadingIndicator(QPainter& painter) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(100, 149, 237), 4));
    
    int size = 60;
    int x = (width() - size) / 2;
    int y = (height() - size) / 2;
    
    // Draw animated spinner arc
    static int rotation = 0;
    rotation = (rotation + 10) % 360;
    painter.drawArc(x, y, size, size, rotation * 16, 120 * 16);
    
    painter.setPen(QColor(150, 150, 150));
    painter.drawText(rect().adjusted(0, size + 20, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "Loading...");
    
    // Trigger repaint for animation
    QTimer::singleShot(50, this, [this]() { if (m_isLoading) update(); });
}

} // namespace PhotoGuru
