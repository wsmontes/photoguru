#pragma once

#include <QString>
#include <QJsonObject>
#include <QDateTime>
#include <QGeoCoordinate>
#include <optional>
#include <vector>

namespace PhotoGuru {

/**
 * @brief Parser for Google Takeout metadata JSON files
 * 
 * Google Takeout exports photos with companion .json files containing
 * enriched metadata that Google Photos keeps in the cloud:
 * - Descriptions/captions
 * - Album names
 * - People (face labels)
 * - Location (GPS coordinates and place names)
 * - Timestamps (taken, created, modified)
 * - Google Photos origin info
 * 
 * This parser extracts these metadata and prepares them for writing
 * to EXIF/IPTC/XMP fields in the actual image files.
 * 
 * Reference: Google Takeout format as of 2026
 */
class GoogleTakeoutParser {
public:
    struct TakeoutMetadata {
        // Description/Caption
        QString description;
        
        // Album associations
        QStringList albumNames;
        
        // People recognized (face labels)
        QStringList people;
        
        // Location data
        std::optional<QGeoCoordinate> geoData;           // Final location (Google adjusted or user set)
        std::optional<QGeoCoordinate> geoDataExif;       // Original EXIF GPS
        QString locationName;                             // Place name (e.g., "Paris, France")
        
        // Timestamps
        QDateTime photoTakenTime;                         // Capture time (possibly edited)
        QDateTime photoTakenTimeOriginal;                 // Original capture time
        QDateTime creationTime;                           // Upload time to Google Photos
        QDateTime modificationTime;                       // Last modification time
        
        // Origin info
        QString googlePhotosOrigin;                       // e.g., "mobileUpload"
        QString deviceType;                               // e.g., "IOS_PHONE"
        
        // Validation
        bool isValid = false;                             // True if JSON was successfully parsed
        
        // Helper: Check if has useful metadata to apply
        bool hasMetadataToApply() const {
            return !description.isEmpty() || 
                   !albumNames.isEmpty() || 
                   !people.isEmpty() || 
                   geoData.has_value() ||
                   !locationName.isEmpty();
        }
    };
    
    /**
     * @brief Detect if a directory contains Google Takeout structure
     * 
     * Checks for presence of .json files alongside image files
     * and validates JSON structure to confirm it's Google Takeout format
     * 
     * @param directoryPath Path to directory to check
     * @return true if directory appears to be from Google Takeout
     */
    static bool isGoogleTakeoutDirectory(const QString& directoryPath);
    
    /**
     * @brief Parse Google Takeout JSON file
     * 
     * @param jsonPath Path to .json file (e.g., IMG_0001.jpg.json)
     * @return TakeoutMetadata struct with parsed data, or invalid if parsing failed
     */
    static TakeoutMetadata parseJsonFile(const QString& jsonPath);
    
    /**
     * @brief Find corresponding JSON file for an image
     * 
     * Google Takeout naming: IMG_0001.jpg → IMG_0001.jpg.json
     * 
     * @param imagePath Path to image file
     * @return Path to JSON file if exists, empty string otherwise
     */
    static QString findJsonForImage(const QString& imagePath);
    
    /**
     * @brief Parse JSON from QJsonObject (for testing or advanced use)
     */
    static TakeoutMetadata parseJsonObject(const QJsonObject& json);

private:
    // Helper to parse timestamp fields (handles both epoch and formatted strings)
    static QDateTime parseTimestamp(const QJsonObject& timestampObj);
    
    // Helper to parse geo coordinates
    static std::optional<QGeoCoordinate> parseGeoData(const QJsonObject& geoObj);
    
    // Helper to extract people names from array
    static QStringList parsePeople(const QJsonObject& json);
};

} // namespace PhotoGuru
