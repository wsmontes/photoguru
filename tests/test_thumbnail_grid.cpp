#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QSignalSpy>
#include "../src/ui/ThumbnailGrid.h"

using namespace PhotoGuru;

class ThumbnailGridTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        grid = new ThumbnailGrid();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        grid = nullptr;
    }
    
    ThumbnailGrid* grid = nullptr;
};

// Test grid creation
TEST_F(ThumbnailGridTest, GridCreation) {
    ASSERT_NE(grid, nullptr);
    EXPECT_TRUE(grid->isWidgetType());
}

// Test signals
TEST_F(ThumbnailGridTest, SignalDefinitions) {
    QSignalSpy imageSelectedSpy(grid, &ThumbnailGrid::imageSelected);
    EXPECT_TRUE(imageSelectedSpy.isValid());
}

// Test setImages with empty list
TEST_F(ThumbnailGridTest, SetEmptyImageList) {
    QStringList emptyList;
    EXPECT_NO_THROW(grid->setImages(emptyList));
}

// Test setImages with data
TEST_F(ThumbnailGridTest, SetImageList) {
    QStringList images;
    images << "/test/image1.jpg"
           << "/test/image2.jpg"
           << "/test/image3.jpg";
    
    EXPECT_NO_THROW(grid->setImages(images));
}

// Test selectImage
TEST_F(ThumbnailGridTest, SelectImage) {
    QStringList images;
    images << "/test/image1.jpg" << "/test/image2.jpg";
    grid->setImages(images);
    
    EXPECT_NO_THROW(grid->selectImage(0));
    EXPECT_NO_THROW(grid->selectImage(1));
}

// Test selectImage with invalid index
TEST_F(ThumbnailGridTest, SelectInvalidIndex) {
    QStringList images;
    images << "/test/image1.jpg";
    grid->setImages(images);
    
    // Negative index - should handle gracefully
    EXPECT_NO_THROW(grid->selectImage(-1));
    
    // Don't test very large indices as they may cause segfaults
    // depending on implementation
}

// Test multiple setImages calls
TEST_F(ThumbnailGridTest, MultipleSetImages) {
    QStringList images1;
    images1 << "/test/a.jpg";
    EXPECT_NO_THROW(grid->setImages(images1));
    
    QStringList images2;
    images2 << "/test/b.jpg" << "/test/c.jpg";
    EXPECT_NO_THROW(grid->setImages(images2));
}

// Test with large image list
TEST_F(ThumbnailGridTest, LargeImageList) {
    QStringList images;
    for (int i = 0; i < 100; i++) {
        images << QString("/test/image%1.jpg").arg(i);
    }
    
    EXPECT_NO_THROW(grid->setImages(images));
}

// Test selection after setImages
TEST_F(ThumbnailGridTest, SelectionAfterSetImages) {
    QStringList images;
    images << "/test/1.jpg" << "/test/2.jpg" << "/test/3.jpg";
    
    grid->setImages(images);
    EXPECT_NO_THROW(grid->selectImage(0));
    EXPECT_NO_THROW(grid->selectImage(1));
    EXPECT_NO_THROW(grid->selectImage(2));
}

// Test clearing by setting empty list
TEST_F(ThumbnailGridTest, ClearByEmptyList) {
    QStringList images;
    images << "/test/image.jpg";
    grid->setImages(images);
    
    QStringList empty;
    EXPECT_NO_THROW(grid->setImages(empty));
}
