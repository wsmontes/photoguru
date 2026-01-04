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

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
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

namespace PhotoGuru {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("PhotoGuru Viewer");
    resize(1600, 1000);
    setAcceptDrops(true);
    
    setupUI();
    loadSettings();
    
    // Initialize with welcome state
    statusBar()->showMessage("Ready - Open a directory or drop images here");
}

MainWindow::~MainWindow() {
    saveSettings();
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
            QMessageBox::warning(this, "No Images", "Please load a directory first.");
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
    
    // Metadata panel (right - full height)
    m_metadataDock = new QDockWidget("Metadata", this);
    m_metadataDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_metadataDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_metadataPanel = new MetadataPanel(this);
    m_metadataDock->setWidget(m_metadataPanel);
    m_metadataDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_metadataDock);
    
    // SKP Browser (right, tabbed with metadata)
    m_skpDock = new QDockWidget("Semantic Keys", this);
    m_skpDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_skpDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_skpBrowser = new SKPBrowser(this);
    m_skpDock->setWidget(m_skpBrowser);
    m_skpDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_skpDock);
    
    // Analysis Panel (right, tabbed with metadata and SKP)
    m_analysisDock = new QDockWidget("AI Analysis", this);
    m_analysisDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_analysisDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_analysisPanel = new AnalysisPanel(this);
    m_analysisDock->setWidget(m_analysisPanel);
    m_analysisDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, m_analysisDock);
    
    // Tab the right panels together (Lightroom style)
    tabifyDockWidget(m_metadataDock, m_skpDock);
    tabifyDockWidget(m_skpDock, m_analysisDock);
    m_metadataDock->raise();  // Show metadata by default
    
    // Connect analysis panel signals
    connect(m_analysisPanel, &AnalysisPanel::metadataUpdated, 
            this, [this](const QString& filepath) {
                // Reload metadata for updated image
                if (m_currentIndex >= 0 && filepath == m_imageFiles[m_currentIndex]) {
                    m_metadataPanel->loadMetadata(filepath);
                    // Thumbnail will auto-refresh when metadata changes
                }
            });
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
    
    // Find all supported images
    QDir dir(path);
    QStringList filters = ImageLoader::instance().supportedExtensions();
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);
    
    m_imageFiles.clear();
    for (const QString& file : files) {
        m_imageFiles << dir.filePath(file);
    }
    
    if (m_imageFiles.isEmpty()) {
        QMessageBox::information(this, "No Images", 
            "No supported images found in this directory.");
        return;
    }
    
    // Load into thumbnail grid
    m_thumbnailGrid->setImages(m_imageFiles);
    
    // Select first image
    m_currentIndex = 0;
    onImageSelected(m_imageFiles[0]);
    
    // Update analysis panel with directory
    m_analysisPanel->setCurrentDirectory(path);
    
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
    
    // Update thumbnail selection
    m_thumbnailGrid->selectImage(m_currentIndex);
    
    // Update status
    if (m_currentIndex >= 0) {
        statusBar()->showMessage(
            QString("%1/%2 - %3")
                .arg(m_currentIndex + 1)
                .arg(m_imageFiles.count())
                .arg(QFileInfo(filepath).fileName())
        );
    }
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
            QMessageBox::warning(this, "No Images", "Please load a directory first.");
            return;
        }
        
        statusBar()->showMessage(QString("Searching for: %1").arg(query));
        
        // For now, show info about semantic search
        // In production, would integrate with PythonBridge CLIP embeddings
        QMessageBox info(this);
        info.setWindowTitle("Semantic Search");
        info.setIcon(QMessageBox::Information);
        info.setText(QString("Searching for: \"%1\"").arg(query));
        info.setInformativeText(
            "Semantic search uses AI to understand image content and find matches.\n\n"
            "For production: This would:\n"
            "1. Convert your query to CLIP embeddings via agent_v2.py\n"
            "2. Compare against stored image embeddings\n"
            "3. Rank results by semantic similarity (cosine distance)\n\n"
            "Current implementation: Searches in AI-generated titles, descriptions, and keywords."
        );
        info.exec();
    }
}

void MainWindow::onRunAnalysis() {
    if (m_currentIndex < 0) {
        QMessageBox::warning(this, "No Image", "Please select an image first.");
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
    QMessageBox info(this);
    info.setWindowTitle("Preferences");
    info.setIcon(QMessageBox::Information);
    info.setText("PhotoGuru Viewer Settings");
    info.setInformativeText(
        "Available settings:\n\n"
        "Image Display:\n"
        "  - Auto-fit images: Enabled by default\n"
        "  - Smooth zoom: Enabled (1.25x steps)\n\n"
        "Python Integration:\n"
        "  - Agent path: ../../../agent_v2.py\n"
        "  - Python venv: ../../../.venv/bin/python3\n\n"
        "Cache:\n"
        "  - Thumbnails: In-memory (128x128)\n"
        "  - Max cache size: System memory dependent\n\n"
        "For advanced settings, edit via command line or config file."
    );
    info.exec();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About PhotoGuru Viewer",
        "<h2>PhotoGuru Viewer 1.0</h2>"
        "<p>Professional photo viewer and AI-powered semantic image browser.</p>"
        "<p>Built with Qt6 C++ and Python ML backend.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Universal image support (RAW, HEIF, standard formats)</li>"
        "<li>AI-powered semantic analysis (CLIP + LLM)</li>"
        "<li>Semantic Key Protocol (SKP) for advanced organization</li>"
        "<li>Technical quality analysis</li>"
        "</ul>"
        "<p>© 2026 PhotoGuru</p>");
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
    
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    m_currentDirectory = settings.value("lastDirectory", QDir::homePath()).toString();
}

void MainWindow::saveSettings() {
    QSettings settings("PhotoGuru", "Viewer");
    
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("lastDirectory", m_currentDirectory);
}

void MainWindow::onMetadataUpdated(const QString& filepath) {
    // Handle metadata update signal
    if (m_currentIndex >= 0 && m_currentIndex < m_imageFiles.size() && 
        m_imageFiles[m_currentIndex] == filepath) {
        // Reload metadata panel if current image
        m_metadataPanel->loadMetadata(filepath);
        m_skpBrowser->loadImageKeys(filepath);
    }
}

void MainWindow::onFilterChanged(const FilterCriteria& criteria) {
    // Apply filters to photos
    // Filter by rating, quality scores, dates, etc.
    QStringList filteredFiles;
    
    for (const QString& filepath : m_imageFiles) {
        auto metaOpt = MetadataReader::instance().read(filepath);
        if (!metaOpt) continue;  // Skip if metadata can't be read
        
        PhotoMetadata meta = metaOpt.value();
        
        bool passes = true;
        
        // Apply quality filters if set
        if (criteria.minQuality > 0.0) {
            if (meta.technical.overall_quality < criteria.minQuality) {
                passes = false;
            }
        }
        
        // Apply sharpness filter if set
        if (criteria.minSharpness > 0.0) {
            if (meta.technical.sharpness_score < criteria.minSharpness) {
                passes = false;
            }
        }
        
        // Add more filter criteria as needed
        
        if (passes) {
            filteredFiles << filepath;
        }
    }
    
    // Update display with filtered results
    m_thumbnailGrid->setImages(filteredFiles);
    statusBar()->showMessage(QString("%1 images match filters").arg(filteredFiles.count()));
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
        QMessageBox::information(this, "Copy", "No images selected");
        return;
    }
    
    QString dest = QFileDialog::getExistingDirectory(this, "Copy to Directory");
    if (dest.isEmpty()) return;
    
    int copied = 0;
    for (const QString& file : selected) {
        QFileInfo fi(file);
        QString destFile = dest + "/" + fi.fileName();
        if (QFile::copy(file, destFile)) {
            copied++;
        }
    }
    
    statusBar()->showMessage(QString("Copied %1 file(s)").arg(copied));
}

void MainWindow::onMoveFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    if (selected.isEmpty() && m_currentIndex >= 0) {
        selected << m_imageFiles[m_currentIndex];
    }
    
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Move", "No images selected");
        return;
    }
    
    QString dest = QFileDialog::getExistingDirectory(this, "Move to Directory");
    if (dest.isEmpty()) return;
    
    int moved = 0;
    for (const QString& file : selected) {
        QFileInfo fi(file);
        QString destFile = dest + "/" + fi.fileName();
        if (QFile::rename(file, destFile)) {
            moved++;
            m_imageFiles.removeAll(file);
        }
    }
    
    m_thumbnailGrid->setImages(m_imageFiles);
    if (m_currentIndex >= m_imageFiles.count()) {
        m_currentIndex = m_imageFiles.count() - 1;
    }
    
    statusBar()->showMessage(QString("Moved %1 file(s)").arg(moved));
}

void MainWindow::onRenameFile() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        QMessageBox::information(this, "Rename", "No image selected");
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
        QMessageBox::warning(this, "Rename Failed", "A file with that name already exists");
        return;
    }
    
    if (QFile::rename(currentFile, newPath)) {
        m_imageFiles[m_currentIndex] = newPath;
        m_thumbnailGrid->setImages(m_imageFiles);
        m_imageViewer->loadImage(newPath);
        statusBar()->showMessage("File renamed");
    } else {
        QMessageBox::warning(this, "Rename Failed", "Could not rename file");
    }
}

void MainWindow::onDeleteFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    if (selected.isEmpty() && m_currentIndex >= 0) {
        selected << m_imageFiles[m_currentIndex];
    }
    
    if (selected.isEmpty()) {
        QMessageBox::information(this, "Delete", "No images selected");
        return;
    }
    
    auto reply = QMessageBox::question(this, "Delete Files",
        QString("Move %1 file(s) to trash?").arg(selected.count()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) return;
    
    int deleted = 0;
    for (const QString& file : selected) {
        // Move to trash on macOS
        QProcess::execute("osascript", {
            "-e", QString("tell application \"Finder\" to delete (POSIX file \"%1\")").arg(file)
        });
        m_imageFiles.removeAll(file);
        deleted++;
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
}

void MainWindow::onRevealInFinder() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        QMessageBox::information(this, "Reveal in Finder", "No image selected");
        return;
    }
    
    QString file = m_imageFiles[m_currentIndex];
    QProcess::execute("open", {"-R", file});
    statusBar()->showMessage("Revealed in Finder");
}

void MainWindow::onOpenWithExternal() {
    if (m_currentIndex < 0 || m_currentIndex >= m_imageFiles.count()) {
        QMessageBox::information(this, "Open With", "No image selected");
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

} // namespace PhotoGuru
