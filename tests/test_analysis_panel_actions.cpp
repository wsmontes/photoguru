#include <gtest/gtest.h>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QTest>
#include "../src/ui/AnalysisPanel.h"

using namespace PhotoGuru;

class AnalysisPanelActionsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        // Pass false to skip AI initialization (3GB models) - prevents system freeze
        panel = new AnalysisPanel(nullptr, false);
        // DO NOT call panel->show() - tests must be autonomous!
    }
    
    void TearDown() override {
        panel = nullptr;
    }
    
    QPushButton* findButton(const QString& textContains) {
        auto buttons = panel->findChildren<QPushButton*>();
        for (auto* btn : buttons) {
            if (btn->text().contains(textContains, Qt::CaseInsensitive)) {
                return btn;
            }
        }
        return nullptr;
    }
    
    QLabel* findLabel(const QString& objectName) {
        return panel->findChild<QLabel*>(objectName);
    }
    
    AnalysisPanel* panel = nullptr;
};

// ========== Test 1: setCurrentImage() with Valid Path ==========

TEST_F(AnalysisPanelActionsTest, SetCurrentImage_WithValidPath) {
    QString testPath = "/test/photos/image001.jpg";
    
    panel->setCurrentImage(testPath);
    
    // Verify button states updated
    QPushButton* analyzeBtn = findButton("Analyze");
    if (analyzeBtn) {
        EXPECT_TRUE(analyzeBtn->isEnabled()) 
            << "Analyze button should be enabled after setting image";
    }
}

TEST_F(AnalysisPanelActionsTest, SetCurrentImage_UpdatesLabel) {
    QLabel* label = findLabel("currentImageLabel");
    if (!label) {
        // Try finding by text content instead
        auto labels = panel->findChildren<QLabel*>();
        for (auto* l : labels) {
            if (l->text().contains("image", Qt::CaseInsensitive) || 
                l->text().contains("selected", Qt::CaseInsensitive)) {
                label = l;
                break;
            }
        }
    }
    
    if (label) {
        QString testPath = "/test/photos/vacation.jpg";
        panel->setCurrentImage(testPath);
        
        // Label should show filename or path
        QString labelText = label->text();
        EXPECT_TRUE(labelText.contains("vacation.jpg") || 
                    labelText.contains(testPath))
            << "Label should display current image name/path";
    }
}

TEST_F(AnalysisPanelActionsTest, SetCurrentImage_MultipleTimesChangesState) {
    QPushButton* analyzeBtn = findButton("Analyze");
    if (!analyzeBtn) {
        GTEST_SKIP() << "Analyze button not found";
    }
    
    // Set first image
    panel->setCurrentImage("/test/image1.jpg");
    EXPECT_TRUE(analyzeBtn->isEnabled());
    
    // Change to second image
    panel->setCurrentImage("/test/image2.jpg");
    EXPECT_TRUE(analyzeBtn->isEnabled()) 
        << "Should remain enabled with different image";
}

// ========== Test 2: setCurrentImage() with Empty Path ==========

TEST_F(AnalysisPanelActionsTest, SetCurrentImage_WithEmptyPath) {
    // First set a valid image
    panel->setCurrentImage("/test/image.jpg");
    
    // Then clear it
    panel->setCurrentImage("");
    
    QPushButton* analyzeBtn = findButton("Analyze");
    if (analyzeBtn) {
        EXPECT_FALSE(analyzeBtn->isEnabled()) 
            << "Analyze button should be disabled when image path is empty";
    }
}

TEST_F(AnalysisPanelActionsTest, SetCurrentImage_EmptyPathResetsLabel) {
    QLabel* label = findLabel("currentImageLabel");
    if (!label) {
        auto labels = panel->findChildren<QLabel*>();
        for (auto* l : labels) {
            if (l->text().contains("No", Qt::CaseInsensitive) || 
                l->text().contains("image", Qt::CaseInsensitive)) {
                label = l;
                break;
            }
        }
    }
    
    if (label) {
        // Set then clear
        panel->setCurrentImage("/test/image.jpg");
        panel->setCurrentImage("");
        
        QString labelText = label->text();
        EXPECT_TRUE(labelText.contains("No", Qt::CaseInsensitive) || 
                    labelText.isEmpty())
            << "Label should show 'No image' or be empty when cleared";
    }
}

// ========== Test 3: setCurrentDirectory() with Valid Path ==========

TEST_F(AnalysisPanelActionsTest, SetCurrentDirectory_WithValidPath) {
    QString testDir = "/test/photos/";
    
    panel->setCurrentDirectory(testDir);
    
    // Verify batch buttons enabled
    QPushButton* batchBtn = findButton("Analyze All");
    if (!batchBtn) batchBtn = findButton("Directory");
    
    if (batchBtn) {
        EXPECT_TRUE(batchBtn->isEnabled()) 
            << "Batch analyze button should be enabled after setting directory";
    }
    
    QPushButton* duplicatesBtn = findButton("Duplicate");
    if (duplicatesBtn) {
        EXPECT_TRUE(duplicatesBtn->isEnabled());
    }
    
    QPushButton* burstsBtn = findButton("Burst");
    if (burstsBtn) {
        EXPECT_TRUE(burstsBtn->isEnabled());
    }
    
    QPushButton* reportBtn = findButton("Report");
    if (reportBtn) {
        EXPECT_TRUE(reportBtn->isEnabled());
    }
}

TEST_F(AnalysisPanelActionsTest, SetCurrentDirectory_MultipleDirectories) {
    QPushButton* batchBtn = findButton("Analyze All");
    if (!batchBtn) batchBtn = findButton("Directory");
    
    if (!batchBtn) {
        GTEST_SKIP() << "Batch button not found";
    }
    
    // Set first directory
    panel->setCurrentDirectory("/test/dir1/");
    EXPECT_TRUE(batchBtn->isEnabled());
    
    // Change to different directory
    panel->setCurrentDirectory("/test/dir2/");
    EXPECT_TRUE(batchBtn->isEnabled()) 
        << "Should remain enabled with different directory";
}

// ========== Test 4: setCurrentDirectory() with Empty Path ==========

TEST_F(AnalysisPanelActionsTest, SetCurrentDirectory_WithEmptyPath) {
    // First set a valid directory
    panel->setCurrentDirectory("/test/photos/");
    
    // Then clear it
    panel->setCurrentDirectory("");
    
    // Verify batch buttons disabled
    QPushButton* batchBtn = findButton("Analyze All");
    if (!batchBtn) batchBtn = findButton("Directory");
    
    if (batchBtn) {
        EXPECT_FALSE(batchBtn->isEnabled()) 
            << "Batch button should be disabled when directory is empty";
    }
    
    QPushButton* duplicatesBtn = findButton("Duplicate");
    if (duplicatesBtn) {
        EXPECT_FALSE(duplicatesBtn->isEnabled());
    }
}

// ========== Test 5: Setting Both Image and Directory ==========

TEST_F(AnalysisPanelActionsTest, SetBothImageAndDirectory) {
    panel->setCurrentImage("/test/photos/image.jpg");
    panel->setCurrentDirectory("/test/photos/");
    
    // Both single and batch operations should be enabled
    QPushButton* analyzeBtn = findButton("Analyze");
    QPushButton* batchBtn = findButton("Analyze All");
    if (!batchBtn) batchBtn = findButton("Directory");
    
    if (analyzeBtn) {
        EXPECT_TRUE(analyzeBtn->isEnabled()) 
            << "Single analyze should work when both are set";
    }
    
    if (batchBtn) {
        EXPECT_TRUE(batchBtn->isEnabled()) 
            << "Batch analyze should work when both are set";
    }
}

TEST_F(AnalysisPanelActionsTest, IndependentImageAndDirectory) {
    // Set directory only
    panel->setCurrentDirectory("/test/photos/");
    
    QPushButton* analyzeBtn = findButton("Analyze");
    if (analyzeBtn) {
        EXPECT_FALSE(analyzeBtn->isEnabled()) 
            << "Single analyze needs image even if directory is set";
    }
    
    // Now set image
    panel->setCurrentImage("/test/other/image.jpg");
    
    if (analyzeBtn) {
        EXPECT_TRUE(analyzeBtn->isEnabled()) 
            << "Single analyze should work after setting image";
    }
}

// ========== Test 6: Signal Emissions ==========

TEST_F(AnalysisPanelActionsTest, AnalysisStartedSignal_NotEmittedWithoutAction) {
    QSignalSpy spy(panel, &AnalysisPanel::analysisStarted);
    
    // Just setting image/directory shouldn't emit signals
    panel->setCurrentImage("/test/image.jpg");
    panel->setCurrentDirectory("/test/photos/");
    
    EXPECT_EQ(spy.count(), 0) 
        << "analysisStarted should not emit without user action";
}

TEST_F(AnalysisPanelActionsTest, MetadataUpdatedSignal_Defined) {
    QSignalSpy spy(panel, &AnalysisPanel::metadataUpdated);
    
    EXPECT_TRUE(spy.isValid()) 
        << "metadataUpdated signal should be properly defined";
}

TEST_F(AnalysisPanelActionsTest, DirectoryAnalysisCompletedSignal_Defined) {
    QSignalSpy spy(panel, &AnalysisPanel::directoryAnalysisCompleted);
    
    EXPECT_TRUE(spy.isValid()) 
        << "directoryAnalysisCompleted signal should be properly defined";
}

// ========== Test 7: Initial State ==========

TEST_F(AnalysisPanelActionsTest, InitialState_NoImageSelected) {
    // On fresh panel, no image should be selected
    QPushButton* analyzeBtn = findButton("Analyze");
    if (analyzeBtn) {
        EXPECT_FALSE(analyzeBtn->isEnabled()) 
            << "Initially, analyze button should be disabled";
    }
}

TEST_F(AnalysisPanelActionsTest, InitialState_NoDirectorySelected) {
    // On fresh panel, no directory should be selected
    QPushButton* batchBtn = findButton("Analyze All");
    if (!batchBtn) batchBtn = findButton("Directory");
    
    if (batchBtn) {
        EXPECT_FALSE(batchBtn->isEnabled()) 
            << "Initially, batch button should be disabled";
    }
}

TEST_F(AnalysisPanelActionsTest, InitialState_CancelDisabled) {
    QPushButton* cancelBtn = findButton("Cancel");
    if (cancelBtn) {
        EXPECT_FALSE(cancelBtn->isEnabled()) 
            << "Initially, cancel button should be disabled";
    }
}

// ========== Test 8: State Transitions ==========

TEST_F(AnalysisPanelActionsTest, StateTransition_ImageSelectedToCleared) {
    panel->setCurrentImage("/test/image.jpg");
    
    QPushButton* analyzeBtn = findButton("Analyze");
    if (analyzeBtn) {
        EXPECT_TRUE(analyzeBtn->isEnabled());
        
        // Clear image
        panel->setCurrentImage("");
        
        EXPECT_FALSE(analyzeBtn->isEnabled()) 
            << "Button should disable when image is cleared";
    }
}

TEST_F(AnalysisPanelActionsTest, StateTransition_DirectorySelectedToCleared) {
    panel->setCurrentDirectory("/test/photos/");
    
    QPushButton* batchBtn = findButton("Analyze All");
    if (!batchBtn) batchBtn = findButton("Directory");
    
    if (batchBtn) {
        EXPECT_TRUE(batchBtn->isEnabled());
        
        // Clear directory
        panel->setCurrentDirectory("");
        
        EXPECT_FALSE(batchBtn->isEnabled()) 
            << "Batch button should disable when directory is cleared";
    }
}

// ========== Test 9: Widget Visibility ==========

TEST_F(AnalysisPanelActionsTest, PanelComponents_Visible) {
    // Panel itself should be valid
    EXPECT_TRUE(panel->isWidgetType());
    
    // Key components should exist (don't need to be shown on screen)
    auto buttons = panel->findChildren<QPushButton*>();
    EXPECT_GT(buttons.size(), 0) << "Panel should have buttons";
    
    auto labels = panel->findChildren<QLabel*>();
    EXPECT_GT(labels.size(), 0) << "Panel should have labels";
}

TEST_F(AnalysisPanelActionsTest, AllWidgets_Accessible) {
    // Verify we can access widgets without showing GUI
    // This ensures the panel is properly constructed
    
    auto buttons = panel->findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        EXPECT_NE(btn, nullptr) << "All buttons should be accessible";
        EXPECT_FALSE(btn->text().isEmpty()) << "Buttons should have text";
    }
}
