#include "GoogleTakeoutParser.h"
#include "Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QFileInfo>
#include <QTimeZone>

namespace PhotoGuru {

bool GoogleTakeoutParser::isGoogleTakeoutDirectory(const QString& directoryPath) {
    QDir dir(directoryPath);
    if (!dir.exists()) {
        return false;
    }
    
    // Check for .json files alongside images
    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
    if (jsonFiles.isEmpty()) {
        return false;
    }
    
    // Validate that at least one JSON has Google Takeout structure
    int validCount = 0;
    int checked = 0;
    
    for (const QString& jsonFile : jsonFiles) {
        if (checked >= 5) break; // Sample first 5 JSONs
        
        QString jsonPath = dir.filePath(jsonFile);
        QFile file(jsonPath);
        
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            continue;
        }
        
        QJsonObject json = doc.object();
        
        // Check for Google Takeout signature fields
        // Google Takeout JSONs typically have: photoTakenTime, creationTime, and other specific fields
        if (json.contains("photoTakenTime") || 
            json.contains("creationTime") ||
            json.contains("geoData") ||
            json.contains("googlePhotosOrigin")) {
            validCount++;
        }
        
        checked++;
    }
    
    // If at least 50% of sampled JSONs look like Takeout, consider it a Takeout directory
    bool isTakeout = validCount > 0 && (validCount * 2 >= checked);
    
    if (isTakeout) {
        LOG_INFO("GoogleTakeout", QString("Detected Google Takeout directory: %1 (%2/%3 valid JSONs)")
            .arg(directoryPath).arg(validCount).arg(checked));
    }
    
    return isTakeout;
}

QString GoogleTakeoutParser::findJsonForImage(const QString& imagePath) {
    // Google Takeout naming: image.jpg → image.jpg.json
    QString jsonPath = imagePath + ".json";
    
    if (QFileInfo::exists(jsonPath)) {
        return jsonPath;
    }
    
    // Some exports might use alternative naming
    QFileInfo info(imagePath);
    QString baseName = info.completeBaseName();
    QString dir = info.path();
    
    // Try: image.json (without extension duplication)
    QString altJsonPath = dir + "/" + baseName + ".json";
    if (QFileInfo::exists(altJsonPath)) {
        return altJsonPath;
    }
    
    return QString(); // Not found
}

GoogleTakeoutParser::TakeoutMetadata GoogleTakeoutParser::parseJsonFile(const QString& jsonPath) {
    TakeoutMetadata metadata;
    metadata.isValid = false;
    
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARNING("GoogleTakeout", "Failed to open JSON: " + jsonPath);
        return metadata;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        LOG_WARNING("GoogleTakeout", "Invalid JSON format: " + jsonPath);
        return metadata;
    }
    
    return parseJsonObject(doc.object());
}

GoogleTakeoutParser::TakeoutMetadata GoogleTakeoutParser::parseJsonObject(const QJsonObject& json) {
    TakeoutMetadata metadata;
    
    try {
        // Description/Caption
        if (json.contains("description")) {
            metadata.description = json["description"].toString();
        }
        
        // Album names (can be array or single string)
        if (json.contains("albumNames")) {
            QJsonValue albumsVal = json["albumNames"];
            if (albumsVal.isArray()) {
                for (const QJsonValue& val : albumsVal.toArray()) {
                    QString albumName = val.toString();
                    if (!albumName.isEmpty()) {
                        metadata.albumNames.append(albumName);
                    }
                }
            }
        }
        
        // Alternative field name
        if (json.contains("albumName")) {
            QString albumName = json["albumName"].toString();
            if (!albumName.isEmpty() && !metadata.albumNames.contains(albumName)) {
                metadata.albumNames.append(albumName);
            }
        }
        
        // People (face labels)
        metadata.people = parsePeople(json);
        
        // Location - final (Google adjusted or user set)
        if (json.contains("geoData")) {
            metadata.geoData = parseGeoData(json["geoData"].toObject());
        }
        
        // Location - original EXIF
        if (json.contains("geoDataExif")) {
            metadata.geoDataExif = parseGeoData(json["geoDataExif"].toObject());
        }
        
        // Location name (textual)
        if (json.contains("location")) {
            metadata.locationName = json["location"].toString();
        } else if (json.contains("address")) {
            metadata.locationName = json["address"].toString();
        }
        
        // Timestamps
        if (json.contains("photoTakenTime")) {
            metadata.photoTakenTime = parseTimestamp(json["photoTakenTime"].toObject());
        }
        
        if (json.contains("photoTakenTimeOriginal")) {
            metadata.photoTakenTimeOriginal = parseTimestamp(json["photoTakenTimeOriginal"].toObject());
        }
        
        if (json.contains("creationTime")) {
            metadata.creationTime = parseTimestamp(json["creationTime"].toObject());
        }
        
        if (json.contains("modificationTime")) {
            metadata.modificationTime = parseTimestamp(json["modificationTime"].toObject());
        }
        
        // Google Photos origin
        if (json.contains("googlePhotosOrigin")) {
            QJsonObject origin = json["googlePhotosOrigin"].toObject();
            
            if (origin.contains("mobileUpload")) {
                metadata.googlePhotosOrigin = "mobileUpload";
                QJsonObject mobileUpload = origin["mobileUpload"].toObject();
                if (mobileUpload.contains("deviceType")) {
                    metadata.deviceType = mobileUpload["deviceType"].toString();
                }
            }
        }
        
        metadata.isValid = true;
        
        LOG_DEBUG("GoogleTakeout", QString("Parsed metadata: desc=%1 chars, albums=%2, people=%3, hasGeo=%4")
            .arg(metadata.description.length())
            .arg(metadata.albumNames.size())
            .arg(metadata.people.size())
            .arg(metadata.geoData.has_value()));
        
    } catch (const std::exception& e) {
        LOG_ERROR("GoogleTakeout", QString("Exception parsing JSON: %1").arg(e.what()));
        metadata.isValid = false;
    }
    
    return metadata;
}

QDateTime GoogleTakeoutParser::parseTimestamp(const QJsonObject& timestampObj) {
    // Google Takeout timestamps have two formats:
    // 1. "timestamp": "1234567890" (epoch seconds as string)
    // 2. "formatted": "Jun 15, 2019, 10:30:45 AM UTC"
    
    if (timestampObj.contains("timestamp")) {
        QString timestampStr = timestampObj["timestamp"].toString();
        qint64 epochSeconds = timestampStr.toLongLong();
        
        if (epochSeconds > 0) {
            return QDateTime::fromSecsSinceEpoch(epochSeconds, QTimeZone::utc());
        }
    }
    
    // Fallback to formatted string (less reliable due to parsing)
    if (timestampObj.contains("formatted")) {
        QString formatted = timestampObj["formatted"].toString();
        // Try to parse formatted string (format varies)
        QDateTime dt = QDateTime::fromString(formatted, "MMM d, yyyy, h:mm:ss AP");
        if (dt.isValid()) {
            return dt;
        }
    }
    
    return QDateTime(); // Invalid
}

std::optional<QGeoCoordinate> GoogleTakeoutParser::parseGeoData(const QJsonObject& geoObj) {
    if (geoObj.isEmpty()) {
        return std::nullopt;
    }
    
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    bool hasAltitude = false;
    
    if (geoObj.contains("latitude")) {
        latitude = geoObj["latitude"].toDouble();
    }
    
    if (geoObj.contains("longitude")) {
        longitude = geoObj["longitude"].toDouble();
    }
    
    if (geoObj.contains("altitude")) {
        altitude = geoObj["altitude"].toDouble();
        hasAltitude = true;
    }
    
    // Validate coordinates
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        return std::nullopt;
    }
    
    QGeoCoordinate coord(latitude, longitude);
    if (hasAltitude) {
        coord.setAltitude(altitude);
    }
    
    return coord.isValid() ? std::optional<QGeoCoordinate>(coord) : std::nullopt;
}

QStringList GoogleTakeoutParser::parsePeople(const QJsonObject& json) {
    QStringList people;
    
    if (!json.contains("people")) {
        return people;
    }
    
    QJsonValue peopleVal = json["people"];
    if (!peopleVal.isArray()) {
        return people;
    }
    
    QJsonArray peopleArray = peopleVal.toArray();
    for (const QJsonValue& personVal : peopleArray) {
        if (personVal.isObject()) {
            QJsonObject personObj = personVal.toObject();
            if (personObj.contains("name")) {
                QString name = personObj["name"].toString();
                if (!name.isEmpty()) {
                    people.append(name);
                }
            }
        } else if (personVal.isString()) {
            // Some exports might have simple string array
            QString name = personVal.toString();
            if (!name.isEmpty()) {
                people.append(name);
            }
        }
    }
    
    return people;
}

} // namespace PhotoGuru
