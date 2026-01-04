#pragma once

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QPoint>
#include <QFutureWatcher>
#include <optional>

namespace PhotoGuru {

class ImageViewer : public QWidget {
    Q_OBJECT
    
public:
    explicit ImageViewer(QWidget* parent = nullptr);
    
    void loadImage(const QString& filepath);
    void clear();
    
    // Zoom controls
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomActual();
    void setZoom(double factor);
    double zoom() const { return m_zoom; }
    
    // Navigation
    void nextImage();
    void previousImage();
    
    // Loading state
    bool isLoading() const { return m_isLoading; }
    
signals:
    void zoomChanged(double zoom);
    void imageLoaded(const QString& filepath);
    void nextImageRequested();
    void previousImageRequested();
    void escapePressed();
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    void updateTransform();
    void centerImage();
    void drawLoadingIndicator(QPainter& painter);
    void onImageLoadComplete();
    
    QImage m_image;
    QString m_filepath;
    
    // Transform
    double m_zoom = 1.0;
    QPoint m_offset;
    
    // Pan state
    bool m_isPanning = false;
    QPoint m_lastPanPos;
    
    // Fit mode
    bool m_autoFit = true;
    
    // Loading state
    bool m_isLoading = false;
    QFutureWatcher<std::optional<QImage>>* m_imageWatcher = nullptr;
    QString m_pendingFilepath;
};

} // namespace PhotoGuru
