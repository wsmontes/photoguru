#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include "../src/ui/MetadataPanel.h"

using namespace PhotoGuru;

class MetadataPanelTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        panel = new MetadataPanel();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        panel = nullptr;
    }
    
    MetadataPanel* panel = nullptr;
};

// Test panel creation
TEST_F(MetadataPanelTest, PanelCreation) {
    ASSERT_NE(panel, nullptr);
    EXPECT_TRUE(panel->isWidgetType());
}

// Test loadMetadata
TEST_F(MetadataPanelTest, LoadMetadata) {
    EXPECT_NO_THROW(panel->loadMetadata("/test/image.jpg"));
}

// Test loadMetadata with various paths
TEST_F(MetadataPanelTest, LoadMetadataVariousPaths) {
    EXPECT_NO_THROW(panel->loadMetadata("/absolute/path/image.jpg"));
    EXPECT_NO_THROW(panel->loadMetadata("relative/image.jpg"));
    EXPECT_NO_THROW(panel->loadMetadata(""));
}

// Test clear
TEST_F(MetadataPanelTest, ClearPanel) {
    panel->loadMetadata("/test/image.jpg");
    EXPECT_NO_THROW(panel->clear());
}

// Test load and clear sequence
TEST_F(MetadataPanelTest, LoadClearSequence) {
    panel->loadMetadata("/test/image1.jpg");
    panel->clear();
    panel->loadMetadata("/test/image2.jpg");
    panel->clear();
}

// Test multiple loads without clearing
TEST_F(MetadataPanelTest, MultipleLoads) {
    EXPECT_NO_THROW(panel->loadMetadata("/test/image1.jpg"));
    EXPECT_NO_THROW(panel->loadMetadata("/test/image2.jpg"));
    EXPECT_NO_THROW(panel->loadMetadata("/test/image3.jpg"));
}

// Test loading same file multiple times
TEST_F(MetadataPanelTest, ReloadSameFile) {
    const QString filepath = "/test/same_image.jpg";
    EXPECT_NO_THROW(panel->loadMetadata(filepath));
    EXPECT_NO_THROW(panel->loadMetadata(filepath));
    EXPECT_NO_THROW(panel->loadMetadata(filepath));
}

// Test clear when already empty
TEST_F(MetadataPanelTest, ClearWhenEmpty) {
    EXPECT_NO_THROW(panel->clear());
    EXPECT_NO_THROW(panel->clear());
}
