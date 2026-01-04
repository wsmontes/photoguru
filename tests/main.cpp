#include <gtest/gtest.h>
#include <QApplication>
#include <QThreadPool>
#include <QtGlobal>
#include "core/ThumbnailCache.h"

// Custom message handler to suppress expected warnings during tests
void testMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    // Suppress expected "File not found" warnings during tests
    if (msg.contains("Failed to load image") || 
        msg.contains("File not found") ||
        msg.contains("ExifTool error") ||
        msg.contains("No metadata output") ||
        msg.contains("MetadataReader::read called") ||
        msg.contains("Running \"/opt/homebrew/bin/exiftool\"")) {
        return;
    }
    
    // Forward other messages to default handler
    if (type == QtWarningMsg) {
        fprintf(stderr, "Warning: %s\n", qPrintable(msg));
    } else if (type == QtCriticalMsg) {
        fprintf(stderr, "Critical: %s\n", qPrintable(msg));
    } else if (type == QtFatalMsg) {
        fprintf(stderr, "Fatal: %s\n", qPrintable(msg));
        abort();
    }
}

// Test event listener to clean up Qt resources
class QtCleanupListener : public ::testing::EmptyTestEventListener {
    void OnTestEnd(const ::testing::TestInfo&) override {
        // Wait for all background tasks to complete
        QThreadPool::globalInstance()->waitForDone();
        // Process pending deleteLater() events after each test
        QApplication::processEvents();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};

int main(int argc, char **argv) {
    // Initialize Qt Application for tests (needed for QWidget/QPixmap/QImage)
    QApplication app(argc, argv);
    
    // Install custom message handler to suppress expected test warnings
    qInstallMessageHandler(testMessageHandler);
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add cleanup listener
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new QtCleanupListener);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    // Wait for all background tasks to complete
    QThreadPool::globalInstance()->waitForDone();
    
    // Clean up all pending Qt events and delete scheduled objects
    QApplication::processEvents();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    
    // Clean up caches before QApplication is destroyed to prevent crashes
    PhotoGuru::ThumbnailCache::instance().clear();
    
    return result;
}
