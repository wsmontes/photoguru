#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QSignalSpy>
#include "../src/ui/SemanticSearch.h"
#include "../src/core/PhotoMetadata.h"

using namespace PhotoGuru;

class SemanticSearchTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }
    
    void SetUp() override {
        searchWidget = new SemanticSearch();
    }
    
    void TearDown() override {
        // Don't delete - let Qt handle cleanup
        searchWidget = nullptr;
    }
    
    SemanticSearch* searchWidget = nullptr;
};

// Test widget creation
TEST_F(SemanticSearchTest, WidgetCreation) {
    ASSERT_NE(searchWidget, nullptr);
    EXPECT_TRUE(searchWidget->isWidgetType());
}

// Test signals
TEST_F(SemanticSearchTest, SignalDefinitions) {
    QSignalSpy photoSelectedSpy(searchWidget, &SemanticSearch::photoSelected);
    QSignalSpy searchStartedSpy(searchWidget, &SemanticSearch::searchStarted);
    QSignalSpy searchCompletedSpy(searchWidget, &SemanticSearch::searchCompleted);
    
    EXPECT_TRUE(photoSelectedSpy.isValid());
    EXPECT_TRUE(searchStartedSpy.isValid());
    EXPECT_TRUE(searchCompletedSpy.isValid());
}

// Test setPhotos with empty list
TEST_F(SemanticSearchTest, SetEmptyPhotoList) {
    QList<PhotoMetadata> emptyList;
    EXPECT_NO_THROW(searchWidget->setPhotos(emptyList));
}

// Test setPhotos with data
TEST_F(SemanticSearchTest, SetPhotoList) {
    QList<PhotoMetadata> photos;
    
    PhotoMetadata meta1;
    meta1.filepath = "/test/sunset.jpg";
    meta1.llm_title = "Beautiful Sunset";
    meta1.llm_description = "A stunning sunset over the ocean";
    photos.append(meta1);
    
    PhotoMetadata meta2;
    meta2.filepath = "/test/mountain.jpg";
    meta2.llm_title = "Mountain Peak";
    meta2.llm_description = "Snow-covered mountain peak";
    photos.append(meta2);
    
    EXPECT_NO_THROW(searchWidget->setPhotos(photos));
}

// Test performSearch with empty query
TEST_F(SemanticSearchTest, SearchEmptyQuery) {
    EXPECT_NO_THROW(searchWidget->performSearch(""));
}

// Test performSearch with query
TEST_F(SemanticSearchTest, SearchWithQuery) {
    QList<PhotoMetadata> photos;
    PhotoMetadata meta;
    meta.filepath = "/test/beach.jpg";
    meta.llm_title = "Beach Scene";
    photos.append(meta);
    
    searchWidget->setPhotos(photos);
    EXPECT_NO_THROW(searchWidget->performSearch("beach"));
}

// Test multiple searches
TEST_F(SemanticSearchTest, MultipleSearches) {
    QList<PhotoMetadata> photos;
    for (int i = 0; i < 5; i++) {
        PhotoMetadata meta;
        meta.filepath = QString("/test/photo%1.jpg").arg(i);
        meta.llm_title = QString("Photo %1").arg(i);
        photos.append(meta);
    }
    
    searchWidget->setPhotos(photos);
    EXPECT_NO_THROW(searchWidget->performSearch("photo"));
    EXPECT_NO_THROW(searchWidget->performSearch("nature"));
    EXPECT_NO_THROW(searchWidget->performSearch("landscape"));
}

// Test search with special characters
TEST_F(SemanticSearchTest, SearchSpecialCharacters) {
    EXPECT_NO_THROW(searchWidget->performSearch("sunset & sunrise"));
    EXPECT_NO_THROW(searchWidget->performSearch("café"));
    EXPECT_NO_THROW(searchWidget->performSearch("北京"));
}
