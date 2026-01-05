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
    
    // PERFORMANCE: Larger memory cache
    m_thumbnailCache.setMaxCost(1000);  // Cache up to 1000 thumbnails
    
    // PERFORMANCE: Dedicated thread pool for thumbnails
    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(4);  // 4 parallel thumbnail loads
    
    // PERFORMANCE: Setup disk cache directory
    QString cacheDir = QDir::homePath() + "/.photoguru/thumbnails";
    m_diskCacheDir = QDir(cacheDir);
    if (!m_diskCacheDir.exists()) {
        m_diskCacheDir.mkpath(".");
    }
    
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
        
        QString cacheKey = getCacheKey(path, m_thumbnailSize);
        
        // Check memory cache first
        if (QPixmap* cached = m_thumbnailCache.object(cacheKey)) {
            item->setIcon(QIcon(*cached));
            continue;
        }
        
        // Check disk cache
        QPixmap diskCached = loadFromDiskCache(cacheKey);
        if (!diskCached.isNull()) {
            item->setIcon(QIcon(diskCached));
            m_thumbnailCache.insert(cacheKey, new QPixmap(diskCached));
            continue;
        }
        
        // Placeholder icon for loading
        QPixmap placeholder(m_thumbnailSize, m_thumbnailSize);
        placeholder.fill(QColor(60, 60, 60));
        item->setIcon(QIcon(placeholder));
    }
    
    // PERFORMANCE: Load visible items first, then load rest
    QList<int> visibleIndexes;
    QList<int> hiddenIndexes;
    
    for (int i = 0; i < m_imagePaths.count(); ++i) {
        QString cacheKey = getCacheKey(m_imagePaths[i], m_thumbnailSize);
        if (m_thumbnailCache.contains(cacheKey)) continue;
        
        QListWidgetItem* item = this->item(i);
        if (item && visualItemRect(item).intersects(viewport()->rect())) {
            visibleIndexes.append(i);
        } else {
            hiddenIndexes.append(i);
        }
    }
    
    // Load visible items first with higher priority
    for (int i : visibleIndexes) {
        loadThumbnailAsync(i, QThread::HighPriority);
    }
    
    // Load hidden items with normal priority
    for (int i : hiddenIndexes) {
        loadThumbnailAsync(i, QThread::NormalPriority);
    }
}

void ThumbnailGrid::loadThumbnailAsync(int index, QThread::Priority priority) {
    if (index < 0 || index >= m_imagePaths.count()) return;
    
    QString path = m_imagePaths[index];
    QString cacheKey = getCacheKey(path, m_thumbnailSize);
    
    m_loadingTasks.fetchAndAddOrdered(1);
    
    QPointer<ThumbnailGrid> self(this);
    int size = m_thumbnailSize;
    
    // PERFORMANCE: Use managed thread pool
    QRunnable* task = QRunnable::create([self, path, index, cacheKey, size]() {
        if (!self) return;
        
        QPixmap thumb = self->generateThumbnail(path);
        
        // Save to disk cache
        self->saveToDiskCache(cacheKey, thumb);
        
        // Update UI in main thread
        QMetaObject::invokeMethod(self, [self, path, index, thumb, cacheKey]() {
            if (!self) return;
            
            self->m_thumbnailCache.insert(cacheKey, new QPixmap(thumb));
            
            if (index < self->count()) {
                QListWidgetItem* item = self->item(index);
                if (item && item->data(Qt::UserRole).toString() == path) {
                    item->setIcon(QIcon(thumb));
                }
            }
            
            self->m_loadingTasks.fetchAndAddOrdered(-1);
        }, Qt::QueuedConnection);
    });
    
    m_threadPool->start(task, priority);
}

QPixmap ThumbnailGrid::generateThumbnail(const QString& filepath) {
    // PERFORMANCE: Load with exact thumbnail size for faster decoding
    // Qt and LibRaw can decode at lower resolution directly
    auto imageOpt = ImageLoader::instance().load(filepath, 
        QSize(m_thumbnailSize * 2, m_thumbnailSize * 2));  // 2x for retina
    
    if (!imageOpt) {
        // Error placeholder
        QPixmap placeholder(m_thumbnailSize, m_thumbnailSize);
        placeholder.fill(QColor(80, 40, 40));
        return placeholder;
    }
    
    QImage image = *imageOpt;
    
    // PERFORMANCE: Fast scaling with SmoothTransformation only when needed
    if (image.width() > m_thumbnailSize * 3 || image.height() > m_thumbnailSize * 3) {
        // For very large images, do a fast scale first
        image = image.scaled(m_thumbnailSize * 2, m_thumbnailSize * 2, 
            Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    
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

QString ThumbnailGrid::getCacheKey(const QString& filepath, int size) {
    QFileInfo fi(filepath);
    return QString("%1_%2_%3")
        .arg(fi.fileName())
        .arg(fi.lastModified().toSecsSinceEpoch())
        .arg(size);
}

QPixmap ThumbnailGrid::loadFromDiskCache(const QString& cacheKey) {
    QString cachePath = m_diskCacheDir.filePath(cacheKey + ".jpg");
    if (QFile::exists(cachePath)) {
        QPixmap cached;
        if (cached.load(cachePath)) {
            return cached;
        }
    }
    return QPixmap();
}

void ThumbnailGrid::saveToDiskCache(const QString& cacheKey, const QPixmap& pixmap) {
    QString cachePath = m_diskCacheDir.filePath(cacheKey + ".jpg");
    pixmap.save(cachePath, "JPG", 85);  // 85% quality for thumbnails
}

} // namespace PhotoGuru
