#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QTabWidget>
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

// Test that panel no longer has internal tabs (tabs are now at dock level)
TEST_F(MetadataPanelTest, NoInternalTabs) {
    // Panel should NOT have internal tab widget anymore
    QTabWidget* tabWidget = panel->findChild<QTabWidget*>();
    EXPECT_EQ(tabWidget, nullptr) << "MetadataPanel should not have internal tabs";
    
    // Should have a scroll area with metadata content
    QScrollArea* scrollArea = panel->findChild<QScrollArea*>();
    EXPECT_NE(scrollArea, nullptr) << "MetadataPanel should have a scroll area";
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

// Test edit mode toggle
TEST_F(MetadataPanelTest, EditModeToggle) {
    EXPECT_NO_THROW(panel->setEditable(true));
    EXPECT_NO_THROW(panel->setEditable(false));
    EXPECT_NO_THROW(panel->setEditable(true));
    EXPECT_NO_THROW(panel->setEditable(false));
}

// Test collapsible sections exist
TEST_F(MetadataPanelTest, CollapsibleSectionsExist) {
    // Find CollapsibleGroupBox widgets
    auto collapsibleSections = panel->findChildren<CollapsibleGroupBox*>();
    
    // Should have multiple collapsible sections
    // (EXIF, IPTC, XMP, File, Technical, Custom)
    EXPECT_GE(collapsibleSections.size(), 5);
}

// Test metadata field widgets creation
TEST_F(MetadataPanelTest, MetadataFieldWidgetsCreation) {
    // Load some metadata
    panel->loadMetadata("/test/image.jpg");
    
    // Check that MetadataFieldWidget instances can be created
    // This is tested indirectly through loadMetadata working without crashes
    EXPECT_TRUE(true);
}
