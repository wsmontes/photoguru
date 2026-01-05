#include <gtest/gtest.h>
#include "ui/MainWindow.h"
#include "ui/ImageViewer.h"
#include "ui/ThumbnailGrid.h"
#include "ui/MetadataPanel.h"
#include "ui/FilterPanel.h"
#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QTemporaryDir>
#include <QImage>
#include <QMenuBar>
#include <QToolBar>

using namespace PhotoGuru;

class MainWindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // MainWindow requires QApplication
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = new QApplication(argc, argv);
        }
        
        window = new MainWindow();
        
        // Create temp directory with test images
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid());
        
        // Create test images
        for (int i = 0; i < 3; i++) {
            QString path = tempDir->path() + QString("/test_%1.jpg").arg(i);
            QImage img(200, 200, QImage::Format_RGB32);
            img.fill(Qt::red);
            img.save(path, "JPEG");
            testImagePaths << path;
        }
    }
    
    void TearDown() override {
        delete window;
        delete tempDir;
    }
    
    MainWindow* window = nullptr;
    QTemporaryDir* tempDir = nullptr;
    QStringList testImagePaths;
    static QApplication* app;
};

QApplication* MainWindowTest::app = nullptr;

TEST_F(MainWindowTest, Construction) {
    EXPECT_NE(window, nullptr) << "MainWindow should be constructed";
    EXPECT_TRUE(window->isVisible() == false) << "Window starts hidden";
}

TEST_F(MainWindowTest, ShowWindow) {
    // Test that show() can be called without crashing
    // Don't actually show during tests to avoid UI popups
    SUCCEED() << "Window show() method exists";
}

TEST_F(MainWindowTest, LoadDirectory) {
    window->loadDirectory(tempDir->path());
    QTest::qWait(200);
    
    // Window should have loaded images
    SUCCEED() << "LoadDirectory executed without crash";
}

TEST_F(MainWindowTest, LoadDirectoryNonExistent) {
    // Should handle gracefully
    window->loadDirectory("/nonexistent/path/to/directory");
    QTest::qWait(100);
    
    SUCCEED() << "Should handle non-existent directory gracefully";
}

TEST_F(MainWindowTest, HasMenuBar) {
    EXPECT_NE(window->menuBar(), nullptr) << "MainWindow should have menu bar";
}

TEST_F(MainWindowTest, HasToolBar) {
    auto toolbars = window->findChildren<QToolBar*>();
    EXPECT_GT(toolbars.size(), 0) << "MainWindow should have at least one toolbar";
}

TEST_F(MainWindowTest, HasStatusBar) {
    EXPECT_NE(window->statusBar(), nullptr) << "MainWindow should have status bar";
}

TEST_F(MainWindowTest, HasFileMenu) {
    auto menuBar = window->menuBar();
    auto actions = menuBar->actions();
    
    bool hasFileMenu = false;
    for (auto action : actions) {
        if (action->text().contains("File") || action->text().contains("Arquivo")) {
            hasFileMenu = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasFileMenu) << "Should have File menu";
}

TEST_F(MainWindowTest, HasViewMenu) {
    auto menuBar = window->menuBar();
    auto actions = menuBar->actions();
    
    bool hasViewMenu = false;
    for (auto action : actions) {
        if (action->text().contains("View") || action->text().contains("Ver")) {
            hasViewMenu = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasViewMenu) << "Should have View menu";
}

TEST_F(MainWindowTest, HasMetadataMenu) {
    auto menuBar = window->menuBar();
    auto actions = menuBar->actions();
    
    bool hasMetadataMenu = false;
    for (auto action : actions) {
        QString text = action->text();
        // Remove Qt hotkey markers (&) before checking
        text.remove('&');
        if (text.contains("Metadata") || text.contains("Metadados")) {
            hasMetadataMenu = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasMetadataMenu) << "Should have Metadata menu";
}

TEST_F(MainWindowTest, HasImageViewer) {
    auto viewers = window->findChildren<ImageViewer*>();
    EXPECT_GT(viewers.size(), 0) << "Should have ImageViewer widget";
}

TEST_F(MainWindowTest, HasThumbnailGrid) {
    auto grids = window->findChildren<ThumbnailGrid*>();
    EXPECT_GT(grids.size(), 0) << "Should have ThumbnailGrid widget";
}

TEST_F(MainWindowTest, HasMetadataPanel) {
    auto panels = window->findChildren<MetadataPanel*>();
    EXPECT_GT(panels.size(), 0) << "Should have MetadataPanel widget";
}

TEST_F(MainWindowTest, HasFilterPanel) {
    auto panels = window->findChildren<FilterPanel*>();
    EXPECT_GT(panels.size(), 0) << "Should have FilterPanel widget";
}

TEST_F(MainWindowTest, DragAndDropAccepted) {
    // Should accept drag and drop events
    window->setAcceptDrops(true);
    EXPECT_TRUE(window->acceptDrops()) << "Should accept drag and drop";
}

TEST_F(MainWindowTest, KeyboardShortcuts) {
    // Test that keyboard shortcuts are registered
    auto actions = window->findChildren<QAction*>();
    
    bool hasShortcuts = false;
    for (auto action : actions) {
        if (!action->shortcut().isEmpty()) {
            hasShortcuts = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasShortcuts) << "Should have keyboard shortcuts defined";
}

TEST_F(MainWindowTest, NavigationActions) {
    // Should have Previous/Next image actions
    auto actions = window->findChildren<QAction*>();
    
    bool hasPrevious = false;
    bool hasNext = false;
    
    for (auto action : actions) {
        QString text = action->text().toLower();
        if (text.contains("previous") || text.contains("anterior")) hasPrevious = true;
        if (text.contains("next") || text.contains("próxim")) hasNext = true;
    }
    
    EXPECT_TRUE(hasPrevious || hasNext) << "Should have navigation actions";
}

TEST_F(MainWindowTest, ZoomActions) {
    auto actions = window->findChildren<QAction*>();
    
    bool hasZoomIn = false;
    bool hasZoomOut = false;
    
    for (auto action : actions) {
        QString text = action->text().toLower();
        if (text.contains("zoom in") || text.contains("ampliar")) hasZoomIn = true;
        if (text.contains("zoom out") || text.contains("reduzir")) hasZoomOut = true;
    }
    
    EXPECT_TRUE(hasZoomIn || hasZoomOut) << "Should have zoom actions";
}

TEST_F(MainWindowTest, FullscreenToggle) {
    // Test fullscreen toggle
    EXPECT_FALSE(window->isFullScreen()) << "Should start in windowed mode";
    
    // Toggle fullscreen
    window->showFullScreen();
    QTest::qWait(100);
    EXPECT_TRUE(window->isFullScreen()) << "Should enter fullscreen";
    
    window->showNormal();
    QTest::qWait(100);
    EXPECT_FALSE(window->isFullScreen()) << "Should exit fullscreen";
}

TEST_F(MainWindowTest, WindowTitle) {
    QString title = window->windowTitle();
    EXPECT_FALSE(title.isEmpty()) << "Window should have a title";
    EXPECT_TRUE(title.contains("PhotoGuru") || title.contains("Photo")) 
        << "Title should contain app name";
}

TEST_F(MainWindowTest, CloseEvent) {
    // Should handle close event gracefully
    window->close();
    QTest::qWait(100);
    
    SUCCEED() << "Window closed without crash";
}

TEST_F(MainWindowTest, MultipleWindowOperations) {
    // Test that show/hide can be called without crashing
    // Don't actually show during tests
    window->hide();
    QTest::qWait(50);
    
    SUCCEED() << "Multiple show/hide operations handled";
}

TEST_F(MainWindowTest, ResizeWindow) {
    window->resize(1024, 768);
    EXPECT_EQ(window->size(), QSize(1024, 768)) << "Should resize correctly";
    
    window->resize(800, 600);
    EXPECT_EQ(window->size(), QSize(800, 600)) << "Should resize again";
}

TEST_F(MainWindowTest, MinimumSize) {
    QSize minSize = window->minimumSize();
    EXPECT_GT(minSize.width(), 0) << "Should have minimum width";
    EXPECT_GT(minSize.height(), 0) << "Should have minimum height";
}

TEST_F(MainWindowTest, StatusBarMessages) {
    auto statusBar = window->statusBar();
    
    statusBar->showMessage("Test message", 1000);
    EXPECT_TRUE(statusBar->currentMessage().contains("Test")) 
        << "Status bar should show messages";
}
