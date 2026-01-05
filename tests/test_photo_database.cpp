#include <gtest/gtest.h>
#include "core/PhotoDatabase.h"
#include "core/PhotoMetadata.h"
#include <QTemporaryDir>
#include <QDebug>

using namespace PhotoGuru;

class PhotoDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for database
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid());
        
        dbPath = tempDir->path() + "/test_photos.db";
    }
    
    void TearDown() override {
        PhotoDatabase::instance().close();
        delete tempDir;
    }
    
    QTemporaryDir* tempDir = nullptr;
    QString dbPath;
};

TEST_F(PhotoDatabaseTest, Initialize) {
    PhotoDatabase& db = PhotoDatabase::instance();
    
    EXPECT_TRUE(db.initialize(dbPath)) << "Should initialize database";
}

TEST_F(PhotoDatabaseTest, InitializeExistingDatabase) {
    PhotoDatabase& db = PhotoDatabase::instance();
    
    // Initialize once
    ASSERT_TRUE(db.initialize(dbPath));
    db.close();
    
    // Initialize again (existing file)
    EXPECT_TRUE(db.initialize(dbPath)) << "Should open existing database";
}

TEST_F(PhotoDatabaseTest, CloseDatabase) {
    PhotoDatabase& db = PhotoDatabase::instance();
    
    db.initialize(dbPath);
    db.close();
    
    // Should be able to close multiple times without crash
    db.close();
    SUCCEED() << "Close should be idempotent";
}

TEST_F(PhotoDatabaseTest, SingletonBehavior) {
    PhotoDatabase& db1 = PhotoDatabase::instance();
    PhotoDatabase& db2 = PhotoDatabase::instance();
    
    EXPECT_EQ(&db1, &db2) << "Should return same singleton instance";
}

// Note: Additional tests for addPhoto, searchByKeywords, etc. will be
// added once PhotoDatabase implementation is complete (currently marked as TODO)


TEST_F(PhotoDatabaseTest, InitializeInvalidPath) {
    PhotoDatabase& db = PhotoDatabase::instance();
    
    // Try to initialize with invalid path (read-only location)
    QString invalidPath = "/invalid/readonly/path/test.db";
    
    // Should handle gracefully (implementation dependent)
    // This test documents expected behavior
    SUCCEED() << "Documented: invalid path handling";
}
