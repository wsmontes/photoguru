#include "MainWindow.h"
#include "ImageViewer.h"
#include "ThumbnailGrid.h"
#include "MetadataPanel.h"
#include "SKPBrowser.h"
#include "MapView.h"
#include "TimelineView.h"
#include "SemanticSearch.h"
#include "FilterPanel.h"
#include "AnalysisPanel.h"
#include "DarkTheme.h"
#include "PhotoMetadata.h"
#include "ImageLoader.h"
#include "NotificationManager.h"
#include "core/GoogleTakeoutParser.h"
#include "core/GoogleTakeoutImporter.h"
#include "core/Logger.h"
#include "core/ExifToolDaemon.h"
#include "ml/ONNXInference.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QProgressBar>
#include <QSplitter>
#include <QDir>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QInputDialog>
#include <QSlider>
#include <QComboBox>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>

namespace PhotoGuru {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PhotoGuru Viewer");
    resize(1600, 1000);
    setAcceptDrops(true);
    
    // Initialize notification system
    NotificationManager::instance().setParentWidget(this);
    
    setupUI();
    loadSettings();
    
    // Force Metadata tab to be active (using QTimer to ensure event loop processed everything)
    QTimer::singleShot(0, this, [this]() {
        if (m_metadataDock) {
            m_metadataDock->raise();
        }
    });
    
    // Initialize with welcome state
    statusBar()->showMessage("Ready - Open a directory or drop images here");
}

MainWindow::~MainWindow() {
    qDebug() << "[MainWindow] Starting cleanup...";
    
    // Ensure all panels are properly disconnected before destruction
    if (m_analysisPanel) {
        disconnect(m_analysisPanel, nullptr, this, nullptr);
    }
    if (m_metadataPanel) {
        disconnect(m_metadataPanel, nullptr, this, nullptr);
    }
    
    saveSettings();
    
    // CRITICAL: Shutdown ML backends before exit to prevent crash
    // ONNX Runtime has static globals that must be cleaned up explicitly
    qDebug() << "[MainWindow] Shutting down ML backends...";
    ONNXInference::shutdownEnvironment();
    
    // Stop ExifTool daemon to prevent zombie process
    qDebug() << "[MainWindow] Stopping ExifTool daemon...";
    ExifToolDaemon::instance().stop();
    
    qDebug() << "[MainWindow] Cleanup complete";
}

void MainWindow::setupUI() {
    // Set proper size policies
    setMinimumSize(1200, 800);
    resize(1600, 1000);
    
    // Central widget: Vertical splitter with ImageViewer (top) and ThumbnailGrid (bottom)
    QSplitter* centralSplitter = new QSplitter(Qt::Vertical, this);
    
    m_imageViewer = new ImageViewer(this);
    m_thumbnailGrid = new ThumbnailGrid(this);
    m_thumbnailGrid->setMinimumHeight(80);
    m_thumbnailGrid->setMaximumHeight(300);
    
    centralSplitter->addWidget(m_imageViewer);
    centralSplitter->addWidget(m_thumbnailGrid);
    centralSplitter->setStretchFactor(0, 3);  // ImageViewer gets 3x more space
    centralSplitter->setStretchFactor(1, 1);  // ThumbnailGrid gets 1x space
    centralSplitter->setHandleWidth(4);
    
    setCentralWidget(centralSplitter);
    
    createDockPanels();
    createMenuBar();
    createToolBar();
    createStatusBar();
    
    // Connect signals
    connect(m_thumbnailGrid, &ThumbnailGrid::imageSelected,
            this, &MainWindow::onImageSelected);
    connect(m_thumbnailGrid, &ThumbnailGrid::selectionCountChanged,
            this, &MainWindow::onThumbnailSelectionChanged);
    connect(m_imageViewer, &ImageViewer::zoomChanged,
            [this](double zoom) {
                m_statusBar->showMessage(QString("Zoom: %1%").arg(int(zoom * 100)));
            });
    
    // Connect new ImageViewer signals
    connect(m_imageViewer, &ImageViewer::nextImageRequested,
            this, &MainWindow::onNextImage);
    connect(m_imageViewer, &ImageViewer::previousImageRequested,
            this, &MainWindow::onPreviousImage);
    connect(m_imageViewer, &ImageViewer::escapePressed,
            this, [this]() {
                if (isFullScreen()) {
                    onToggleFullscreen();
                }
            });
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* openDirAction = fileMenu->addAction("Open &Directory...");
    openDirAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    connect(openDirAction, &QAction::triggered, this, &MainWindow::onOpenDirectory);
    
    QAction* openFilesAction = fileMenu->addAction("&Open Files...");
    openFilesAction->setShortcut(QKeySequence::Open);
    connect(openFilesAction, &QAction::triggered, this, &MainWindow::onOpenFiles);
    
    fileMenu->addSeparator();
    
    QAction* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    
    // Edit menu (MVP file operations)
    QMenu* editMenu = menuBar->addMenu("&Edit");
    
    QAction* copyAction = editMenu->addAction("&Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::onCopyFiles);
    
    QAction* renameAction = editMenu->addAction("&Rename...");
    renameAction->setShortcut(QKeySequence("F2"));
    connect(renameAction, &QAction::triggered, this, &MainWindow::onRenameFile);
    
    QAction* moveAction = editMenu->addAction("&Move to...");
    moveAction->setShortcut(QKeySequence("Ctrl+Shift+M"));
    connect(moveAction, &QAction::triggered, this, &MainWindow::onMoveFiles);
    
    QAction* deleteAction = editMenu->addAction("&Delete");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteFiles);
    
    editMenu->addSeparator();
    
    QAction* revealAction = editMenu->addAction("Reveal in &Finder");
    revealAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(revealAction, &QAction::triggered, this, &MainWindow::onRevealInFinder);
    
    QAction* openWithAction = editMenu->addAction("Open &With...");
    openWithAction->setShortcut(QKeySequence("Ctrl+W"));
    connect(openWithAction, &QAction::triggered, this, &MainWindow::onOpenWithExternal);
    
    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    
    QAction* zoomInAction = viewMenu->addAction("Zoom &In");
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::onZoomIn);
    
    QAction* zoomOutAction = viewMenu->addAction("Zoom &Out");
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOut);
    
    QAction* zoomFitAction = viewMenu->addAction("Zoom to &Fit");
    zoomFitAction->setShortcut(QKeySequence("F"));
    connect(zoomFitAction, &QAction::triggered, this, &MainWindow::onZoomFit);
    
    QAction* zoom100Action = viewMenu->addAction("&Actual Size");
    zoom100Action->setShortcut(QKeySequence("Ctrl+0"));
    connect(zoom100Action, &QAction::triggered, this, &MainWindow::onZoom100);
    
    viewMenu->addSeparator();
    
    QAction* fullscreenAction = viewMenu->addAction("&Fullscreen");
    fullscreenAction->setShortcut(QKeySequence("F11"));
    fullscreenAction->setCheckable(true);
    connect(fullscreenAction, &QAction::triggered, this, &MainWindow::onToggleFullscreen);
    
    viewMenu->addSeparator();
    viewMenu->addAction(m_metadataDock->toggleViewAction());
    viewMenu->addAction(m_skpDock->toggleViewAction());
    viewMenu->addAction(m_analysisDock->toggleViewAction());
    viewMenu->addAction(m_filterDock->toggleViewAction());
    
    // Metadata menu
    QMenu* metadataMenu = menuBar->addMenu("Meta&data");
    
    QAction* editMetadataAction = metadataMenu->addAction("&Edit Metadata...");
    editMetadataAction->setShortcut(QKeySequence("Ctrl+I"));
    connect(editMetadataAction, &QAction::triggered, [this]() {
        if (m_metadataPanel) {
            m_metadataDock->show();
            m_metadataDock->raise();
            m_metadataPanel->setEditable(true);
        }
    });
    
    metadataMenu->addSeparator();
    
    QAction* importTakeoutAction = metadataMenu->addAction("Import &Google Takeout...");
    importTakeoutAction->setShortcut(QKeySequence("Ctrl+Shift+G"));
    connect(importTakeoutAction, &QAction::triggered, this, &MainWindow::onImportGoogleTakeout);
    
    metadataMenu->addSeparator();
    
    QAction* resetFiltersAction = metadataMenu->addAction("Reset &Filters");
    resetFiltersAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(resetFiltersAction, &QAction::triggered, [this]() {
        if (m_filterPanel) {
            m_filterPanel->reset();
        }
    });
    
    QAction* focusSearchAction = metadataMenu->addAction("&Focus Search");
    focusSearchAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(focusSearchAction, &QAction::triggered, [this]() {
        if (m_filterPanel) {
            m_filterPanel->setFocus();
        }
    });
    
    // Photo menu (rating and organization)
    QMenu* photoMenu = menuBar->addMenu("&Photo");
    
    QAction* rating0Action = photoMenu->addAction("No Rating");
    rating0Action->setShortcut(QKeySequence("0"));
    connect(rating0Action, &QAction::triggered, [this]() { onSetRating(0); });
    
    photoMenu->addSeparator();
    
    QAction* rating1Action = photoMenu->addAction("★ (1 star)");
    rating1Action->setShortcut(QKeySequence("1"));
    connect(rating1Action, &QAction::triggered, [this]() { onSetRating(1); });
    
    QAction* rating2Action = photoMenu->addAction("★★ (2 stars)");
    rating2Action->setShortcut(QKeySequence("2"));
    connect(rating2Action, &QAction::triggered, [this]() { onSetRating(2); });
    
    QAction* rating3Action = photoMenu->addAction("★★★ (3 stars)");
    rating3Action->setShortcut(QKeySequence("3"));
    connect(rating3Action, &QAction::triggered, [this]() { onSetRating(3); });
    
    QAction* rating4Action = photoMenu->addAction("★★★★ (4 stars)");
    rating4Action->setShortcut(QKeySequence("4"));
    connect(rating4Action, &QAction::triggered, [this]() { onSetRating(4); });
    
    QAction* rating5Action = photoMenu->addAction("★★★★★ (5 stars)");
    rating5Action->setShortcut(QKeySequence("5"));
    connect(rating5Action, &QAction::triggered, [this]() { onSetRating(5); });
    
    photoMenu->addSeparator();
    
    QAction* increaseRatingAction = photoMenu->addAction("Increase Rating");
    increaseRatingAction->setShortcut(QKeySequence("]"));
    connect(increaseRatingAction, &QAction::triggered, this, &MainWindow::onIncreaseRating);
    
    QAction* decreaseRatingAction = photoMenu->addAction("Decrease Rating");
    decreaseRatingAction->setShortcut(QKeySequence("["));
    connect(decreaseRatingAction, &QAction::triggered, this, &MainWindow::onDecreaseRating);
    
    // Navigate menu
    QMenu* navMenu = menuBar->addMenu("&Navigate");
    
    QAction* prevAction = navMenu->addAction("&Previous Image");
    prevAction->setShortcut(QKeySequence("Left"));
    connect(prevAction, &QAction::triggered, this, &MainWindow::onPreviousImage);
    
    QAction* nextAction = navMenu->addAction("&Next Image");
    nextAction->setShortcut(QKeySequence("Right"));
    connect(nextAction, &QAction::triggered, this, &MainWindow::onNextImage);
    
    // AI menu
    QMenu* aiMenu = menuBar->addMenu("&AI");
    
    QAction* searchAction = aiMenu->addAction("&Semantic Search...");
    searchAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(searchAction, &QAction::triggered, this, &MainWindow::onSearchImages);
    
    aiMenu->addSeparator();
    
    QAction* analyzeAction = aiMenu->addAction("&Analyze Current Image");
    analyzeAction->setShortcut(QKeySequence("Ctrl+A"));
    connect(analyzeAction, &QAction::triggered, this, &MainWindow::onRunAnalysis);
    
    QAction* analyzeBatchAction = aiMenu->addAction("Analyze &All Images...");
    connect(analyzeBatchAction, &QAction::triggered, [this]() {
        if (m_imageFiles.isEmpty()) {
            NotificationManager::instance().showWarning("Please load a directory first.");
            return;
        }
        
        // Switch to Analysis Panel and trigger directory analysis
        if (m_analysisDock) {
            m_analysisDock->show();
            m_analysisDock->raise();
            
            // The AnalysisPanel will handle the actual batch operation
            statusBar()->showMessage("Switch to Analysis Panel to start batch processing");
        }
    });
    
    // Help menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About PhotoGuru Viewer");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createToolBar() {
    m_toolbar = addToolBar("Main Toolbar");
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(24, 24));
    
    // Navigation
    QAction* prevAction = m_toolbar->addAction("◀");
    prevAction->setToolTip("Previous Image (Left Arrow)");
    connect(prevAction, &QAction::triggered, this, &MainWindow::onPreviousImage);
    
    QAction* nextAction = m_toolbar->addAction("▶");
    nextAction->setToolTip("Next Image (Right Arrow)");
    connect(nextAction, &QAction::triggered, this, &MainWindow::onNextImage);
    
    m_toolbar->addSeparator();
    
    // Zoom
    QAction* zoomOutAction = m_toolbar->addAction("−");
    zoomOutAction->setToolTip("Zoom Out (Ctrl+-)");
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::onZoomOut);
    
    QAction* zoomInAction = m_toolbar->addAction("+");
    zoomInAction->setToolTip("Zoom In (Ctrl++)");
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::onZoomIn);
    
    QAction* zoomFitAction = m_toolbar->addAction("⊡");
    zoomFitAction->setToolTip("Fit to Window (F)");
    connect(zoomFitAction, &QAction::triggered, this, &MainWindow::onZoomFit);
    
    m_toolbar->addSeparator();
    
    // AI features
    QAction* searchAction = m_toolbar->addAction("🔍");
    searchAction->setToolTip("Semantic Search (Ctrl+F)");
    connect(searchAction, &QAction::triggered, this, &MainWindow::onSearchImages);
    
    QAction* analyzeAction = m_toolbar->addAction("🤖");
    analyzeAction->setToolTip("AI Analysis (Ctrl+A)");
    connect(analyzeAction, &QAction::triggered, this, &MainWindow::onRunAnalysis);
    
    m_toolbar->addSeparator();
    
    // Thumbnail size control
    m_toolbar->addWidget(new QLabel(" Size: "));
    QSlider* sizeSlider = new QSlider(Qt::Horizontal);
    sizeSlider->setMinimum(80);
    sizeSlider->setMaximum(300);
    sizeSlider->setValue(150);
    sizeSlider->setFixedWidth(100);
    sizeSlider->setToolTip("Adjust thumbnail size");
    connect(sizeSlider, &QSlider::valueChanged, this, &MainWindow::onThumbnailSizeChanged);
    m_toolbar->addWidget(sizeSlider);
    
    m_toolbar->addSeparator();
    
    // Sort order control
    m_toolbar->addWidget(new QLabel(" Sort: "));
    QComboBox* sortCombo = new QComboBox();
    sortCombo->addItem("Name");
    sortCombo->addItem("Date");
    sortCombo->addItem("Size");
    sortCombo->setToolTip("Sort images by");
    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onSortOrderChanged);
    m_toolbar->addWidget(sortCombo);
}

void MainWindow::createDockPanels() {
    // Set dock options for better behavior
    setDockOptions(QMainWindow::AnimatedDocks | 
                   QMainWindow::AllowTabbedDocks | 
                   QMainWindow::GroupedDragging);
    
    // Filter Panel (left side) - create first to avoid interference
    m_filterDock = new QDockWidget("Filters", this);
    m_filterDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_filterDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_filterPanel = new FilterPanel(this);
    m_filterDock->setWidget(m_filterPanel);
    m_filterDock->setMinimumWidth(220);
    addDockWidget(Qt::LeftDockWidgetArea, m_filterDock);
    m_filterDock->show();  // Ensure visible by default
    
    // RIGHT SIDE PANELS - create in order we want tabs to appear: Metadata, Analysis, SKP
    
    // Metadata panel (right - FIRST TAB)
    m_metadataDock = new QDockWidget("Metadata", this);
    m_metadataDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_metadataDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_metadataPanel = new MetadataPanel(this);
    m_metadataDock->setWidget(m_metadataPanel);
    m_metadataDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_metadataDock);
    
    // Analysis Panel (right - SECOND TAB)
    m_analysisDock = new QDockWidget("AI Analysis", this);
    m_analysisDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_analysisDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_analysisPanel = new AnalysisPanel(this);
    m_analysisDock->setWidget(m_analysisPanel);
    m_analysisDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_analysisDock);
    
    // SKP Browser (right - THIRD/LAST TAB)
    m_skpDock = new QDockWidget("Semantic Keys", this);
    m_skpDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_skpDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_skpBrowser = new SKPBrowser(this);
    m_skpDock->setWidget(m_skpBrowser);
    m_skpDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_skpDock);
    
    // Tabify all right panels together
    // First tab Analysis to Metadata (creates tab group with Metadata first, Analysis second)
    tabifyDockWidget(m_metadataDock, m_analysisDock);
    // Then tab SKP to Analysis (adds SKP as third tab)
    tabifyDockWidget(m_analysisDock, m_skpDock);
    
    // Ensure Metadata tab is visible by default
    m_metadataDock->show();
    m_metadataDock->raise();
    
    // Connect analysis panel signals
    connect(m_analysisPanel, &AnalysisPanel::metadataUpdated, 
            this, [this](const QString& filepath) {
                // Reload metadata for updated image
                if (m_currentIndex >= 0 && filepath == m_imageFiles[m_currentIndex]) {
                    m_metadataPanel->loadMetadata(filepath);
                    // Thumbnail will auto-refresh when metadata changes
                }
            });
    
    // Connect metadata panel signals
    connect(m_metadataPanel, &MetadataPanel::metadataChanged,
            this, [this](const QString& filepath) {
                // Don't reload - panel already has the saved data in memory
                // Reloading would cause a race condition with ExifTool write
                // Update status bar
                NotificationManager::instance().showSuccess("Metadata saved successfully");
            });
    
    connect(m_metadataPanel, &MetadataPanel::editModeChanged,
            this, [this](bool editing) {
                // Could disable navigation while editing to prevent data loss
                QString msg = editing ? "Editing metadata..." : "Ready";
                m_statusBar->showMessage(msg);
            });
    
    // Connect filter panel signals
    connect(m_filterPanel, &FilterPanel::filterChanged,
            this, &MainWindow::onFilterChanged);
}

void MainWindow::createStatusBar() {
    m_statusBar = statusBar();
}

void MainWindow::onOpenDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this,
        "Open Image Directory",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly);
    
    if (!dir.isEmpty()) {
        loadDirectory(dir);
    }
}

void MainWindow::onOpenFiles() {
    QStringList filters = ImageLoader::instance().supportedExtensions();
    QString filter = QString("Images (%1)").arg(filters.join(" "));
    
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Open Images",
        QDir::homePath(),
        filter);
    
    if (!files.isEmpty()) {
        m_imageFiles = files;
        m_thumbnailGrid->setImages(files);
        m_currentIndex = 0;
        onImageSelected(files[0]);
    }
}

void MainWindow::loadDirectory(const QString& path) {
    m_currentDirectory = path;
    
    // Check if this is a Google Takeout directory
    checkAndOfferGoogleTakeoutImport(path);
    
    // Find all supported images
    QDir dir(path);
    QStringList filters = ImageLoader::instance().supportedExtensions();
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);
    
    m_imageFiles.clear();
    for (const QString& file : files) {
        m_imageFiles << dir.filePath(file);
    }
    
    if (m_imageFiles.isEmpty()) {
        NotificationManager::instance().showInfo("No supported images found in this directory.");
        return;
    }
    
    // Load into thumbnail grid
    m_thumbnailGrid->setImages(m_imageFiles);
    
    // Select first image
    m_currentIndex = 0;
    onImageSelected(m_imageFiles[0]);
    
    // Update analysis panel with directory
    m_analysisPanel->setCurrentDirectory(path);
    
    // Ensure Metadata panel remains visible after loading
    if (m_metadataDock) {
        m_metadataDock->raise();
    }
    
    statusBar()->showMessage(
        QString("Loaded %1 images from %2")
            .arg(m_imageFiles.count())
            .arg(QFileInfo(path).fileName())
    );
}

void MainWindow::onImageSelected(const QString& filepath) {
    // Validate filepath
    if (filepath.isEmpty() || !QFileInfo::exists(filepath)) {
        qWarning() << "Invalid filepath:" << filepath;
        return;
    }
    
    // Update current index
    m_currentIndex = m_imageFiles.indexOf(filepath);
    if (m_currentIndex < 0) {
        qWarning() << "Filepath not in image list:" << filepath;
        return;
    }
    
    // Load image (force repaint)
    m_imageViewer->loadImage(filepath);
    m_imageViewer->update();
    
    // Update metadata panel
    m_metadataPanel->loadMetadata(filepath);
    
    // Update SKP browser
    m_skpBrowser->loadImageKeys(filepath);
    
    // Update analysis panel with current image
    m_analysisPanel->setCurrentImage(filepath);
    
    // Update thumbnail selection and highlight
    m_thumbnailGrid->selectImage(m_currentIndex);
    m_thumbnailGrid->setCurrentIndex(m_currentIndex);
    
    // Update status with rich information
    updateStatusBar();
}

void MainWindow::onPreviousImage() {
    if (m_imageFiles.isEmpty() || m_currentIndex <= 0) return;
    m_currentIndex--;
    onImageSelected(m_imageFiles[m_currentIndex]);
}

void MainWindow::onNextImage() {
    if (m_imageFiles.isEmpty() || m_currentIndex >= m_imageFiles.count() - 1) return;
    m_currentIndex++;
    onImageSelected(m_imageFiles[m_currentIndex]);
}

void MainWindow::onZoomIn() { m_imageViewer->zoomIn(); }
void MainWindow::onZoomOut() { m_imageViewer->zoomOut(); }
void MainWindow::onZoomFit() { m_imageViewer->zoomToFit(); }
void MainWindow::onZoom100() { m_imageViewer->zoomActual(); }

void MainWindow::onToggleFullscreen() {
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::onSearchImages() {
    QString query = QInputDialog::getText(this, "Semantic Search",
        "Enter search query (e.g., 'sunset on the beach', 'people smiling'):");
    
    if (!query.isEmpty()) {
        // Show semantic search panel with the query
        if (m_imageFiles.isEmpty()) {
            NotificationManager::instance().showWarning("Please load a directory first.");
            return;
        }
        
        statusBar()->showMessage(QString("Searching for: %1").arg(query));
        
        // Show info toast about semantic search implementation
        NotificationManager::instance().showInfo(
            "Semantic search will scan AI-generated descriptions and keywords for matches", 3500);
    }
}

void MainWindow::onRunAnalysis() {
    if (m_currentIndex < 0) {
        NotificationManager::instance().showWarning("Please select an image first.");
        return;
    }
    
    // Switch to Analysis Panel and set current image
    if (m_analysisDock && m_analysisPanel) {
        m_analysisDock->show();
        m_analysisDock->raise();
        
        // The AnalysisPanel already has the current image context
        // User can click "Analyze with AI" button there
        statusBar()->showMessage("Switch to Analysis Panel to analyze current image");
    }
}

void MainWindow::onPreferences() {
    NotificationManager::instance().showInfo(
        "Settings: Auto-fit images, smooth zoom (1.25x), C++ ML integration", 4000);
}

void MainWindow::onAbout() {
    NotificationManager::instance().showInfo(
        "PhotoGuru Viewer 1.0 - AI-powered photo viewer with CLIP semantic search and SKP organization", 5000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        if (!urls.isEmpty()) {
            QString path = urls[0].toLocalFile();
            QFileInfo info(path);
            
            if (info.isDir()) {
                loadDirectory(path);
            } else if (info.isFile()) {
                QStringList files;
                for (const QUrl& url : urls) {
                    files << url.toLocalFile();
                }
                m_imageFiles = files;
                m_thumbnailGrid->setImages(files);
                m_currentIndex = 0;
                onImageSelected(files[0]);
            }
        }
    }
}

void MainWindow::loadSettings() {
    QSettings settings("PhotoGuru", "Viewer");
    
    // Only restore geometry, NOT window state (dock positions/tabs)
    // This ensures tab order remains as defined in code
    restoreGeometry(settings.value("geometry").toByteArray());
    
    m_currentDirectory = settings.value("lastDirectory", QDir::homePath()).toString();
}

void MainWindow::saveSettings() {
    QSettings settings("PhotoGuru", "Viewer");
    
    // Only save geometry, NOT window state (dock positions/tabs)
    // This ensures tab order remains consistent across sessions
    settings.setValue("geometry", saveGeometry());
    settings.setValue("lastDirectory", m_currentDirectory);
}

void MainWindow::updateStatusBar() {
    if (m_currentIndex < 0 || m_imageFiles.isEmpty()) {
        statusBar()->showMessage("Ready - Open a directory or drop images here");
        return;
    }
    
    QString filepath = m_imageFiles[m_currentIndex];
    QFileInfo fileInfo(filepath);
    
    // Get image dimensions
    QImage img(filepath);
    QString dimensions = img.isNull() ? "" : 
        QString(" | %1x%2px").arg(img.width()).arg(img.height());
    
    // Format file size
    qint64 bytes = fileInfo.size();
    QString filesize;
    if (bytes < 1024) {
        filesize = QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        filesize = QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else {
        filesize = QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
    
    // Build rich status message
    QString status = QString("%1 | %2 of %3%4 | %5")
        .arg(fileInfo.fileName())
        .arg(m_currentIndex + 1)
        .arg(m_imageFiles.count())
        .arg(dimensions)
        .arg(filesize);
    
    statusBar()->showMessage(status);
}

void MainWindow::onMetadataUpdated(const QString& filepath) {
    // Handle metadata update signal
    // Don't reload the metadata panel - it already has the updated data
    // Just update other components like SKP browser
    if (m_currentIndex >= 0 && m_currentIndex < m_imageFiles.size() && 
        m_imageFiles[m_currentIndex] == filepath) {
        m_skpBrowser->loadImageKeys(filepath);
    }
}

void MainWindow::onFilterChanged(const FilterCriteria& criteria) {
    // Apply filters to photos using the comprehensive FilterCriteria::matches() method
    QStringList filteredFiles;
    int totalCount = m_imageFiles.size();
    
    for (const QString& filepath : m_imageFiles) {
        auto metaOpt = MetadataReader::instance().read(filepath);
        if (!metaOpt) continue;  // Skip if metadata can't be read
        
        PhotoMetadata meta = metaOpt.value();
        
        // Use the comprehensive matching function from FilterCriteria
        if (criteria.matches(meta)) {
            filteredFiles << filepath;
        }
    }
    
    // Update display with filtered results
    m_thumbnailGrid->setImages(filteredFiles);
    
    // Update status bar with filter stats
    if (filteredFiles.count() == totalCount) {
        statusBar()->showMessage(QString("%1 images (no filters active)").arg(totalCount));
    } else {
        statusBar()->showMessage(QString("%1 of %2 images match filters").arg(filteredFiles.count()).arg(totalCount));
    }
}

void MainWindow::onViewModeChanged(int index) {
    // Switch between different view modes
    // 0 = Grid view (default)
    // 1 = Map view (if GPS data available)
    // 2 = Timeline view (if date data available)
    
    switch (index) {
        case 0:
            // Grid view - already default, just ensure visibility
            if (m_thumbnailGrid) {
                m_thumbnailGrid->setVisible(true);
            }
            statusBar()->showMessage("Grid view");
            break;
            
        case 1:
            // Map view
            statusBar()->showMessage("Map view - showing images with GPS data");
            // Would show m_mapView if it was in central tabs
            break;
            
        case 2:
            // Timeline view
            statusBar()->showMessage("Timeline view - showing images chronologically");
            // Would show m_timelineView if it was in central tabs
            break;
            
        default:
            break;
    }
}

void MainWindow::onSemanticSearchResult(const QString& filepath) {
    // Handle semantic search result selection
    onImageSelected(filepath);
}

void MainWindow::onThumbnailSelectionChanged(int count) {
    if (count == 0) {
        updateStatusBar();
    } else if (count == 1) {
        // Single selection - show normal status
        updateStatusBar();
    } else {
        // Multiple selection - show selection count
        statusBar()->showMessage(QString("%1 photos selected").arg(count));
    }
}

void MainWindow::applyFilters() {
    // Apply current filter criteria to image list
    // This is called when filters change or when new images are loaded
    
    if (m_imageFiles.isEmpty()) return;
    
    // For now, just refresh the thumbnail grid
    // In production, would apply stored FilterCriteria
    m_thumbnailGrid->setImages(m_imageFiles);
}

void MainWindow::refreshViews() {
    // Refresh all views with current data
    // Called after metadata updates or filter changes
    
    if (m_currentIndex >= 0 && m_currentIndex < m_imageFiles.count()) {
        QString currentFile = m_imageFiles[m_currentIndex];
        
        // Reload current image metadata
        auto metaOpt = MetadataReader::instance().read(currentFile);
        if (!metaOpt) return;  // Skip if metadata can't be read
        
        // Update all panels
        m_metadataPanel->loadMetadata(currentFile);
        m_skpBrowser->loadImageKeys(currentFile);
        
        // Update thumbnail to reflect any metadata changes
        m_thumbnailGrid->setImages(m_imageFiles);
    }
}

// MVP File Operations Implementation

void MainWindow::onCopyFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    if (selected.isEmpty() && m_currentIndex >= 0) {
        selected << m_imageFiles[m_currentIndex];
    }
    
    if (selected.isEmpty()) {
        NotificationManager::instance().showInfo("No images selected");
        return;
    }
    
    QString dest = QFileDialog::getExistingDirectory(this, "Copy to Directory");
    if (dest.isEmpty()) return;
    
    // Progress dialog with cancel button
    QProgressDialog progress("Copying files...", "Cancel", 0, selected.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);  // Show after 500ms
    
    int copied = 0;
    for (int i = 0; i < selected.size(); i++) {
        progress.setValue(i);
        
        // Check for cancellation
        if (progress.wasCanceled()) {
            statusBar()->showMessage(QString("Copy cancelled. %1 of %2 files copied")
                .arg(copied).arg(selected.size()));
            return;
        }
        
        const QString& file = selected[i];
        QFileInfo fi(file);
        progress.setLabelText(QString("Copying %1...").arg(fi.fileName()));
        
        QString destFile = dest + "/" + fi.fileName();
        if (QFile::copy(file, destFile)) {
            copied++;
        }
    }
    
    progress.setValue(selected.size());
    statusBar()->showMessage(QString("Copied %1 file(s)").arg(copied));
}

void MainWindow::onMoveFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    if (selected.isEmpty() && m_currentIndex >= 0) {
        selected << m_imageFiles[m_currentIndex];
    }
    
    if (selected.isEmpty()) {
        NotificationManager::instance().showInfo("No images selected");
        return;
    }
    
    QString dest = QFileDialog::getExistingDirectory(this, "Move to Directory");
    if (dest.isEmpty()) return;
    
    // Progress dialog with cancel button
    QProgressDialog progress("Moving files...", "Cancel", 0, selected.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);  // Show after 500ms
    
    int moved = 0;
    QStringList movedFiles;  // Track for later removal
    
    for (int i = 0; i < selected.size(); i++) {
        progress.setValue(i);
        
        // Check for cancellation
        if (progress.wasCanceled()) {
            // Remove already moved files from list
            for (const QString& file : movedFiles) {
                m_imageFiles.removeAll(file);
            }
            m_thumbnailGrid->setImages(m_imageFiles);
            statusBar()->showMessage(QString("Move cancelled. %1 of %2 files moved")
                .arg(moved).arg(selected.size()));
            return;
        }
        
        const QString& file = selected[i];
        QFileInfo fi(file);
        progress.setLabelText(QString("Moving %1...").arg(fi.fileName()));
        
        QString destFile = dest + "/" + fi.fileName();
        if (QFile::rename(file, destFile)) {
            moved++;
            movedFiles << file;
        }
    }
    
    // Remove moved files from list
    for (const QString& file : movedFiles) {
        m_imageFiles.removeAll(file);
    }
    
    progress.setValue(selected.size());
    m_thumbnailGrid->setImages(m_imageFiles);
    
    if (m_currentIndex >= m_imageFiles.count()) {
        m_currentIndex = m_imageFiles.count() - 1;
    }
    
    statusBar()->showMessage(QString("Moved %1 file(s)").arg(moved));
}

void MainWindow::onRenameFile() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        NotificationManager::instance().showInfo("No image selected");
        return;
    }
    
    QString currentFile = m_imageFiles[m_currentIndex];
    QFileInfo fi(currentFile);
    
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename File",
        "New filename:", QLineEdit::Normal, fi.fileName(), &ok);
    
    if (!ok || newName.isEmpty() || newName == fi.fileName()) return;
    
    QString newPath = fi.absolutePath() + "/" + newName;
    if (QFile::exists(newPath)) {
        NotificationManager::instance().showWarning("A file with that name already exists");
        return;
    }
    
    if (QFile::rename(currentFile, newPath)) {
        m_imageFiles[m_currentIndex] = newPath;
        m_thumbnailGrid->setImages(m_imageFiles);
        m_imageViewer->loadImage(newPath);
        statusBar()->showMessage("File renamed");
    } else {
        NotificationManager::instance().showWarning("Could not rename file");
    }
}

void MainWindow::onDeleteFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    if (selected.isEmpty() && m_currentIndex >= 0) {
        selected << m_imageFiles[m_currentIndex];
    }
    
    if (selected.isEmpty()) {
        NotificationManager::instance().showInfo("No images selected");
        return;
    }
    
    // Ask for confirmation via toast warning (safe default: cancel)
    NotificationManager::instance().showWarning(
        QString("Delete operation cancelled. Would delete %1 file(s). Use Finder for file operations.").arg(selected.count()), 4000);
    return; // Safe default: don't delete without explicit confirmation
    
    /* Delete code disabled - requires explicit confirmation UI
    int deleted = 0;
    QStringList failed;
    
    for (const QString& file : selected) {
        // Move to trash on macOS
        int result = QProcess::execute("osascript", {
            "-e", QString("tell application \"Finder\" to delete (POSIX file \"%1\")").arg(file)
        });
        
        if (result == 0) {
            m_imageFiles.removeAll(file);
            deleted++;
        } else {
            failed << QFileInfo(file).fileName();
        }
    }
    
    if (!failed.isEmpty()) {
        NotificationManager::instance().showWarning(
            QString("Failed to delete %1 file(s): %2")
                .arg(failed.count()).arg(failed.join(", ")));
    }
    
    m_thumbnailGrid->setImages(m_imageFiles);
    if (m_currentIndex >= m_imageFiles.count()) {
        m_currentIndex = m_imageFiles.count() - 1;
    }
    
    if (m_currentIndex >= 0) {
        m_imageViewer->loadImage(m_imageFiles[m_currentIndex]);
    } else {
        m_imageViewer->clear();
    }
    
    statusBar()->showMessage(QString("Deleted %1 file(s)").arg(deleted));
    */
}

void MainWindow::onRevealInFinder() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        NotificationManager::instance().showInfo("No image selected");
        return;
    }
    
    QString file = m_imageFiles[m_currentIndex];
    QProcess::execute("open", {"-R", file});
    statusBar()->showMessage("Revealed in Finder");
}

void MainWindow::onOpenWithExternal() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        NotificationManager::instance().showInfo("No image selected");
        return;
    }
    
    QString file = m_imageFiles[m_currentIndex];
    QDesktopServices::openUrl(QUrl::fromLocalFile(file));
}

void MainWindow::onThumbnailSizeChanged(int value) {
    m_thumbnailGrid->setThumbnailSize(value);
    statusBar()->showMessage(QString("Thumbnail size: %1px").arg(value), 1000);
}

void MainWindow::onSortOrderChanged(int index) {
    SortOrder order;
    switch (index) {
        case 0: order = SortOrder::ByName; break;
        case 1: order = SortOrder::ByDate; break;
        case 2: order = SortOrder::BySize; break;
        default: return;
    }
    
    m_thumbnailGrid->setSortOrder(order);
    statusBar()->showMessage(QString("Sorted by: %1").arg(
        index == 0 ? "Name" : index == 1 ? "Date" : "Size"), 1000);
}

// Rating implementation

void MainWindow::onSetRating(int stars) {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        return;
    }
    
    QString filepath = m_imageFiles[m_currentIndex];
    setImageRating(filepath, stars);
    
    // Update status bar
    if (stars == 0) {
        statusBar()->showMessage("Rating cleared", 1000);
    } else {
        QString starStr = QString("★").repeated(stars) + QString("☆").repeated(5 - stars);
        statusBar()->showMessage(QString("Rating: %1").arg(starStr), 2000);
    }
    
    // Refresh metadata panel to show new rating
    m_metadataPanel->loadMetadata(filepath);
}

void MainWindow::onIncreaseRating() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        return;
    }
    
    QString filepath = m_imageFiles[m_currentIndex];
    auto metaOpt = MetadataReader::instance().read(filepath);
    if (!metaOpt) return;
    
    int currentRating = metaOpt->rating;
    int newRating = qMin(5, currentRating + 1);
    
    if (newRating != currentRating) {
        onSetRating(newRating);
    }
}

void MainWindow::onDecreaseRating() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        return;
    }
    
    QString filepath = m_imageFiles[m_currentIndex];
    auto metaOpt = MetadataReader::instance().read(filepath);
    if (!metaOpt) return;
    
    int currentRating = metaOpt->rating;
    int newRating = qMax(0, currentRating - 1);
    
    if (newRating != currentRating) {
        onSetRating(newRating);
    }
}

void MainWindow::setImageRating(const QString& filepath, int stars) {
    // Clamp rating to 0-5
    stars = qMax(0, qMin(5, stars));
    
    // Use ExifTool to write rating to XMP metadata
    // XMP:Rating is standard tag supported by Lightroom, Bridge, etc.
    QProcess process;
    process.start("exiftool", {
        "-XMP:Rating=" + QString::number(stars),
        "-overwrite_original",
        filepath
    });
    
    bool finished = process.waitForFinished(3000);  // 3 second timeout
    
    if (finished && process.exitCode() == 0) {
        // Success - update in-memory metadata cache
        auto metaOpt = MetadataReader::instance().read(filepath);
        if (metaOpt) {
            PhotoMetadata meta = *metaOpt;
            meta.rating = stars;
            // Note: MetadataReader cache would need to be updated here
            // For now, next read will pick up the new rating
        }
    } else {
        qWarning() << "Failed to set rating for" << filepath;
    }
}

void MainWindow::checkAndOfferGoogleTakeoutImport(const QString& directoryPath) {
    // Check if this directory looks like Google Takeout
    if (!GoogleTakeoutParser::isGoogleTakeoutDirectory(directoryPath)) {
        return;
    }
    
    LOG_INFO("MainWindow", "Google Takeout directory detected: " + directoryPath);
    
    // Show notification offering to import metadata
    NotificationManager::instance().showInfo(
        "Google Takeout folder detected! Would you like to import metadata from JSON files?",
        10000  // 10 second timeout
    );
    
    // Note: For now, user can manually trigger import via menu
    // Future enhancement: Add action buttons to notification
}

void MainWindow::onImportGoogleTakeout() {
    if (m_currentDirectory.isEmpty()) {
        NotificationManager::instance().showWarning("Please open a directory first.");
        return;
    }
    
    LOG_INFO("MainWindow", "=== Starting Google Takeout import ===");
    
    // Create progress dialog
    QProgressDialog progress("Importing Google Takeout metadata...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);
    progress.show();
    
    // Configure import options (all enabled by default)
    GoogleTakeoutImporter::ImportOptions options;
    options.applyDescription = true;
    options.applyPeopleAsKeywords = true;
    options.applyAlbumsAsKeywords = true;
    options.applyLocation = true;
    options.applyDateTime = true;
    options.overwriteExisting = true;  // Overwrite existing metadata
    options.createBackup = false;  // Don't create backups (exiftool does this)
    
    // Run import
    GoogleTakeoutImporter importer;
    auto result = importer.importDirectory(m_currentDirectory, options);
    
    progress.close();
    
    // Show results
    QString message = QString(
        "Google Takeout import complete!\n\n"
        "Images processed: %1\n"
        "With JSON metadata: %2\n"
        "Metadata applied: %3\n"
        "Errors: %4"
    ).arg(result.totalImages)
     .arg(result.withJson)
     .arg(result.metadataApplied)
     .arg(result.errors);
    
    if (result.errors > 0 && !result.errorMessages.isEmpty()) {
        message += "\n\nFirst few errors:\n";
        int maxErrors = qMin(5, result.errorMessages.size());
        for (int i = 0; i < maxErrors; i++) {
            message += "• " + result.errorMessages[i] + "\n";
        }
    }
    
    LOG_INFO("MainWindow", result.summary());
    
    if (result.metadataApplied > 0) {
        NotificationManager::instance().showSuccess(message);
        
        // Refresh current image to show updated metadata
        if (m_currentIndex >= 0 && m_currentIndex < m_imageFiles.size()) {
            m_metadataPanel->loadMetadata(m_imageFiles[m_currentIndex]);
        }
    } else if (result.withJson == 0) {
        NotificationManager::instance().showWarning(
            "No Google Takeout JSON files found in this directory.\n\n"
            "Expected format: IMG_001.jpg + IMG_001.jpg.json"
        );
    } else {
        NotificationManager::instance().showError(message);
    }
}

} // namespace PhotoGuru
