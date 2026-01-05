#include "GoogleTakeoutImporter.h"
#include "GoogleTakeoutParser.h"
#include "MetadataWriter.h"
#include "Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace PhotoGuru {

QString GoogleTakeoutImporter::ImportResult::summary() const {
    return QString("Google Takeout Import: %1 images processed, %2 with JSON, %3 metadata applied, %4 errors")
        .arg(totalImages)
        .arg(withJson)
        .arg(metadataApplied)
        .arg(errors);
}

GoogleTakeoutImporter::ImportResult GoogleTakeoutImporter::importDirectory(
    const QString& directoryPath,
    const ImportOptions& options)
{
    ImportResult result;
    
    LOG_INFO("GoogleTakeoutImporter", "=== Starting Google Takeout import ===");
    LOG_INFO("GoogleTakeoutImporter", "Directory: " + directoryPath);
    
    // Check if directory looks like Google Takeout
    if (!GoogleTakeoutParser::isGoogleTakeoutDirectory(directoryPath)) {
        LOG_WARNING("GoogleTakeoutImporter", "Directory does not appear to be Google Takeout format");
        result.errorMessages.append("Not a Google Takeout directory");
        result.errors++;
        return result;
    }
    
    // Find all images
    QStringList images = findImagesInDirectory(directoryPath);
    result.totalImages = images.size();
    
    LOG_INFO("GoogleTakeoutImporter", QString("Found %1 images").arg(images.size()));
    
    // Process each image
    for (const QString& imagePath : images) {
        // Find JSON sidecar
        QString jsonPath = GoogleTakeoutParser::findJsonForImage(imagePath);
        
        if (jsonPath.isEmpty()) {
            LOG_DEBUG("GoogleTakeoutImporter", "No JSON found for: " + QFileInfo(imagePath).fileName());
            continue;
        }
        
        result.withJson++;
        
        // Parse JSON
        auto metadata = GoogleTakeoutParser::parseJsonFile(jsonPath);
        
        if (!metadata.isValid) {
            LOG_WARNING("GoogleTakeoutImporter", "Invalid JSON: " + jsonPath);
            result.errorMessages.append("Invalid JSON: " + jsonPath);
            result.errors++;
            continue;
        }
        
        // Check if has metadata to apply
        if (!metadata.hasMetadataToApply()) {
            LOG_DEBUG("GoogleTakeoutImporter", "No useful metadata in JSON for: " + QFileInfo(imagePath).fileName());
            continue;
        }
        
        // Apply metadata
        if (applyMetadataToImage(imagePath, metadata, options)) {
            result.metadataApplied++;
            LOG_INFO("GoogleTakeoutImporter", QString("✅ Applied metadata to: %1").arg(QFileInfo(imagePath).fileName()));
        } else {
            result.errors++;
            result.errorMessages.append("Failed to apply metadata: " + imagePath);
            LOG_ERROR("GoogleTakeoutImporter", "Failed to apply metadata to: " + imagePath);
        }
    }
    
    LOG_INFO("GoogleTakeoutImporter", "=== Import complete ===");
    LOG_INFO("GoogleTakeoutImporter", result.summary());
    
    return result;
}

bool GoogleTakeoutImporter::importSingleImage(
    const QString& imagePath,
    const ImportOptions& options)
{
    // Find JSON sidecar
    QString jsonPath = GoogleTakeoutParser::findJsonForImage(imagePath);
    
    if (jsonPath.isEmpty()) {
        LOG_DEBUG("GoogleTakeoutImporter", "No JSON found for: " + imagePath);
        return false;
    }
    
    // Parse JSON
    auto metadata = GoogleTakeoutParser::parseJsonFile(jsonPath);
    
    if (!metadata.isValid || !metadata.hasMetadataToApply()) {
        return false;
    }
    
    // Apply metadata
    return applyMetadataToImage(imagePath, metadata, options);
}

bool GoogleTakeoutImporter::applyMetadataToImage(
    const QString& imagePath,
    const GoogleTakeoutParser::TakeoutMetadata& metadata,
    const ImportOptions& options)
{
    MetadataWriter& writer = MetadataWriter::instance();
    
    LOG_DEBUG("GoogleTakeoutImporter", QString("Applying metadata to: %1").arg(imagePath));
    
    bool hasChanges = false;
    bool overallSuccess = true;
    
    // 1. Description/Caption
    if (options.applyDescription && !metadata.description.isEmpty()) {
        if (writer.updateDescription(imagePath, metadata.description)) {
            LOG_DEBUG("GoogleTakeoutImporter", QString("  Description: %1 chars").arg(metadata.description.length()));
            hasChanges = true;
        } else {
            LOG_WARNING("GoogleTakeoutImporter", "  Failed to update description");
            overallSuccess = false;
        }
    }
    
    // 2. Prepare keywords (people + albums)
    QStringList keywords;
    
    if (options.applyPeopleAsKeywords && !metadata.people.isEmpty()) {
        keywords.append(metadata.people);
        LOG_DEBUG("GoogleTakeoutImporter", QString("  People: %1").arg(metadata.people.join(", ")));
    }
    
    if (options.applyAlbumsAsKeywords && !metadata.albumNames.isEmpty()) {
        for (const QString& album : metadata.albumNames) {
            keywords.append("Album: " + album);
        }
        LOG_DEBUG("GoogleTakeoutImporter", QString("  Albums: %1").arg(metadata.albumNames.join(", ")));
    }
    
    // Apply keywords
    if (!keywords.isEmpty()) {
        if (writer.updateKeywords(imagePath, keywords)) {
            hasChanges = true;
        } else {
            LOG_WARNING("GoogleTakeoutImporter", "  Failed to update keywords");
            overallSuccess = false;
        }
    }
    
    // 3. Location (GPS coordinates)
    if (options.applyLocation && metadata.geoData.has_value()) {
        QGeoCoordinate coord = *metadata.geoData;
        
        if (writer.updateGPS(imagePath, coord.latitude(), coord.longitude())) {
            LOG_DEBUG("GoogleTakeoutImporter", QString("  GPS: %1, %2")
                .arg(coord.latitude(), 0, 'f', 6)
                .arg(coord.longitude(), 0, 'f', 6));
            hasChanges = true;
        } else {
            LOG_WARNING("GoogleTakeoutImporter", "  Failed to update GPS");
            overallSuccess = false;
        }
    }
    
    // 4. Location name (textual)
    if (options.applyLocation && !metadata.locationName.isEmpty()) {
        // Parse location name into parts if possible
        // Format is usually: "City, State, Country" or "City, Country"
        QStringList parts = metadata.locationName.split(",");
        
        QString city, state, country;
        
        if (parts.size() >= 2) {
            city = parts[0].trimmed();
            
            if (parts.size() >= 3) {
                state = parts[1].trimmed();
                country = parts[2].trimmed();
            } else {
                country = parts[1].trimmed();
            }
        } else {
            city = metadata.locationName;
        }
        
        if (writer.updateLocation(imagePath, city, state, country)) {
            LOG_DEBUG("GoogleTakeoutImporter", QString("  Location: %1").arg(metadata.locationName));
            hasChanges = true;
        } else {
            LOG_WARNING("GoogleTakeoutImporter", "  Failed to update location");
            overallSuccess = false;
        }
    }
    
    // 5. Date/Time
    // Note: MetadataWriter doesn't have a direct updateDateTime method,
    // so we'll use ExifTool directly for this
    if (options.applyDateTime && metadata.photoTakenTime.isValid()) {
        QString dateTimeStr = metadata.photoTakenTime.toString("yyyy:MM:dd HH:mm:ss");
        
        QProcess process;
        process.start("exiftool", {
            "-DateTimeOriginal=" + dateTimeStr,
            "-CreateDate=" + dateTimeStr,
            "-overwrite_original",
            imagePath
        });
        
        if (process.waitForFinished(5000) && process.exitCode() == 0) {
            LOG_DEBUG("GoogleTakeoutImporter", QString("  DateTime: %1").arg(dateTimeStr));
            hasChanges = true;
        } else {
            LOG_WARNING("GoogleTakeoutImporter", "  Failed to update datetime");
            overallSuccess = false;
        }
    }
    
    if (hasChanges && overallSuccess) {
        LOG_INFO("GoogleTakeoutImporter", QString("✅ Metadata applied to: %1").arg(QFileInfo(imagePath).fileName()));
    } else if (hasChanges) {
        LOG_WARNING("GoogleTakeoutImporter", QString("⚠️  Partial metadata applied to: %1").arg(QFileInfo(imagePath).fileName()));
    }
    
    return hasChanges && overallSuccess;
}

QStringList GoogleTakeoutImporter::findImagesInDirectory(const QString& directoryPath) {
    QDir dir(directoryPath);
    if (!dir.exists()) {
        return QStringList();
    }
    
    // Supported image extensions (common formats)
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.JPG" << "*.JPEG"
            << "*.png" << "*.PNG"
            << "*.heic" << "*.HEIC" << "*.heif" << "*.HEIF"
            << "*.tif" << "*.tiff" << "*.TIF" << "*.TIFF"
            << "*.webp" << "*.WEBP";
    
    // Get all matching files
    QStringList images;
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    for (const QFileInfo& fileInfo : fileList) {
        // Skip edited versions (Google Takeout sometimes exports edited copies with -edited suffix)
        QString fileName = fileInfo.fileName();
        if (fileName.contains("-edited", Qt::CaseInsensitive) || 
            fileName.contains("_edit", Qt::CaseInsensitive)) {
            continue;
        }
        
        images.append(fileInfo.absoluteFilePath());
    }
    
    return images;
}

} // namespace PhotoGuru
