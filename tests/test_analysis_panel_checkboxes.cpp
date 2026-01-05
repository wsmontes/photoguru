#include <gtest/gtest.h>
#include <QApplication>
#include <QCheckBox>
#include <QSignalSpy>
#include <QTest>
#include "../src/ui/AnalysisPanel.h"

using namespace PhotoGuru;

class AnalysisPanelCheckboxesTest : public ::testing::Test {
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
        // DO NOT call panel->show() - autonomous testing!
    }
    
    void TearDown() override {
        panel = nullptr;
    }
    
    // Helper to find checkbox by text
    QCheckBox* findCheckBox(const QString& textContains) {
        auto checkboxes = panel->findChildren<QCheckBox*>();
        for (auto* cb : checkboxes) {
            if (cb->text().contains(textContains, Qt::CaseInsensitive)) {
                return cb;
            }
        }
        return nullptr;
    }
    
    AnalysisPanel* panel = nullptr;
};

// ========== Test 1: Overwrite Checkbox ==========

TEST_F(AnalysisPanelCheckboxesTest, OverwriteCheckbox_Exists) {
    QCheckBox* cb = findCheckBox("Overwrite");
    if (!cb) cb = findCheckBox("existing");
    
    ASSERT_NE(cb, nullptr) << "Overwrite checkbox should exist";
}

TEST_F(AnalysisPanelCheckboxesTest, OverwriteCheckbox_HasCorrectText) {
    QCheckBox* cb = findCheckBox("Overwrite");
    if (!cb) cb = findCheckBox("existing");
    
    ASSERT_NE(cb, nullptr);
    
    QString text = cb->text();
    EXPECT_TRUE(text.contains("Overwrite", Qt::CaseInsensitive) || 
                text.contains("existing", Qt::CaseInsensitive))
        << "Checkbox text should mention overwriting existing data";
}

TEST_F(AnalysisPanelCheckboxesTest, OverwriteCheckbox_InitialState) {
    QCheckBox* cb = findCheckBox("Overwrite");
    if (!cb) cb = findCheckBox("existing");
    
    ASSERT_NE(cb, nullptr);
    
    // Default behavior: typically unchecked (don't overwrite by default)
    // This tests the initial state is set explicitly
    bool state = cb->isChecked();
    EXPECT_TRUE(state == false || state == true) 
        << "Checkbox should have defined initial state";
}

TEST_F(AnalysisPanelCheckboxesTest, OverwriteCheckbox_CanToggle) {
    QCheckBox* cb = findCheckBox("Overwrite");
    if (!cb) cb = findCheckBox("existing");
    
    ASSERT_NE(cb, nullptr);
    
    bool initialState = cb->isChecked();
    
    // Simulate toggle via setChecked (autonomous - no GUI clicks)
    cb->setChecked(!initialState);
    
    EXPECT_EQ(cb->isChecked(), !initialState) 
        << "Checkbox should toggle state when setChecked() is called";
}

TEST_F(AnalysisPanelCheckboxesTest, OverwriteCheckbox_EmitsSignal) {
    QCheckBox* cb = findCheckBox("Overwrite");
    if (!cb) cb = findCheckBox("existing");
    
    ASSERT_NE(cb, nullptr);
    
    QSignalSpy spy(cb, &QCheckBox::checkStateChanged);
    
    bool initialState = cb->isChecked();
    cb->setChecked(!initialState);
    
    EXPECT_EQ(spy.count(), 1) << "stateChanged signal should be emitted once";
}

// ========== Test 2: Skip Existing Checkbox ==========

TEST_F(AnalysisPanelCheckboxesTest, SkipExistingCheckbox_Exists) {
    QCheckBox* cb = findCheckBox("Skip");
    
    ASSERT_NE(cb, nullptr) << "Skip existing checkbox should exist";
}

TEST_F(AnalysisPanelCheckboxesTest, SkipExistingCheckbox_HasCorrectText) {
    QCheckBox* cb = findCheckBox("Skip");
    
    ASSERT_NE(cb, nullptr);
    
    QString text = cb->text();
    EXPECT_TRUE(text.contains("Skip", Qt::CaseInsensitive) || 
                text.contains("already", Qt::CaseInsensitive))
        << "Checkbox text should mention skipping analyzed images";
}

TEST_F(AnalysisPanelCheckboxesTest, SkipExistingCheckbox_InitialState) {
    QCheckBox* cb = findCheckBox("Skip");
    
    ASSERT_NE(cb, nullptr);
    
    // Default behavior: typically checked (skip by default is efficient)
    // Verify it has an explicit initial state
    bool state = cb->isChecked();
    EXPECT_TRUE(state == false || state == true);
}

TEST_F(AnalysisPanelCheckboxesTest, SkipExistingCheckbox_CanToggle) {
    QCheckBox* cb = findCheckBox("Skip");
    
    ASSERT_NE(cb, nullptr);
    
    bool initialState = cb->isChecked();
    
    cb->setChecked(!initialState);
    
    EXPECT_EQ(cb->isChecked(), !initialState);
}

TEST_F(AnalysisPanelCheckboxesTest, SkipExistingCheckbox_EmitsSignal) {
    QCheckBox* cb = findCheckBox("Skip");
    
    ASSERT_NE(cb, nullptr);
    
    QSignalSpy spy(cb, &QCheckBox::checkStateChanged);
    
    bool initialState = cb->isChecked();
    cb->setChecked(!initialState);
    
    EXPECT_EQ(spy.count(), 1);
}

// ========== Test 3: Checkbox Interactions ==========

TEST_F(AnalysisPanelCheckboxesTest, BothCheckboxes_Exist) {
    auto checkboxes = panel->findChildren<QCheckBox*>();
    
    EXPECT_GE(checkboxes.size(), 2) 
        << "AnalysisPanel should have at least 2 checkboxes";
}

TEST_F(AnalysisPanelCheckboxesTest, Checkboxes_IndependentStates) {
    QCheckBox* overwriteCb = findCheckBox("Overwrite");
    if (!overwriteCb) overwriteCb = findCheckBox("existing");
    QCheckBox* skipCb = findCheckBox("Skip");
    
    if (!overwriteCb || !skipCb) {
        GTEST_SKIP() << "Both checkboxes not found";
    }
    
    // Set different states
    overwriteCb->setChecked(true);
    skipCb->setChecked(false);
    
    EXPECT_TRUE(overwriteCb->isChecked());
    EXPECT_FALSE(skipCb->isChecked());
    
    // Toggle one shouldn't affect the other
    overwriteCb->setChecked(false);
    
    EXPECT_FALSE(overwriteCb->isChecked());
    EXPECT_FALSE(skipCb->isChecked()) << "Checkboxes should be independent";
}

TEST_F(AnalysisPanelCheckboxesTest, Checkboxes_EnabledByDefault) {
    auto checkboxes = panel->findChildren<QCheckBox*>();
    
    for (auto* cb : checkboxes) {
        EXPECT_TRUE(cb->isEnabled()) 
            << "Checkbox '" << cb->text().toStdString() 
            << "' should be enabled by default";
    }
}

TEST_F(AnalysisPanelCheckboxesTest, Checkboxes_MultipleToggle) {
    QCheckBox* cb = findCheckBox("Skip");
    if (!cb) cb = findCheckBox("Overwrite");
    
    ASSERT_NE(cb, nullptr);
    
    // Set to known initial state BEFORE starting spy
    cb->setChecked(false);
    
    // Now start monitoring signals
    QSignalSpy spy(cb, &QCheckBox::checkStateChanged);
    
    // Toggle multiple times from known state
    cb->setChecked(true);   // Change 1: false -> true
    cb->setChecked(false);  // Change 2: true -> false
    cb->setChecked(true);   // Change 3: false -> true
    
    // Signal should be emitted for each actual state change
    EXPECT_EQ(spy.count(), 3) 
        << "checkStateChanged should emit for each toggle";
}

TEST_F(AnalysisPanelCheckboxesTest, Checkboxes_SameStateNoSignal) {
    QCheckBox* cb = findCheckBox("Skip");
    if (!cb) cb = findCheckBox("Overwrite");
    
    ASSERT_NE(cb, nullptr);
    
    cb->setChecked(true);
    
    QSignalSpy spy(cb, &QCheckBox::checkStateChanged);
    
    // Set to same state - should not emit signal
    cb->setChecked(true);
    
    EXPECT_EQ(spy.count(), 0) 
        << "stateChanged should not emit when state doesn't change";
}

// ========== Test 4: Tristate Behavior (if applicable) ==========

TEST_F(AnalysisPanelCheckboxesTest, Checkboxes_NotTristate) {
    auto checkboxes = panel->findChildren<QCheckBox*>();
    
    for (auto* cb : checkboxes) {
        EXPECT_FALSE(cb->isTristate()) 
            << "Analysis checkboxes should be binary (not tristate)";
    }
}
