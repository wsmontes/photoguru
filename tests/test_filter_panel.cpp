#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QSignalSpy>
#include "../src/ui/FilterPanel.h"

using namespace PhotoGuru;

class FilterPanelTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        panel = new FilterPanel();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        panel = nullptr;
    }
    
    FilterPanel* panel = nullptr;
};

// Test panel creation
TEST_F(FilterPanelTest, PanelCreation) {
    ASSERT_NE(panel, nullptr);
    EXPECT_TRUE(panel->isWidgetType());
}

// Test signals
TEST_F(FilterPanelTest, SignalDefinitions) {
    QSignalSpy filterChangedSpy(panel, &FilterPanel::filterChanged);
    EXPECT_TRUE(filterChangedSpy.isValid());
}

// Test getCriteria returns valid criteria
TEST_F(FilterPanelTest, GetCriteria) {
    FilterCriteria criteria;
    EXPECT_NO_THROW(criteria = panel->getCriteria());
    
    // Default values should be reasonable
    EXPECT_GE(criteria.minQuality, 0.0);
    EXPECT_GE(criteria.minSharpness, 0.0);
    EXPECT_GE(criteria.minAesthetic, 0.0);
}

// Test reset
TEST_F(FilterPanelTest, Reset) {
    EXPECT_NO_THROW(panel->reset());
    
    FilterCriteria criteria = panel->getCriteria();
    EXPECT_EQ(criteria.minQuality, 0.0);
    EXPECT_EQ(criteria.minSharpness, 0.0);
    EXPECT_EQ(criteria.minAesthetic, 0.0);
    EXPECT_FALSE(criteria.onlyWithFaces);
    EXPECT_FALSE(criteria.onlyBestInBurst);
    EXPECT_FALSE(criteria.excludeDuplicates);
}

// Test multiple resets
TEST_F(FilterPanelTest, MultipleResets) {
    EXPECT_NO_THROW(panel->reset());
    EXPECT_NO_THROW(panel->reset());
    EXPECT_NO_THROW(panel->reset());
}

// Test getCriteria after reset
TEST_F(FilterPanelTest, GetCriteriaAfterReset) {
    panel->reset();
    FilterCriteria criteria1 = panel->getCriteria();
    panel->reset();
    FilterCriteria criteria2 = panel->getCriteria();
    
    EXPECT_EQ(criteria1.minQuality, criteria2.minQuality);
    EXPECT_EQ(criteria1.minSharpness, criteria2.minSharpness);
}

// Test default criteria values
TEST_F(FilterPanelTest, DefaultCriteriaValues) {
    FilterCriteria criteria = panel->getCriteria();
    
    EXPECT_EQ(criteria.minQuality, 0.0);
    EXPECT_EQ(criteria.minSharpness, 0.0);
    EXPECT_EQ(criteria.minAesthetic, 0.0);
    EXPECT_FALSE(criteria.onlyWithFaces);
    EXPECT_FALSE(criteria.onlyBestInBurst);
    EXPECT_FALSE(criteria.excludeDuplicates);
    EXPECT_FALSE(criteria.excludeBlurry);
    EXPECT_FALSE(criteria.onlyWithGPS);
    EXPECT_TRUE(criteria.categories.isEmpty());
    EXPECT_TRUE(criteria.scenes.isEmpty());
}
