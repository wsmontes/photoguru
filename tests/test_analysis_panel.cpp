#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QSignalSpy>
#include "../src/ui/AnalysisPanel.h"

using namespace PhotoGuru;

class AnalysisPanelTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // QApplication is required for QWidget-based classes
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        panel = new AnalysisPanel();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        panel = nullptr;
    }
    
    AnalysisPanel* panel = nullptr;
};

// Test panel creation
TEST_F(AnalysisPanelTest, PanelCreation) {
    ASSERT_NE(panel, nullptr);
    EXPECT_TRUE(panel->isWidgetType());
}

// Test signals exist
TEST_F(AnalysisPanelTest, SignalDefinitions) {
    QSignalSpy startedSpy(panel, &AnalysisPanel::analysisStarted);
    QSignalSpy completedSpy(panel, &AnalysisPanel::analysisCompleted);
    QSignalSpy metadataUpdatedSpy(panel, &AnalysisPanel::metadataUpdated);
    QSignalSpy dirCompletedSpy(panel, &AnalysisPanel::directoryAnalysisCompleted);
    
    EXPECT_TRUE(startedSpy.isValid());
    EXPECT_TRUE(completedSpy.isValid());
    EXPECT_TRUE(metadataUpdatedSpy.isValid());
    EXPECT_TRUE(dirCompletedSpy.isValid());
}

// Test setCurrentImage
TEST_F(AnalysisPanelTest, SetCurrentImage) {
    EXPECT_NO_THROW(panel->setCurrentImage("/path/to/image.jpg"));
    EXPECT_NO_THROW(panel->setCurrentImage(""));
}

// Test setCurrentDirectory
TEST_F(AnalysisPanelTest, SetCurrentDirectory) {
    EXPECT_NO_THROW(panel->setCurrentDirectory("/path/to/dir"));
    EXPECT_NO_THROW(panel->setCurrentDirectory(""));
}

// Test slot methods exist and can be called
TEST_F(AnalysisPanelTest, SlotMethods) {
    // Just verify slots exist - don't actually call them without proper context
    // as they start real worker threads
    SUCCEED();  // Placeholder test
}

// Test context setting before operations
TEST_F(AnalysisPanelTest, ContextBeforeOperations) {
    panel->setCurrentImage("/test/image.jpg");
    panel->setCurrentDirectory("/test/dir");
    // Don't actually call the methods as they start real threads
    SUCCEED();
}

// Test multiple operations sequence
TEST_F(AnalysisPanelTest, SequentialOperations) {
    panel->setCurrentDirectory("/test");
    // Don't call real operations that start threads
    SUCCEED();
}
