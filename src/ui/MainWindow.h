#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QSplitter>
#include <QTabWidget>
#include <memory>
#include "core/PhotoMetadata.h"

namespace PhotoGuru {

class ImageViewer;
class ThumbnailGrid;
class MetadataPanel;
class SKPBrowser;
class MapView;
class TimelineView;
class SemanticSearch;
class FilterPanel;
class AnalysisPanel;
struct FilterCriteria;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    
    // Load images from directory
    void loadDirectory(const QString& path);
    
protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
private slots:
    void onOpenDirectory();
    void onOpenFiles();
    void onPreviousImage();
    void onNextImage();
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onZoom100();
    void onToggleFullscreen();
    void onSearchImages();
    void onRunAnalysis();
    void onPreferences();
    void onAbout();
    
    void onImageSelected(const QString& filepath);
    void onMetadataUpdated(const QString& filepath);
    void onFilterChanged(const FilterCriteria& criteria);
    void onViewModeChanged(int index);
    void onSemanticSearchResult(const QString& filepath);
    void onThumbnailSelectionChanged(int count);
    
    // New MVP features
    void onCopyFiles();
    void onMoveFiles();
    void onRenameFile();
    void onDeleteFiles();
    void onRevealInFinder();
    void onOpenWithExternal();
    void onThumbnailSizeChanged(int value);
    void onSortOrderChanged(int index);
    
    // Rating
    void onSetRating(int stars);
    void onIncreaseRating();
    void onDecreaseRating();
    
    // Google Takeout
    void onImportGoogleTakeout();
    
private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createDockPanels();
    void createStatusBar();
    void loadSettings();
    void saveSettings();
    void applyFilters();
    void refreshViews();
    void updateStatusBar();
    void setImageRating(const QString& filepath, int stars);
    void checkAndOfferGoogleTakeoutImport(const QString& directoryPath);
    
    // UI Components
    QToolBar* m_toolbar;
    QStatusBar* m_statusBar;
    
    // Central view modes (tabs)
    QTabWidget* m_centralTabs;
    ImageViewer* m_imageViewer;
    MapView* m_mapView;
    TimelineView* m_timelineView;
    
    // Dock panels (Lightroom-style)
    ThumbnailGrid* m_thumbnailGrid;
    
    QDockWidget* m_metadataDock;
    MetadataPanel* m_metadataPanel;
    
    QDockWidget* m_skpDock;
    SKPBrowser* m_skpBrowser;
    
    QDockWidget* m_searchDock;
    SemanticSearch* m_semanticSearch;
    
    QDockWidget* m_filterDock;
    FilterPanel* m_filterPanel;
    
    QDockWidget* m_analysisDock;
    AnalysisPanel* m_analysisPanel;
    
    // Current state
    QString m_currentDirectory;
    QStringList m_imageFiles;
    QList<PhotoMetadata> m_allPhotos;
    QList<PhotoMetadata> m_filteredPhotos;
    int m_currentIndex = -1;
};

} // namespace PhotoGuru
