#include <gtest/gtest.h>
#include "core/MetadataWriter.h"
#include "core/PhotoMetadata.h"
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QImage>
#include <QDebug>

using namespace PhotoGuru;

class MetadataWriterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid());
        
        // Create a test image
        testImagePath = tempDir->path() + "/test_image.jpg";
        QImage img(100, 100, QImage::Format_RGB32);
        img.fill(Qt::blue);
        ASSERT_TRUE(img.save(testImagePath, "JPEG", 95));
    }
    
    void TearDown() override {
        delete tempDir;
    }
    
    QTemporaryDir* tempDir = nullptr;
    QString testImagePath;
};

TEST_F(MetadataWriterTest, VerifyExifToolAvailable) {
    MetadataWriter& writer = MetadataWriter::instance();
    EXPECT_TRUE(writer.verifyExifToolAvailable()) 
        << "ExifTool should be available (install with: brew install exiftool)";
}

TEST_F(MetadataWriterTest, GetExifToolVersion) {
    MetadataWriter& writer = MetadataWriter::instance();
    QString version = writer.getExifToolVersion();
    EXPECT_FALSE(version.isEmpty()) << "Should return ExifTool version";
    qDebug() << "ExifTool version:" << version;
}

TEST_F(MetadataWriterTest, UpdateRating) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    // Write rating
    EXPECT_TRUE(writer.updateRating(testImagePath, 4));
    
    // Verify by reading back
    auto metadata = MetadataReader::instance().read(testImagePath);
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->rating, 4) << "Rating should be 4";
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateTitle) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QString testTitle = "Test Image Title";
    EXPECT_TRUE(writer.updateTitle(testImagePath, testTitle));
    
    // Verify
    auto metadata = MetadataReader::instance().read(testImagePath);
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->llm_title, testTitle) << "Title should match";
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateDescription) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QString testDesc = "This is a test image for unit testing";
    EXPECT_TRUE(writer.updateDescription(testImagePath, testDesc));
    
    auto metadata = MetadataReader::instance().read(testImagePath);
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->llm_description, testDesc);
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateKeywords) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QStringList keywords = {"test", "unittest", "photoguru"};
    EXPECT_TRUE(writer.updateKeywords(testImagePath, keywords));
    
    auto metadata = MetadataReader::instance().read(testImagePath);
    if (metadata.has_value()) {
        for (const QString& kw : keywords) {
            EXPECT_TRUE(metadata->llm_keywords.contains(kw)) 
                << "Should contain keyword: " << kw.toStdString();
        }
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateLocation) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    EXPECT_TRUE(writer.updateLocation(testImagePath, "San Francisco", "California", "USA"));
    
    auto metadata = MetadataReader::instance().read(testImagePath);
    if (metadata.has_value()) {
        EXPECT_EQ(metadata->location_name, "San Francisco, California, USA");
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateGPS) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    double lat = 37.7749;  // San Francisco
    double lon = -122.4194;
    
    EXPECT_TRUE(writer.updateGPS(testImagePath, lat, lon));
    
    auto metadata = MetadataReader::instance().read(testImagePath);
    if (metadata.has_value()) {
        EXPECT_NEAR(metadata->gps_lat, lat, 0.0001);
        EXPECT_NEAR(metadata->gps_lon, lon, 0.0001);
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, WriteCompleteMetadata) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    PhotoMetadata meta;
    meta.llm_title = "Complete Test";
    meta.llm_description = "Testing complete metadata write";
    meta.llm_keywords = {"complete", "test", "metadata"};
    meta.rating = 5;
    meta.llm_category = "Test Category";
    meta.location_name = "Test City, Test Country";
    
    EXPECT_TRUE(writer.write(testImagePath, meta));
    
    // Verify all fields
    auto readMeta = MetadataReader::instance().read(testImagePath);
    if (readMeta.has_value()) {
        EXPECT_EQ(readMeta->llm_title, meta.llm_title);
        EXPECT_EQ(readMeta->llm_description, meta.llm_description);
        EXPECT_EQ(readMeta->rating, meta.rating);
        EXPECT_EQ(readMeta->llm_category, meta.llm_category);
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateRatingBatch) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    // Create multiple test images
    QStringList imagePaths;
    for (int i = 0; i < 3; i++) {
        QString path = tempDir->path() + QString("/test_batch_%1.jpg").arg(i);
        QImage img(50, 50, QImage::Format_RGB32);
        img.fill(Qt::red);
        img.save(path, "JPEG");
        imagePaths << path;
    }
    
    // Batch update rating
    EXPECT_TRUE(writer.updateRatingBatch(imagePaths, 3));
    
    // Verify all images
    for (const QString& path : imagePaths) {
        auto meta = MetadataReader::instance().read(path);
        if (meta.has_value()) {
            EXPECT_EQ(meta->rating, 3);
        }
    }
}

TEST_F(MetadataWriterTest, WriteTechnicalMetadata) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    TechnicalMetadata tech;
    tech.sharpness_score = 0.95;
    tech.exposure_quality = 0.88;
    tech.aesthetic_score = 0.75;
    tech.overall_quality = 0.86;
    tech.face_count = 2;
    
    EXPECT_TRUE(writer.writeTechnicalMetadata(testImagePath, tech));
    
    // Verify by reading back
    auto meta = MetadataReader::instance().read(testImagePath);
    if (meta.has_value()) {
        EXPECT_NEAR(meta->technical.sharpness_score, tech.sharpness_score, 0.01);
        EXPECT_NEAR(meta->technical.aesthetic_score, tech.aesthetic_score, 0.01);
        EXPECT_EQ(meta->technical.face_count, tech.face_count);
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, WriteAIAnalysis) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QString title = "AI Generated Title";
    QString desc = "AI analysis description";
    QStringList keywords = {"ai", "analyzed", "test"};
    QString category = "AI Category";
    QString scene = "outdoor";
    QString mood = "happy";
    
    EXPECT_TRUE(writer.writeAIAnalysis(testImagePath, title, desc, keywords, 
                                       category, scene, mood));
    
    auto meta = MetadataReader::instance().read(testImagePath);
    if (meta.has_value()) {
        EXPECT_EQ(meta->llm_title, title);
        EXPECT_EQ(meta->llm_description, desc);
        EXPECT_EQ(meta->llm_category, category);
    } else {
        GTEST_SKIP() << "Metadata read not yet implemented";
    }
}

TEST_F(MetadataWriterTest, UpdateNonExistentFile) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QString fakePath = "/nonexistent/fake_image.jpg";
    EXPECT_FALSE(writer.updateRating(fakePath, 3)) 
        << "Should fail gracefully for non-existent file";
}

TEST_F(MetadataWriterTest, InvalidRating) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    // Rating should be 0-5
    EXPECT_FALSE(writer.updateRating(testImagePath, -1));
    EXPECT_FALSE(writer.updateRating(testImagePath, 6));
    EXPECT_TRUE(writer.updateRating(testImagePath, 0));
    EXPECT_TRUE(writer.updateRating(testImagePath, 5));
}

TEST_F(MetadataWriterTest, EmptyKeywords) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QStringList emptyKeywords;
    EXPECT_TRUE(writer.updateKeywords(testImagePath, emptyKeywords)) 
        << "Should handle empty keywords list";
}

TEST_F(MetadataWriterTest, AddKeywordsBatch) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    QStringList imagePaths;
    for (int i = 0; i < 2; i++) {
        QString path = tempDir->path() + QString("/test_kw_%1.jpg").arg(i);
        QImage img(50, 50, QImage::Format_RGB32);
        img.fill(Qt::green);
        img.save(path, "JPEG");
        imagePaths << path;
    }
    
    QStringList keywords = {"batch", "keyword", "test"};
    EXPECT_TRUE(writer.addKeywordsBatch(imagePaths, keywords));
}

TEST_F(MetadataWriterTest, CreateBackup) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    // Write some metadata first
    writer.updateTitle(testImagePath, "Original Title");
    
    // Create backup
    EXPECT_TRUE(writer.createBackup(testImagePath));
    
    // Verify backup file exists (format: path/basename_backup.ext)
    QFileInfo info(testImagePath);
    QString backupPath = info.path() + "/" + info.baseName() + "_backup." + info.suffix();
    EXPECT_TRUE(QFile::exists(backupPath));
}

TEST_F(MetadataWriterTest, RestoreFromBackup) {
    MetadataWriter& writer = MetadataWriter::instance();
    
    // Create backup
    writer.createBackup(testImagePath);
    
    // Modify original
    writer.updateTitle(testImagePath, "Modified Title");
    
    // Restore from backup
    EXPECT_TRUE(writer.restoreFromBackup(testImagePath));
}
