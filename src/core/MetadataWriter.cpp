#include "MetadataWriter.h"
#include "ExifToolDaemon.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QDir>

namespace PhotoGuru {

MetadataWriter& MetadataWriter::instance() {
    static MetadataWriter writer;
    return writer;
}

bool MetadataWriter::verifyExifToolAvailable() const {
    QProcess process;
    process.start("exiftool", QStringList() << "-ver");
    process.waitForFinished(3000);
    return process.exitCode() == 0;
}

QString MetadataWriter::getExifToolVersion() const {
    QProcess process;
    process.start("exiftool", QStringList() << "-ver");
    process.waitForFinished(3000);
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool MetadataWriter::validateFilePath(const QString& filePath) const {
    QFileInfo info(filePath);
    if (!info.exists()) {
        qWarning() << "File does not exist:" << filePath;
        return false;
    }
    if (!info.isWritable()) {
        qWarning() << "File is not writable:" << filePath;
        return false;
    }
    return true;
}

QString MetadataWriter::escapeForExifTool(const QString& value) const {
    // ExifTool handles escaping internally, but we need to handle newlines
    QString escaped = value;
    escaped.replace("\r\n", "\n");
    return escaped;
}

bool MetadataWriter::runExifTool(const QString& filePath, const QStringList& args, QString* output) {
    if (!validateFilePath(filePath)) {
        return false;
    }
    
    qDebug() << "[MetadataWriter] Executing:" << args.join(" ");
    
    // Use ExifToolDaemon (stay-open mode) 
    QString result = ExifToolDaemon::instance().executeCommand(args);
    
    if (output) {
        *output = result;
    }
    
    // Check for errors first
    if (result.contains("Error:", Qt::CaseInsensitive) || 
        result.contains("Warning:", Qt::CaseInsensitive) ||
        result.contains("weren't updated", Qt::CaseInsensitive)) {
        qWarning() << "[MetadataWriter] ExifTool error:" << result;
        return false;
    }
    
    // Check for success indicators
    bool success = result.contains("image files updated") || 
                   result.contains("image files created") ||
                   result.isEmpty(); // Stay-open mode pode retornar vazio em sucesso
    
    if (!success) {
        qWarning() << "[MetadataWriter] Unexpected output:" << result;
    } else {
        qDebug() << "[MetadataWriter] Write successful. Output:" << result;
    }
    
    return success;
}

bool MetadataWriter::updateRating(const QString& filePath, int rating) {
    if (rating < 0 || rating > 5) {
        qWarning() << "Invalid rating value:" << rating;
        return false;
    }
    
    QStringList args;
    args << "-overwrite_original";
    args << QString("-XMP:Rating=%1").arg(rating);
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateTitle(const QString& filePath, const QString& title) {
    QStringList args;
    args << "-overwrite_original";
    args << QString("-XMP:Title=%1").arg(escapeForExifTool(title));
    args << QString("-IPTC:ObjectName=%1").arg(escapeForExifTool(title));
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateDescription(const QString& filePath, const QString& description) {
    QStringList args;
    args << "-overwrite_original";
    args << QString("-XMP:Description=%1").arg(escapeForExifTool(description));
    args << QString("-IPTC:Caption-Abstract=%1").arg(escapeForExifTool(description));
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateKeywords(const QString& filePath, const QStringList& keywords) {
    QStringList args;
    args << "-overwrite_original";
    
    // Clear existing keywords first
    args << "-XMP:Subject=";
    args << "-IPTC:Keywords=";
    
    // Add new keywords
    for (const QString& keyword : keywords) {
        args << QString("-XMP:Subject+=%1").arg(escapeForExifTool(keyword));
        args << QString("-IPTC:Keywords+=%1").arg(escapeForExifTool(keyword));
    }
    
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateCategory(const QString& filePath, const QString& category) {
    QStringList args;
    args << "-overwrite_original";
    // Use XMP-photoshop:Category (works in HEIC, JPEG, etc)
    args << QString("-XMP-photoshop:Category=%1").arg(escapeForExifTool(category));
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateLocation(const QString& filePath, const QString& city, 
                                    const QString& state, const QString& country) {
    QStringList args;
    args << "-overwrite_original";
    
    if (!city.isEmpty()) {
        args << QString("-IPTC:City=%1").arg(escapeForExifTool(city));
        args << QString("-XMP:City=%1").arg(escapeForExifTool(city));
    }
    
    if (!state.isEmpty()) {
        args << QString("-IPTC:Province-State=%1").arg(escapeForExifTool(state));
        args << QString("-XMP:State=%1").arg(escapeForExifTool(state));
    }
    
    if (!country.isEmpty()) {
        args << QString("-IPTC:Country-PrimaryLocationName=%1").arg(escapeForExifTool(country));
        args << QString("-XMP:Country=%1").arg(escapeForExifTool(country));
    }
    
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateGPS(const QString& filePath, double lat, double lon) {
    QStringList args;
    args << "-overwrite_original";
    args << QString("-GPSLatitude=%1").arg(lat, 0, 'f', 6);
    args << QString("-GPSLongitude=%1").arg(lon, 0, 'f', 6);
    args << "-GPSLatitudeRef=" + QString(lat >= 0 ? "N" : "S");
    args << "-GPSLongitudeRef=" + QString(lon >= 0 ? "E" : "W");
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::updateRatingBatch(const QStringList& filePaths, int rating) {
    if (rating < 0 || rating > 5) {
        qWarning() << "Invalid rating value:" << rating;
        return false;
    }
    
    QStringList args;
    args << "-overwrite_original";
    args << QString("-XMP:Rating=%1").arg(rating);
    args << filePaths;
    
    // For batch operations, we need to validate all files first
    for (const QString& path : filePaths) {
        if (!validateFilePath(path)) {
            return false;
        }
    }
    
    QProcess process;
    process.start("exiftool", args);
    
    if (!process.waitForFinished(60000)) { // 60 second timeout for batch
        qWarning() << "ExifTool batch operation timed out";
        return false;
    }
    
    return process.exitCode() == 0;
}

bool MetadataWriter::addKeywordsBatch(const QStringList& filePaths, const QStringList& keywords) {
    QStringList args;
    args << "-overwrite_original";
    
    for (const QString& keyword : keywords) {
        args << QString("-XMP:Subject+=%1").arg(escapeForExifTool(keyword));
        args << QString("-IPTC:Keywords+=%1").arg(escapeForExifTool(keyword));
    }
    
    args << filePaths;
    
    QProcess process;
    process.start("exiftool", args);
    
    if (!process.waitForFinished(60000)) {
        qWarning() << "ExifTool batch operation timed out";
        return false;
    }
    
    return process.exitCode() == 0;
}

bool MetadataWriter::removeKeywordsBatch(const QStringList& filePaths, const QStringList& keywords) {
    QStringList args;
    args << "-overwrite_original";
    
    for (const QString& keyword : keywords) {
        args << QString("-XMP:Subject-=%1").arg(escapeForExifTool(keyword));
        args << QString("-IPTC:Keywords-=%1").arg(escapeForExifTool(keyword));
    }
    
    args << filePaths;
    
    QProcess process;
    process.start("exiftool", args);
    
    if (!process.waitForFinished(60000)) {
        qWarning() << "ExifTool batch operation timed out";
        return false;
    }
    
    return process.exitCode() == 0;
}

QString MetadataWriter::buildTechnicalJSON(const TechnicalMetadata& technical) const {
    QJsonObject json;
    json["sharp"] = technical.sharpness_score;
    json["expo"] = technical.exposure_quality;
    json["aesth"] = technical.aesthetic_score;
    json["qual"] = technical.overall_quality;
    json["dup"] = technical.duplicate_group.isEmpty() ? QJsonValue() : technical.duplicate_group;
    json["burst"] = technical.burst_group.isEmpty() ? QJsonValue() : technical.burst_group;
    json["burst_pos"] = technical.burst_position >= 0 ? technical.burst_position : QJsonValue();
    json["burst_best"] = technical.is_best_in_burst;
    json["faces"] = technical.face_count;
    
    QJsonDocument doc(json);
    return "PhotoGuru:" + QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

bool MetadataWriter::writeTechnicalMetadata(const QString& filePath, const TechnicalMetadata& technical) {
    QString jsonData = buildTechnicalJSON(technical);
    
    QStringList args;
    args << "-overwrite_original";
    args << QString("-EXIF:UserComment=%1").arg(jsonData);
    args << QString("-XMP:CreatorTool=PhotoGuru");
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::writeAIAnalysis(const QString& filePath, const QString& title,
                                     const QString& description, const QStringList& keywords,
                                     const QString& category, const QString& scene, const QString& mood) {
    QStringList args;
    args << "-overwrite_original";
    args << QString("-XMP:CreatorTool=PhotoGuru");
    
    if (!title.isEmpty()) {
        args << QString("-XMP:Title=%1").arg(escapeForExifTool(title));
        args << QString("-IPTC:ObjectName=%1").arg(escapeForExifTool(title));
    }
    
    if (!description.isEmpty()) {
        args << QString("-XMP:Description=%1").arg(escapeForExifTool(description));
        args << QString("-IPTC:Caption-Abstract=%1").arg(escapeForExifTool(description));
    }
    
    // Clear and set keywords
    if (!keywords.isEmpty()) {
        args << "-XMP:Subject=";
        args << "-IPTC:Keywords=";
        for (const QString& keyword : keywords) {
            args << QString("-XMP:Subject+=%1").arg(escapeForExifTool(keyword));
            args << QString("-IPTC:Keywords+=%1").arg(escapeForExifTool(keyword));
        }
    }
    
    if (!category.isEmpty()) {
        // Use XMP-photoshop:Category for broader format support
        args << QString("-XMP-photoshop:Category=%1").arg(escapeForExifTool(category));
    }
    
    if (!scene.isEmpty()) {
        args << QString("-XMP:LocationShown=%1").arg(escapeForExifTool(scene));
    }
    
    if (!mood.isEmpty()) {
        args << QString("-XMP:Mood=%1").arg(escapeForExifTool(mood));
    }
    
    args << filePath;
    
    return runExifTool(filePath, args);
}

QStringList MetadataWriter::buildMetadataArgs(const PhotoMetadata& metadata) const {
    QStringList args;
    args << "-overwrite_original";
    args << "-XMP:CreatorTool=PhotoGuru";
    
    // Rating
    if (metadata.rating > 0) {
        args << QString("-XMP:Rating=%1").arg(metadata.rating);
    }
    
    // AI Analysis
    if (!metadata.llm_title.isEmpty()) {
        args << QString("-XMP:Title=%1").arg(escapeForExifTool(metadata.llm_title));
        args << QString("-IPTC:ObjectName=%1").arg(escapeForExifTool(metadata.llm_title));
    }
    
    if (!metadata.llm_description.isEmpty()) {
        args << QString("-XMP:Description=%1").arg(escapeForExifTool(metadata.llm_description));
        args << QString("-IPTC:Caption-Abstract=%1").arg(escapeForExifTool(metadata.llm_description));
    }
    
    if (!metadata.llm_keywords.isEmpty()) {
        args << "-XMP:Subject=";
        args << "-IPTC:Keywords=";
        for (const QString& keyword : metadata.llm_keywords) {
            args << QString("-XMP:Subject+=%1").arg(escapeForExifTool(keyword));
            args << QString("-IPTC:Keywords+=%1").arg(escapeForExifTool(keyword));
        }
    }
    
    if (!metadata.llm_category.isEmpty()) {
        // Use XMP-photoshop:Category for broader format support (HEIC, etc)
        args << QString("-XMP-photoshop:Category=%1").arg(escapeForExifTool(metadata.llm_category));
    }
    
    // Location
    if (!metadata.location_name.isEmpty()) {
        QStringList parts = metadata.location_name.split(", ");
        if (parts.size() >= 1) {
            args << QString("-IPTC:City=%1").arg(escapeForExifTool(parts[0]));
        }
        if (parts.size() >= 2) {
            args << QString("-IPTC:Province-State=%1").arg(escapeForExifTool(parts[1]));
        }
        if (parts.size() >= 3) {
            args << QString("-IPTC:Country-PrimaryLocationName=%1").arg(escapeForExifTool(parts[2]));
        }
    }
    
    // GPS
    if (metadata.gps_lat != 0.0 || metadata.gps_lon != 0.0) {
        args << QString("-GPSLatitude=%1").arg(metadata.gps_lat, 0, 'f', 6);
        args << QString("-GPSLongitude=%1").arg(metadata.gps_lon, 0, 'f', 6);
        args << "-GPSLatitudeRef=" + QString(metadata.gps_lat >= 0 ? "N" : "S");
        args << "-GPSLongitudeRef=" + QString(metadata.gps_lon >= 0 ? "E" : "W");
    }
    
    // Technical metadata
    if (metadata.technical.overall_quality > 0.0) {
        QString jsonData = buildTechnicalJSON(metadata.technical);
        args << QString("-EXIF:UserComment=%1").arg(jsonData);
    }
    
    return args;
}

bool MetadataWriter::write(const QString& filePath, const PhotoMetadata& metadata) {
    QStringList args = buildMetadataArgs(metadata);
    args << filePath;
    
    return runExifTool(filePath, args);
}

bool MetadataWriter::createBackup(const QString& filePath) {
    if (!validateFilePath(filePath)) {
        return false;
    }
    
    QFileInfo info(filePath);
    QString backupPath = info.path() + "/" + info.baseName() + "_backup." + info.suffix();
    
    // Copy entire file (not just metadata)
    QFile source(filePath);
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    
    if (!source.copy(backupPath)) {
        qWarning() << "Failed to create backup:" << source.errorString();
        return false;
    }
    
    qDebug() << "Backup created:" << backupPath;
    return true;
}

bool MetadataWriter::restoreFromBackup(const QString& filePath) {
    QFileInfo info(filePath);
    QString backupPath = info.path() + "/" + info.baseName() + "_backup." + info.suffix();
    
    if (!QFileInfo::exists(backupPath)) {
        qWarning() << "Backup file does not exist:" << backupPath;
        return false;
    }
    
    // Remove current file and copy backup
    if (QFile::exists(filePath)) {
        if (!QFile::remove(filePath)) {
            qWarning() << "Failed to remove current file for restore";
            return false;
        }
    }
    
    if (!QFile::copy(backupPath, filePath)) {
        qWarning() << "Failed to restore from backup";
        return false;
    }
    
    qDebug() << "Restored from backup:" << backupPath;
    return true;
}

} // namespace PhotoGuru
