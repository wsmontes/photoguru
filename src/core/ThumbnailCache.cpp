#include "ThumbnailCache.h"
#include "ImageLoader.h"
#include <QPainter>
#include <QtConcurrent>

namespace PhotoGuru {

ThumbnailCache& ThumbnailCache::instance() {
    static ThumbnailCache cache;
    cache.m_cache.setMaxCost(500);  // Cache 500 thumbnails
    return cache;
}

QPixmap ThumbnailCache::getThumbnail(const QString& filepath, const QSize& size) {
    QString key = cacheKey(filepath, size);
    
    // Check cache
    if (QPixmap* cached = m_cache.object(key)) {
        return *cached;
    }
    
    // Generate thumbnail
    QPixmap thumbnail = generateThumbnail(filepath, size);
    
    // Store in cache
    m_cache.insert(key, new QPixmap(thumbnail));
    
    return thumbnail;
}

void ThumbnailCache::pregenerate(const QStringList& filepaths, const QSize& size) {
    for (const QString& filepath : filepaths) {
        QString key = cacheKey(filepath, size);
        
        // Skip if already cached
        if (m_cache.contains(key)) continue;
        
        // Generate in background
        QtConcurrent::run([this, filepath, size, key]() {
            QPixmap thumbnail = generateThumbnail(filepath, size);
            
            // Update cache in main thread
            QMetaObject::invokeMethod(this, [this, key, thumbnail]() {
                m_cache.insert(key, new QPixmap(thumbnail));
                emit thumbnailReady(QString(), thumbnail);
            }, Qt::QueuedConnection);
        });
    }
}

void ThumbnailCache::clear() {
    m_cache.clear();
}

QPixmap ThumbnailCache::generateThumbnail(const QString& filepath, const QSize& size) {
    // Load image at reduced resolution
    auto imageOpt = ImageLoader::instance().load(filepath, 
        QSize(size.width() * 2, size.height() * 2));
    
    if (!imageOpt) {
        // Error placeholder
        QPixmap placeholder(size);
        placeholder.fill(QColor(80, 40, 40));
        return placeholder;
    }
    
    QImage image = *imageOpt;
    
    // Scale to thumbnail size
    QImage scaled = image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Center in square
    QPixmap result(size);
    result.fill(QColor(42, 42, 42));
    
    QPainter painter(&result);
    int x = (size.width() - scaled.width()) / 2;
    int y = (size.height() - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
    
    return result;
}

QString ThumbnailCache::cacheKey(const QString& filepath, const QSize& size) const {
    return QString("%1_%2x%3").arg(filepath).arg(size.width()).arg(size.height());
}

} // namespace PhotoGuru
