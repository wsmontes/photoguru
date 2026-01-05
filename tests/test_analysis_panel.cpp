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
        // Pass false to skip AI initialization (3GB models) - prevents system freeze
        panel = new AnalysisPanel(nullptr, false);
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

// Test public slots are callable (without starting real operations)
TEST_F(AnalysisPanelTest, PublicSlots_Callable) {
    // Verify the public slots exist and can be invoked via Qt meta-object system
    // We don't actually want to start real AI operations in unit tests
    
    // Use metaObject to check slot signatures exist
    const QMetaObject* metaObj = panel->metaObject();
    
    // Check for key slots
    int analyzeImageIdx = metaObj->indexOfSlot("onAnalyzeCurrentImage()");
    EXPECT_GE(analyzeImageIdx, 0) << "onAnalyzeCurrentImage slot should exist";
    
    int analyzeDirIdx = metaObj->indexOfSlot("onAnalyzeDirectory()");
    EXPECT_GE(analyzeDirIdx, 0) << "onAnalyzeDirectory slot should exist";
    
    int duplicatesIdx = metaObj->indexOfSlot("onFindDuplicates()");
    EXPECT_GE(duplicatesIdx, 0) << "onFindDuplicates slot should exist";
    
    int burstsIdx = metaObj->indexOfSlot("onDetectBursts()");
    EXPECT_GE(burstsIdx, 0) << "onDetectBursts slot should exist";
    
    int reportIdx = metaObj->indexOfSlot("onGenerateReport()");
    EXPECT_GE(reportIdx, 0) << "onGenerateReport slot should exist";
    
    int cancelIdx = metaObj->indexOfSlot("onCancelAnalysis()");
    EXPECT_GE(cancelIdx, 0) << "onCancelAnalysis slot should exist";
}

// Test private slot signatures (signals from workers)
TEST_F(AnalysisPanelTest, PrivateSlots_Exist) {
    const QMetaObject* metaObj = panel->metaObject();
    
    int progressIdx = metaObj->indexOfSlot("onAnalysisProgress(int,int,QString)");
    EXPECT_GE(progressIdx, 0) << "onAnalysisProgress slot should exist";
    
    int logIdx = metaObj->indexOfSlot("onAnalysisLog(QString)");
    EXPECT_GE(logIdx, 0) << "onAnalysisLog slot should exist";
    
    int errorIdx = metaObj->indexOfSlot("onAnalysisError(QString)");
    EXPECT_GE(errorIdx, 0) << "onAnalysisError slot should exist";
}

// Test signal-slot connections can be made
TEST_F(AnalysisPanelTest, SignalSlotConnections) {
    // Test that we can connect to panel's signals
    bool connected = false;
    
    connected = QObject::connect(panel, &AnalysisPanel::analysisStarted,
                                [&](){ /* callback */ });
    EXPECT_TRUE(connected) << "Should be able to connect to analysisStarted";
    
    connected = QObject::connect(panel, &AnalysisPanel::analysisCompleted,
                                [&](){ /* callback */ });
    EXPECT_TRUE(connected) << "Should be able to connect to analysisCompleted";
    
    connected = QObject::connect(panel, &AnalysisPanel::metadataUpdated,
                                [&](const QString&){ /* callback */ });
    EXPECT_TRUE(connected) << "Should be able to connect to metadataUpdated";
}

// Test panel has expected child widgets
TEST_F(AnalysisPanelTest, HasExpectedChildren) {
    auto buttons = panel->findChildren<QPushButton*>();
    EXPECT_GT(buttons.size(), 0) << "Panel should have buttons";
    
    auto labels = panel->findChildren<QLabel*>();
    EXPECT_GT(labels.size(), 0) << "Panel should have labels";
    
    auto checkboxes = panel->findChildren<QCheckBox*>();
    EXPECT_GE(checkboxes.size(), 2) << "Panel should have at least 2 checkboxes";
    
    auto progressBars = panel->findChildren<QProgressBar*>();
    EXPECT_GT(progressBars.size(), 0) << "Panel should have a progress bar";
    
    auto textEdits = panel->findChildren<QTextEdit*>();
    EXPECT_GT(textEdits.size(), 0) << "Panel should have text edit widgets";
}
