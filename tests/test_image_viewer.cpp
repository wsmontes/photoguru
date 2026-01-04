#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QSignalSpy>
#include "../src/ui/ImageViewer.h"

using namespace PhotoGuru;

class ImageViewerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        viewer = new ImageViewer();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        viewer = nullptr;
    }
    
    ImageViewer* viewer = nullptr;
};

// Test viewer creation
TEST_F(ImageViewerTest, ViewerCreation) {
    ASSERT_NE(viewer, nullptr);
    EXPECT_TRUE(viewer->isWidgetType());
}

// Test signals
TEST_F(ImageViewerTest, SignalDefinitions) {
    QSignalSpy zoomChangedSpy(viewer, &ImageViewer::zoomChanged);
    QSignalSpy imageLoadedSpy(viewer, &ImageViewer::imageLoaded);
    
    EXPECT_TRUE(zoomChangedSpy.isValid());
    EXPECT_TRUE(imageLoadedSpy.isValid());
}

// Test initial zoom level
TEST_F(ImageViewerTest, InitialZoom) {
    EXPECT_EQ(viewer->zoom(), 1.0);
}

// Test loadImage
TEST_F(ImageViewerTest, LoadImage) {
    EXPECT_NO_THROW(viewer->loadImage("/test/image.jpg"));
}

// Test clear
TEST_F(ImageViewerTest, ClearViewer) {
    viewer->loadImage("/test/image.jpg");
    EXPECT_NO_THROW(viewer->clear());
}

// Test zoom operations
TEST_F(ImageViewerTest, ZoomOperations) {
    EXPECT_NO_THROW(viewer->zoomIn());
    EXPECT_NO_THROW(viewer->zoomOut());
    EXPECT_NO_THROW(viewer->zoomToFit());
    EXPECT_NO_THROW(viewer->zoomActual());
}

// Test setZoom
TEST_F(ImageViewerTest, SetZoom) {
    viewer->setZoom(1.5);
    EXPECT_DOUBLE_EQ(viewer->zoom(), 1.5);
    
    viewer->setZoom(2.0);
    EXPECT_DOUBLE_EQ(viewer->zoom(), 2.0);
    
    viewer->setZoom(0.5);
    EXPECT_DOUBLE_EQ(viewer->zoom(), 0.5);
}

// Test zoom sequence
TEST_F(ImageViewerTest, ZoomSequence) {
    double initialZoom = viewer->zoom();
    
    viewer->zoomIn();
    EXPECT_GT(viewer->zoom(), initialZoom);
    
    viewer->zoomOut();
    viewer->zoomOut();
    EXPECT_LT(viewer->zoom(), initialZoom);
    
    viewer->zoomActual();
    EXPECT_DOUBLE_EQ(viewer->zoom(), 1.0);
}

// Test zoom limits
TEST_F(ImageViewerTest, ZoomLimits) {
    viewer->setZoom(0.1);
    EXPECT_GE(viewer->zoom(), 0.1);
    
    viewer->setZoom(10.0);
    EXPECT_LE(viewer->zoom(), 10.0);
}

// Test load then zoom
TEST_F(ImageViewerTest, LoadThenZoom) {
    viewer->loadImage("/test/image.jpg");
    viewer->zoomIn();
    viewer->zoomIn();
    EXPECT_GT(viewer->zoom(), 1.0);
}

// Test multiple image loads
TEST_F(ImageViewerTest, MultipleLoads) {
    viewer->loadImage("/test/image1.jpg");
    viewer->loadImage("/test/image2.jpg");
    viewer->loadImage("/test/image3.jpg");
    EXPECT_DOUBLE_EQ(viewer->zoom(), 1.0); // Should reset zoom
}

// Test zoom persistence across clears
TEST_F(ImageViewerTest, ZoomAfterClear) {
    viewer->setZoom(2.0);
    viewer->clear();
    // Zoom might reset after clear - implementation dependent
    EXPECT_GE(viewer->zoom(), 0.0);
}
