#include "ImageLoader.h"
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>

#ifdef __APPLE__
#include <libraw/libraw.h>
#else
#include <libraw.h>
#endif

#ifdef HEIF_SUPPORT_ENABLED
#include <libheif/heif.h>
#endif

namespace PhotoGuru {

ImageLoader& ImageLoader::instance() {
    static ImageLoader loader;
    return loader;
}

ImageFormat ImageLoader::detectFormat(const QString& filePath) const {
    QString ext = QFileInfo(filePath).suffix().toLower();
    
    // RAW formats
    static const QStringList rawExts = {
        "cr2", "cr3", "nef", "nrw", "arw", "srf", "sr2", 
        "dng", "orf", "rw2", "pef", "raf", "raw", "rwl",
        "3fr", "ari", "bay", "crw", "dcr", "erf", "fff",
        "iiq", "k25", "kdc", "mdc", "mef", "mos", "mrw",
        "nrw", "obm", "ptx", "pxn", "r3d", "rdc", "rwz",
        "srw", "x3f"
    };
    if (rawExts.contains(ext)) return ImageFormat::RAW;
    
    // HEIF/HEIC
    if (ext == "heif" || ext == "heic") return ImageFormat::HEIF;
    
    // Standard formats
    if (ext == "jpg" || ext == "jpeg") return ImageFormat::JPEG;
    if (ext == "png") return ImageFormat::PNG;
    if (ext == "tif" || ext == "tiff") return ImageFormat::TIFF;
    if (ext == "webp") return ImageFormat::WebP;
    
    return ImageFormat::Unknown;
}

std::optional<QImage> ImageLoader::load(const QString& filePath, const QSize& maxSize) {
    ImageFormat format = detectFormat(filePath);
    
    switch (format) {
        case ImageFormat::RAW: {
            RawLoadOptions opts;
            opts.halfSize = maxSize.isValid() && (maxSize.width() < 2000);
            return loadRAW(filePath, opts);
        }
        case ImageFormat::HEIF:
            return loadHEIF(filePath);
        default:
            return loadStandard(filePath);
    }
}

std::optional<QImage> ImageLoader::loadRAW(const QString& filePath, 
                                           const RawLoadOptions& options) {
    try {
        LibRaw rawProcessor;
        
        // Open file
        int ret = rawProcessor.open_file(filePath.toStdString().c_str());
        if (ret != LIBRAW_SUCCESS) {
            qWarning() << "LibRaw: Failed to open" << filePath;
            return std::nullopt;
        }
        
        // Configure processing
        rawProcessor.imgdata.params.use_camera_wb = options.autoWB ? 1 : 0;
        rawProcessor.imgdata.params.use_auto_wb = options.autoWB ? 1 : 0;
        rawProcessor.imgdata.params.half_size = options.halfSize ? 1 : 0;
        rawProcessor.imgdata.params.output_bps = options.outputBitDepth;
        rawProcessor.imgdata.params.no_auto_bright = 0;  // Enable auto-brightness
        rawProcessor.imgdata.params.highlight = 1;  // Clip highlights
        rawProcessor.imgdata.params.output_color = 1;  // sRGB
        
        // Unpack and process
        ret = rawProcessor.unpack();
        if (ret != LIBRAW_SUCCESS) {
            qWarning() << "LibRaw: Failed to unpack" << filePath;
            return std::nullopt;
        }
        
        ret = rawProcessor.dcraw_process();
        if (ret != LIBRAW_SUCCESS) {
            qWarning() << "LibRaw: Failed to process" << filePath;
            return std::nullopt;
        }
        
        // Get processed image
        libraw_processed_image_t* image = rawProcessor.dcraw_make_mem_image(&ret);
        if (!image) {
            qWarning() << "LibRaw: Failed to make memory image";
            return std::nullopt;
        }
        
        // Convert to QImage
        QImage result;
        if (image->type == LIBRAW_IMAGE_BITMAP) {
            if (image->colors == 3) {
                // RGB image
                result = QImage(image->data, image->width, image->height, 
                               image->width * 3, QImage::Format_RGB888).copy();
            } else if (image->colors == 4) {
                // RGBA image
                result = QImage(image->data, image->width, image->height,
                               image->width * 4, QImage::Format_RGBA8888).copy();
            }
        }
        
        LibRaw::dcraw_clear_mem(image);
        
        if (result.isNull()) {
            qWarning() << "Failed to convert RAW to QImage";
            return std::nullopt;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        qWarning() << "Exception loading RAW:" << e.what();
        return std::nullopt;
    }
}

std::optional<QImage> ImageLoader::loadHEIF(const QString& filePath) {
#ifdef HEIF_SUPPORT_ENABLED
    try {
        heif_context* ctx = heif_context_alloc();
        if (!ctx) return std::nullopt;
        
        heif_error error = heif_context_read_from_file(ctx, 
            filePath.toStdString().c_str(), nullptr);
        
        if (error.code != heif_error_Ok) {
            heif_context_free(ctx);
            qWarning() << "HEIF: Failed to read file:" << error.message;
            return std::nullopt;
        }
        
        // Get primary image handle
        heif_image_handle* handle;
        error = heif_context_get_primary_image_handle(ctx, &handle);
        if (error.code != heif_error_Ok) {
            heif_context_free(ctx);
            return std::nullopt;
        }
        
        // Decode image
        heif_image* img;
        error = heif_decode_image(handle, &img, heif_colorspace_RGB,
                                  heif_chroma_interleaved_RGB, nullptr);
        
        if (error.code != heif_error_Ok) {
            heif_image_handle_release(handle);
            heif_context_free(ctx);
            return std::nullopt;
        }
        
        // Get image data
        int stride;
        const uint8_t* data = heif_image_get_plane_readonly(img, 
            heif_channel_interleaved, &stride);
        
        int width = heif_image_get_width(img, heif_channel_interleaved);
        int height = heif_image_get_height(img, heif_channel_interleaved);
        
        // Create QImage (must copy since data lifetime is limited)
        QImage temp(data, width, height, stride, QImage::Format_RGB888);
        QImage result = temp.copy();
        
        // Cleanup
        heif_image_release(img);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        
        return result;
        
    } catch (const std::exception& e) {
        qWarning() << "Exception loading HEIF:" << e.what();
        return std::nullopt;
    }
#else
    qWarning() << "HEIF support not compiled in";
    return std::nullopt;
#endif
}

std::optional<QImage> ImageLoader::loadStandard(const QString& filePath) {
    QImageReader reader(filePath);
    reader.setAutoTransform(true);  // Handle EXIF orientation
    
    QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Failed to load image:" << reader.errorString();
        return std::nullopt;
    }
    
    return image;
}

QSize ImageLoader::getImageDimensions(const QString& filePath) const {
    ImageFormat format = detectFormat(filePath);
    
    if (format == ImageFormat::RAW) {
        LibRaw rawProcessor;
        if (rawProcessor.open_file(filePath.toStdString().c_str()) == LIBRAW_SUCCESS) {
            return QSize(rawProcessor.imgdata.sizes.width, 
                        rawProcessor.imgdata.sizes.height);
        }
    }
    
    // Use QImageReader for other formats (fast, doesn't load full image)
    QImageReader reader(filePath);
    return reader.size();
}

bool ImageLoader::isSupported(const QString& filePath) const {
    return detectFormat(filePath) != ImageFormat::Unknown;
}

QStringList ImageLoader::supportedExtensions() const {
    QStringList exts;
    
    // RAW formats
    exts << "*.cr2" << "*.cr3" << "*.nef" << "*.arw" << "*.dng" 
         << "*.orf" << "*.rw2" << "*.pef" << "*.raf" << "*.raw";
    
    // HEIF
    exts << "*.heif" << "*.heic";
    
    // Standard
    exts << "*.jpg" << "*.jpeg" << "*.png" << "*.tiff" << "*.tif" << "*.webp";
    
    return exts;
}

} // namespace PhotoGuru
