#include "ThumbnailGrid.h"
#include "ImageLoader.h"
#include <QListWidgetItem>
#include <QFileInfo>
#include <QtConcurrent>
#include <QApplication>
#include <QPainter>
#include <QDateTime>
#include <QThreadPool>
#include <QPointer>

namespace PhotoGuru {

ThumbnailGrid::ThumbnailGrid(QWidget* parent)
    : QListWidget(parent)
{
    setViewMode(QListView::IconMode);
    setIconSize(QSize(m_thumbnailSize, m_thumbnailSize));
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setSpacing(10);
    setUniformItemSizes(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumHeight(80);
    
    // Enable multi-selection
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    m_thumbnailCache.setMaxCost(500);  // Cache up to 500 thumbnails
    
    connect(this, &QListWidget::itemClicked, this, &ThumbnailGrid::onItemClicked);
    connect(this, &QListWidget::itemSelectionChanged, this, &ThumbnailGrid::onSelectionChanged);
}

ThumbnailGrid::~ThumbnailGrid() {
    // Wait for all thumbnail loading tasks to complete
    while (m_loadingTasks.loadAcquire() > 0) {
        QApplication::processEvents();
        QThread::msleep(10);
    }
    m_thumbnailCache.clear();
}

void ThumbnailGrid::setImages(const QStringList& imagePaths) {
    clear();
    m_imagePaths = imagePaths;
    sortImages();
    loadThumbnails();
}

void ThumbnailGrid::setThumbnailSize(int size) {
    m_thumbnailSize = size;
    setIconSize(QSize(size, size));
    
    // Clear cache and reload
    m_thumbnailCache.clear();
    loadThumbnails();
}

void ThumbnailGrid::setSortOrder(SortOrder order) {
    if (m_sortOrder == order) return;
    
    m_sortOrder = order;
    sortImages();
    
    // Clear and reload
    clear();
    loadThumbnails();
}

void ThumbnailGrid::sortImages() {
    switch (m_sortOrder) {
        case SortOrder::ByName:
            std::sort(m_imagePaths.begin(), m_imagePaths.end(), 
                [](const QString& a, const QString& b) {
                    return QFileInfo(a).fileName() < QFileInfo(b).fileName();
                });
            break;
        case SortOrder::ByDate:
            std::sort(m_imagePaths.begin(), m_imagePaths.end(),
                [](const QString& a, const QString& b) {
                    return QFileInfo(a).lastModified() > QFileInfo(b).lastModified();
                });
            break;
        case SortOrder::BySize:
            std::sort(m_imagePaths.begin(), m_imagePaths.end(),
                [](const QString& a, const QString& b) {
                    return QFileInfo(a).size() > QFileInfo(b).size();
                });
            break;
    }
}

QStringList ThumbnailGrid::selectedFiles() const {
    QStringList files;
    for (QListWidgetItem* item : selectedItems()) {
        files << item->data(Qt::UserRole).toString();
    }
    return files;
}

void ThumbnailGrid::selectImage(int index) {
    if (index >= 0 && index < count()) {
        setCurrentRow(index);
    }
}

void ThumbnailGrid::setCurrentIndex(int index) {
    // Clear previous current item special styling
    if (m_currentIndex >= 0 && m_currentIndex < count()) {
        QListWidgetItem* prevItem = item(m_currentIndex);
        if (prevItem) {
            prevItem->setData(Qt::UserRole + 2, false);  // Mark as not current
        }
    }
    
    m_currentIndex = index;
    
    // Set new current item with special styling
    if (m_currentIndex >= 0 && m_currentIndex < count()) {
        QListWidgetItem* currentItem = item(m_currentIndex);
        if (currentItem) {
            currentItem->setData(Qt::UserRole + 2, true);  // Mark as current
            
            // Add subtle background highlight for current item
            QColor highlightColor(31, 145, 255, 30);  // Adobe blue with low opacity
            currentItem->setBackground(QBrush(highlightColor));
            
            // Ensure visible
            scrollToItem(currentItem, QAbstractItemView::EnsureVisible);
        }
    }
}

void ThumbnailGrid::loadThumbnails() {
    // Add placeholder items first
    for (const QString& path : m_imagePaths) {
        QListWidgetItem* item = new QListWidgetItem(this);
        item->setData(Qt::UserRole, path);
        item->setText(QFileInfo(path).fileName());
        
        // Check cache first
        if (QPixmap* cached = m_thumbnailCache.object(path)) {
            item->setIcon(QIcon(*cached));
        } else {
            // Placeholder icon
            QPixmap placeholder(m_thumbnailSize, m_thumbnailSize);
            placeholder.fill(QColor(60, 60, 60));
            item->setIcon(QIcon(placeholder));
        }
    }
    
    // Load thumbnails asynchronously
    for (int i = 0; i < m_imagePaths.count(); ++i) {
        QString path = m_imagePaths[i];
        
        // Skip if already cached
        if (m_thumbnailCache.contains(path)) continue;
        
        m_loadingTasks.fetchAndAddOrdered(1);
        
        QPointer<ThumbnailGrid> self(this);
        QtConcurrent::run([self, path, i]() {
            if (!self) return;
            
            QPixmap thumb = self->generateThumbnail(path);
            
            // Update UI in main thread - check if widget still exists
            QMetaObject::invokeMethod(qApp, [self, path, i, thumb]() {
                if (!self) return;
                
                self->m_thumbnailCache.insert(path, new QPixmap(thumb));
                
                if (i < self->count()) {
                    QListWidgetItem* item = self->item(i);
                    if (item && item->data(Qt::UserRole).toString() == path) {
                        item->setIcon(QIcon(thumb));
                    }
                }
                
                self->m_loadingTasks.fetchAndAddOrdered(-1);
            }, Qt::QueuedConnection);
        });
    }
}

QPixmap ThumbnailGrid::generateThumbnail(const QString& filepath) {
    auto imageOpt = ImageLoader::instance().load(filepath, 
        QSize(m_thumbnailSize * 2, m_thumbnailSize * 2));
    
    if (!imageOpt) {
        // Error placeholder
        QPixmap placeholder(m_thumbnailSize, m_thumbnailSize);
        placeholder.fill(QColor(80, 40, 40));
        return placeholder;
    }
    
    QImage image = *imageOpt;
    
    // Scale to thumbnail size maintaining aspect ratio
    QImage scaled = image.scaled(m_thumbnailSize, m_thumbnailSize,
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Center in square
    QPixmap result(m_thumbnailSize, m_thumbnailSize);
    result.fill(QColor(42, 42, 42));
    
    QPainter painter(&result);
    int x = (m_thumbnailSize - scaled.width()) / 2;
    int y = (m_thumbnailSize - scaled.height()) / 2;
    painter.drawImage(x, y, scaled);
    
    return result;
}

void ThumbnailGrid::onItemClicked(QListWidgetItem* item) {
    QString filepath = item->data(Qt::UserRole).toString();
    emit imageSelected(filepath);
}

void ThumbnailGrid::onSelectionChanged() {
    // Emit signal with selection count
    int count = selectedItems().count();
    emit selectionCountChanged(count);
}

} // namespace PhotoGuru
