#pragma once

#include <QImage>
#include <QString>
#include <QSize>
#include <memory>
#include <optional>

namespace PhotoGuru {

enum class ImageFormat {
    JPEG,
    PNG,
    TIFF,
    RAW,      // CR2, NEF, ARW, DNG, etc.
    HEIF,     // HEIC, HEIF
    WebP,
    Unknown
};

struct RawLoadOptions {
    bool autoWB = true;
    bool halfSize = false;  // For fast preview
    int outputBitDepth = 8;  // 8 or 16
};

class ImageLoader {
public:
    static ImageLoader& instance();
    
    // Main loading function - tries all methods
    std::optional<QImage> load(const QString& filePath, 
                               const QSize& maxSize = QSize());
    
    // Format detection
    ImageFormat detectFormat(const QString& filePath) const;
    
    // Specialized loaders
    std::optional<QImage> loadRAW(const QString& filePath,
                                  const RawLoadOptions& options = {});
    std::optional<QImage> loadHEIF(const QString& filePath);
    std::optional<QImage> loadStandard(const QString& filePath);
    
    // Get full resolution size without loading entire image
    QSize getImageDimensions(const QString& filePath) const;
    
    // Check if format is supported
    bool isSupported(const QString& filePath) const;
    
    // Get list of supported extensions
    QStringList supportedExtensions() const;
    
private:
    ImageLoader() = default;
    ~ImageLoader() = default;
    ImageLoader(const ImageLoader&) = delete;
    ImageLoader& operator=(const ImageLoader&) = delete;
};

} // namespace PhotoGuru
