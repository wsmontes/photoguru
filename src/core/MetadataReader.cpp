#include "PhotoMetadata.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFileInfo>

namespace PhotoGuru {

MetadataReader& MetadataReader::instance() {
    static MetadataReader reader;
    return reader;
}

std::optional<PhotoMetadata> MetadataReader::read(const QString& filePath) {
    qDebug() << "MetadataReader::read called for:" << filePath;
    
    // Use exiftool to read all metadata
    QStringList args = {"-json", "-G", "-a", "-s", filePath};
    QString output = runExifTool(filePath, args);
    
    if (output.isEmpty()) {
        qWarning() << "No metadata output for:" << filePath;
        return std::nullopt;
    }
    
    return parseExifToolOutput(output);
}

bool MetadataReader::hasPhotoGuruData(const QString& filePath) {
    QStringList args = {"-XMP:CreatorTool", filePath};
    QString output = runExifTool(filePath, args);
    return output.contains("PhotoGuru");
}

std::optional<TechnicalMetadata> MetadataReader::readTechnicalOnly(const QString& filePath) {
    QStringList args = {"-EXIF:UserComment", filePath};
    QString output = runExifTool(filePath, args);
    
    if (output.isEmpty()) {
        return std::nullopt;
    }
    
    return parseTechnicalData(output);
}

QString MetadataReader::runExifTool(const QString& filePath, const QStringList& args) {
    QProcess process;
    
    // Use full path to exiftool since GUI apps don't inherit shell PATH
    QString exifToolPath = "/opt/homebrew/bin/exiftool";
    QFileInfo exifToolInfo(exifToolPath);
    
    if (!exifToolInfo.exists()) {
        // Fallback to searching in common locations
        QStringList locations = {
            "/opt/homebrew/bin/exiftool",
            "/usr/local/bin/exiftool",
            "/usr/bin/exiftool",
            "exiftool"  // Try PATH as last resort
        };
        
        exifToolPath = "exiftool";  // Default
        for (const QString& loc : locations) {
            if (QFileInfo(loc).exists()) {
                exifToolPath = loc;
                break;
            }
        }
    }
    
    // Debug: print the command being executed
    qDebug() << "Running" << exifToolPath << "with args:" << args;
    
    process.start(exifToolPath, args);
    
    if (!process.waitForStarted(1000)) {
        qWarning() << "ExifTool failed to start at:" << exifToolPath;
        return QString();
    }
    
    if (!process.waitForFinished(10000)) {
        qWarning() << "ExifTool timeout for file:" << filePath;
        process.kill();
        return QString();
    }
    
    QString stdErr = QString::fromUtf8(process.readAllStandardError());
    QString stdOut = QString::fromUtf8(process.readAllStandardOutput());
    
    if (process.exitCode() != 0) {
        qWarning() << "ExifTool error for" << filePath << ":" << stdErr;
        return QString();
    }
    
    // Debug: show what we got
    if (stdOut.isEmpty()) {
        qWarning() << "ExifTool returned empty output for:" << filePath;
    } else {
        qDebug() << "ExifTool output length:" << stdOut.length() << "bytes";
    }
    
    return stdOut;
}

PhotoMetadata MetadataReader::parseExifToolOutput(const QString& output) {
    PhotoMetadata meta;
    
    qDebug() << "Parsing exiftool output...";
    
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (!doc.isArray() || doc.array().isEmpty()) {
        qWarning() << "ExifTool output is not a valid JSON array";
        qDebug() << "Output was:" << output.left(500);
        return meta;
    }
    
    QJsonObject obj = doc.array()[0].toObject();
    qDebug() << "Found" << obj.keys().count() << "metadata fields";
    
    // File info
    meta.filepath = obj["SourceFile"].toString();
    meta.filename = obj.value("File:FileName").toString(obj["FileName"].toString());
    
    // EXIF data - support both with and without group prefixes
    QString dateStr = obj.value("EXIF:DateTimeOriginal").toString(obj["DateTimeOriginal"].toString());
    if (!dateStr.isEmpty()) {
        meta.datetime_original = QDateTime::fromString(dateStr, "yyyy:MM:dd hh:mm:ss");
    }
    
    meta.camera_make = obj.value("EXIF:Make").toString(obj["Make"].toString());
    meta.camera_model = obj.value("EXIF:Model").toString(obj["Model"].toString());
    meta.aperture = obj.value("EXIF:FNumber").toDouble(obj["FNumber"].toDouble());
    meta.shutter_speed = obj.value("EXIF:ShutterSpeedValue").toDouble(obj["ShutterSpeedValue"].toDouble());
    meta.iso = obj.value("EXIF:ISO").toInt(obj["ISO"].toInt());
    meta.focal_length = obj.value("EXIF:FocalLength").toDouble(obj["FocalLength"].toDouble());
    
    // GPS
    meta.gps_lat = obj.value("EXIF:GPSLatitude").toDouble(obj["GPSLatitude"].toDouble());
    meta.gps_lon = obj.value("EXIF:GPSLongitude").toDouble(obj["GPSLongitude"].toDouble());
    
    // PhotoGuru AI data - ImageDescription contains the AI-generated description
    meta.llm_title = obj.value("XMP:Title").toString(obj["Title"].toString());
    meta.llm_description = obj.value("EXIF:ImageDescription").toString(
        obj.value("XMP:Description").toString(obj["ImageDescription"].toString()));
    
    QJsonArray keywords = obj.value("XMP:Subject").toArray(obj["Subject"].toArray());
    for (const QJsonValue& kw : keywords) {
        meta.llm_keywords << kw.toString();
    }
    
    meta.llm_category = obj.value("IPTC:Category").toString(obj["Category"].toString());
    meta.llm_scene = obj.value("XMP:LocationShown").toString(obj["LocationShown"].toString());
    
    // Rating
    meta.rating = obj.value("XMP:Rating").toInt(obj["Rating"].toInt());
    
    // Location
    meta.location_name = obj.value("IPTC:City").toString(obj["City"].toString());
    QString state = obj.value("IPTC:Province-State").toString(obj["Province-State"].toString());
    QString country = obj.value("IPTC:Country-PrimaryLocationName").toString(obj["Country-PrimaryLocationName"].toString());
    
    if (!state.isEmpty()) {
        meta.location_name += ", " + state;
    }
    if (!country.isEmpty()) {
        if (!meta.location_name.isEmpty()) meta.location_name += ", ";
        meta.location_name += country;
    }
    
    // Technical metadata from UserComment
    QString userComment = obj.value("EXIF:UserComment").toString(obj["UserComment"].toString());
    qDebug() << "UserComment field:" << userComment;
    
    if (userComment.startsWith("PhotoGuru:")) {
        qDebug() << "Found PhotoGuru metadata, parsing...";
        meta.technical = parseTechnicalData(userComment);
        qDebug() << "After parsing - aesthetic_score:" << meta.technical.aesthetic_score
                 << "overall_quality:" << meta.technical.overall_quality
                 << "sharpness:" << meta.technical.sharpness_score;
    } else {
        qDebug() << "No PhotoGuru metadata found in UserComment";
    }
    
    return meta;
}

TechnicalMetadata TechnicalMetadata::fromJson(const QJsonObject& json) {
    TechnicalMetadata tech;
    
    qDebug() << "JSON keys:" << json.keys();
    qDebug() << "JSON values - sharp:" << json["sharp"] 
             << "expo:" << json["expo"]
             << "aesth:" << json["aesth"]
             << "qual:" << json["qual"];
    
    tech.sharpness_score = json["sharp"].toDouble();
    tech.exposure_quality = json["expo"].toDouble();
    tech.aesthetic_score = json["aesth"].toDouble();
    tech.overall_quality = json["qual"].toDouble();
    
    qDebug() << "Converted to doubles - sharp:" << tech.sharpness_score
             << "expo:" << tech.exposure_quality
             << "aesth:" << tech.aesthetic_score
             << "qual:" << tech.overall_quality;
    
    // Handle null values properly
    tech.duplicate_group = json["dup"].isNull() ? QString() : json["dup"].toString();
    tech.burst_group = json["burst"].isNull() ? QString() : json["burst"].toString();
    tech.burst_position = json["burst_pos"].isNull() ? -1 : json["burst_pos"].toInt();
    tech.is_best_in_burst = json["burst_best"].toBool();
    tech.face_count = json["faces"].toInt();
    
    qDebug() << "Parsed technical metadata - quality:" << tech.overall_quality 
             << "sharpness:" << tech.sharpness_score 
             << "faces:" << tech.face_count;
    
    return tech;
}

TechnicalMetadata MetadataReader::parseTechnicalData(const QString& userComment) {
    TechnicalMetadata tech;
    
    qDebug() << "Parsing UserComment:" << userComment.left(100);
    
    // Extract JSON from "PhotoGuru:{...}" format
    int jsonStart = userComment.indexOf('{');
    if (jsonStart < 0) {
        qWarning() << "No '{' found in UserComment";
        return tech;
    }
    
    QString jsonStr = userComment.mid(jsonStart);
    qDebug() << "JSON string:" << jsonStr;
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    
    if (!doc.isObject()) {
        qWarning() << "UserComment JSON is not an object";
        return tech;
    }
    
    qDebug() << "Successfully parsed UserComment JSON";
    return TechnicalMetadata::fromJson(doc.object());
}

} // namespace PhotoGuru
