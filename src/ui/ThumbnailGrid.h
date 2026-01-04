#pragma once

#include <QListWidget>
#include <QStringList>
#include <QCache>
#include <QPixmap>
#include <QFuture>

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
    ~ThumbnailGrid() { m_thumbnailCache.clear(); }
    
    void setImages(const QStringList& imagePaths);
    void selectImage(int index);
    
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
    QPixmap generateThumbnail(const QString& filepath);
    void sortImages();
    
    QStringList m_imagePaths;
    QCache<QString, QPixmap> m_thumbnailCache;
    int m_thumbnailSize = 150;
    SortOrder m_sortOrder = SortOrder::ByName;
};

} // namespace PhotoGuru
