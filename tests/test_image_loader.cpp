#include <gtest/gtest.h>
#include "core/ImageLoader.h"
#include <QImage>
#include <QTemporaryDir>
#include <QFile>

using namespace PhotoGuru;

class ImageLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader = &ImageLoader::instance();
    }
    
    ImageLoader* loader;
};

TEST_F(ImageLoaderTest, SingletonInstance) {
    ImageLoader& instance1 = ImageLoader::instance();
    ImageLoader& instance2 = ImageLoader::instance();
    EXPECT_EQ(&instance1, &instance2) << "Singleton should return same instance";
}

TEST_F(ImageLoaderTest, SupportedExtensions) {
    QStringList extensions = loader->supportedExtensions();
    
    EXPECT_FALSE(extensions.isEmpty()) << "Should have supported extensions";
    EXPECT_TRUE(extensions.contains("*.jpg")) << "Should support JPG";
    EXPECT_TRUE(extensions.contains("*.png")) << "Should support PNG";
    EXPECT_TRUE(extensions.contains("*.heic")) << "Should support HEIC";
}

TEST_F(ImageLoaderTest, LoadNonExistentImage) {
    auto resultOpt = loader->load("/nonexistent/image.jpg");
    EXPECT_FALSE(resultOpt.has_value()) << "Should return empty optional for non-existent file";
}

TEST_F(ImageLoaderTest, SupportedFormats) {
    // Test that loader has formats registered
    QStringList exts = loader->supportedExtensions();
    EXPECT_FALSE(exts.isEmpty()) << "Should have supported formats";
    
    // Test specific format support
    bool hasJPG = false, hasPNG = false, hasHEIC = false;
    for (const QString& ext : exts) {
        if (ext.contains("jpg", Qt::CaseInsensitive)) hasJPG = true;
        if (ext.contains("png", Qt::CaseInsensitive)) hasPNG = true;
        if (ext.contains("heic", Qt::CaseInsensitive)) hasHEIC = true;
    }
    
    EXPECT_TRUE(hasJPG) << "Should support JPG format";
    EXPECT_TRUE(hasPNG) << "Should support PNG format";
    EXPECT_TRUE(hasHEIC) << "Should support HEIC format";
}
