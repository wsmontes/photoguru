#pragma once

#include <QString>
#include <QPixmap>
#include <QCache>
#include <QFuture>
#include <QObject>

namespace PhotoGuru {

class ThumbnailCache : public QObject {
    Q_OBJECT
    
public:
    static ThumbnailCache& instance();
    
    // Get thumbnail (from cache or generate)
    QPixmap getThumbnail(const QString& filepath, const QSize& size);
    
    // Pre-generate thumbnails in background
    void pregenerate(const QStringList& filepaths, const QSize& size);
    
    // Clear cache
    void clear();
    
signals:
    void thumbnailReady(const QString& filepath, const QPixmap& thumbnail);
    
private:
    ThumbnailCache() = default;
    ~ThumbnailCache() = default;
    ThumbnailCache(const ThumbnailCache&) = delete;
    ThumbnailCache& operator=(const ThumbnailCache&) = delete;
    
    QPixmap generateThumbnail(const QString& filepath, const QSize& size);
    QString cacheKey(const QString& filepath, const QSize& size) const;
    
    QCache<QString, QPixmap> m_cache;
};

} // namespace PhotoGuru
