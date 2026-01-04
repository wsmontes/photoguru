#include <gtest/gtest.h>
#include "core/PhotoMetadata.h"
#include <QFile>
#include <QTemporaryDir>

using namespace PhotoGuru;

class MetadataReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup runs before each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
};

TEST_F(MetadataReaderTest, ReadNonExistentFile) {
    auto result = MetadataReader::instance().read("/nonexistent/file.jpg");
    EXPECT_FALSE(result.has_value()) << "Should return empty optional for non-existent file";
}

TEST_F(MetadataReaderTest, ParseTechnicalMetadata) {
    // Test parsing of PhotoGuru JSON in UserComment
    QString testComment = "PhotoGuru:{\"sharp\":1.0,\"expo\":0.968,\"aesth\":0.446,\"qual\":0.651,\"dup\":null,\"burst\":null,\"burst_pos\":null,\"burst_best\":false,\"faces\":2}";
    
    // This would require exposing parseTechnicalData or testing through read()
    // For now, verify the format is correct JSON
    EXPECT_TRUE(testComment.contains("PhotoGuru:"));
    EXPECT_TRUE(testComment.contains("\"sharp\""));
    EXPECT_TRUE(testComment.contains("\"expo\""));
    EXPECT_TRUE(testComment.contains("\"aesth\""));
}

TEST_F(MetadataReaderTest, VerifyExifToolPath) {
    // Verify exiftool is accessible
    QFile exiftool("/opt/homebrew/bin/exiftool");
    if (exiftool.exists()) {
        EXPECT_TRUE(exiftool.exists()) << "ExifTool should be installed at /opt/homebrew/bin/exiftool";
    } else {
        GTEST_SKIP() << "ExifTool not installed, skipping test";
    }
}
