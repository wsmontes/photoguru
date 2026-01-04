#include <gtest/gtest.h>
#include "core/ThumbnailCache.h"
#include <QPixmap>

using namespace PhotoGuru;

class ThumbnailCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = &ThumbnailCache::instance();
    }
    
    ThumbnailCache* cache;
};

TEST_F(ThumbnailCacheTest, SingletonInstance) {
    ThumbnailCache& instance1 = ThumbnailCache::instance();
    ThumbnailCache& instance2 = ThumbnailCache::instance();
    EXPECT_EQ(&instance1, &instance2) << "Singleton should return same instance";
}

TEST_F(ThumbnailCacheTest, GetNonExistentThumbnail) {
    QPixmap result = cache->getThumbnail("/nonexistent/file.jpg", QSize(128, 128));
    EXPECT_FALSE(result.isNull()) << "Should return placeholder for non-existent file";
    EXPECT_EQ(result.width(), 128) << "Placeholder should match requested size";
}

TEST_F(ThumbnailCacheTest, CacheSize) {
    // Test that cache respects size parameter
    QPixmap thumb1 = cache->getThumbnail("/test/file1.jpg", QSize(128, 128));
    QPixmap thumb2 = cache->getThumbnail("/test/file2.jpg", QSize(256, 256));
    
    EXPECT_EQ(thumb1.width(), 128) << "First thumbnail should be 128px";
    EXPECT_EQ(thumb2.width(), 256) << "Second thumbnail should be 256px";
}
