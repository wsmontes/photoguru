#pragma once

#include <QListWidget>
#include <QStringList>
#include <QCache>
#include <QPixmap>
#include <QFuture>
#include <QAtomicInt>
#include <QThreadPool>
#include <QDir>

namespace PhotoGuru {

enum class SortOrder {
    ByName,
    ByDate,
    BySize
};

class ThumbnailGrid : public QListWidget {
    Q_OBJECT
    
public:
    explicit ThumbnailGrid(QWidget* parent = nullptr);
    ~ThumbnailGrid();
    
    void setImages(const QStringList& imagePaths);
    void selectImage(int index);
    void setCurrentIndex(int index);
    
    // Size control
    void setThumbnailSize(int size);
    int thumbnailSize() const { return m_thumbnailSize; }
    
    // Sorting
    void setSortOrder(SortOrder order);
    SortOrder sortOrder() const { return m_sortOrder; }
    
    // Selection
    QStringList selectedFiles() const;
    
signals:
    void imageSelected(const QString& filepath);
    void selectionCountChanged(int count);
    
private slots:
    void onItemClicked(QListWidgetItem* item);
    void onSelectionChanged();
    
private:
    void loadThumbnails();
    void loadThumbnailAsync(int index, QThread::Priority priority = QThread::NormalPriority);
    QPixmap generateThumbnail(const QString& filepath);
    void sortImages();
    
    QStringList m_imagePaths;
    QCache<QString, QPixmap> m_thumbnailCache;
    int m_thumbnailSize = 150;
    int m_currentIndex = -1;
    SortOrder m_sortOrder = SortOrder::ByName;
    QAtomicInt m_loadingTasks{0};
    QThreadPool* m_threadPool;
    QDir m_diskCacheDir;
    
    // Performance optimization
    QPixmap loadFromDiskCache(const QString& filepath);
    void saveToDiskCache(const QString& filepath, const QPixmap& pixmap);
    QString getCacheKey(const QString& filepath, int size);
};

} // namespace PhotoGuru
