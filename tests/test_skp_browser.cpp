#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QSignalSpy>
#include "../src/ui/SKPBrowser.h"

using namespace PhotoGuru;

class SKPBrowserTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        browser = new SKPBrowser();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        browser = nullptr;
    }
    
    SKPBrowser* browser = nullptr;
};

// Test browser creation
TEST_F(SKPBrowserTest, BrowserCreation) {
    ASSERT_NE(browser, nullptr);
    EXPECT_TRUE(browser->isWidgetType());
}

// Test signals
TEST_F(SKPBrowserTest, SignalDefinitions) {
    QSignalSpy searchSpy(browser, &SKPBrowser::searchByKey);
    EXPECT_TRUE(searchSpy.isValid());
}

// Test loadImageKeys
TEST_F(SKPBrowserTest, LoadImageKeys) {
    EXPECT_NO_THROW(browser->loadImageKeys("/test/image.jpg"));
    EXPECT_NO_THROW(browser->loadImageKeys(""));
}

// Test clear
TEST_F(SKPBrowserTest, ClearBrowser) {
    browser->loadImageKeys("/test/image.jpg");
    EXPECT_NO_THROW(browser->clear());
}

// Test multiple loads
TEST_F(SKPBrowserTest, MultipleLoads) {
    EXPECT_NO_THROW(browser->loadImageKeys("/test/image1.jpg"));
    EXPECT_NO_THROW(browser->loadImageKeys("/test/image2.jpg"));
    EXPECT_NO_THROW(browser->clear());
    EXPECT_NO_THROW(browser->loadImageKeys("/test/image3.jpg"));
}

// Test load then clear sequence
TEST_F(SKPBrowserTest, LoadClearSequence) {
    for (int i = 0; i < 3; i++) {
        browser->loadImageKeys(QString("/test/image%1.jpg").arg(i));
        browser->clear();
    }
}

// Test with various file paths
TEST_F(SKPBrowserTest, VariousFilePaths) {
    EXPECT_NO_THROW(browser->loadImageKeys("/absolute/path/image.jpg"));
    EXPECT_NO_THROW(browser->loadImageKeys("relative/path/image.jpg"));
    EXPECT_NO_THROW(browser->loadImageKeys("./local/image.jpg"));
    EXPECT_NO_THROW(browser->loadImageKeys("../parent/image.jpg"));
}
