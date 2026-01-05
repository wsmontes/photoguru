#pragma once

#include "GoogleTakeoutParser.h"
#include <QString>
#include <QStringList>

namespace PhotoGuru {

class MetadataWriter;

/**
 * @brief Importer for Google Takeout photo exports
 * 
 * Coordinates the process of:
 * 1. Detecting Google Takeout directories
 * 2. Finding JSON sidecars for images
 * 3. Parsing metadata from JSONs
 * 4. Writing metadata back to image files (EXIF/IPTC/XMP)
 * 
 * This allows seamless import of Google Photos libraries with
 * all enriched metadata (descriptions, people, albums, locations)
 * preserved in standard formats.
 */
class GoogleTakeoutImporter {
public:
    struct ImportOptions {
        bool applyDescription = true;      // Write caption to EXIF/IPTC/XMP Description
        bool applyPeopleAsKeywords = true; // Add people names as Keywords
        bool applyAlbumsAsKeywords = true; // Add album names as Keywords
        bool applyLocation = true;         // Write GPS coordinates
        bool applyDateTime = true;         // Update capture date/time
        bool overwriteExisting = false;    // Overwrite existing metadata fields
        bool createBackup = true;          // Backup original files before writing
    };
    
    struct ImportResult {
        int totalImages = 0;               // Total images processed
        int withJson = 0;                  // Images with JSON sidecars found
        int metadataApplied = 0;           // Images where metadata was written
        int errors = 0;                    // Errors encountered
        QStringList errorMessages;         // Detailed error messages
        
        QString summary() const;
    };
    
    /**
     * @brief Import metadata from Google Takeout directory
     * 
     * Scans directory for images, finds corresponding JSONs,
     * and applies metadata to image files.
     * 
     * @param directoryPath Path to Google Takeout export directory
     * @param options Import options (what to apply, how)
     * @return ImportResult with statistics
     */
    static ImportResult importDirectory(const QString& directoryPath,
                                       const ImportOptions& options);
    
    /**
     * @brief Import metadata for a single image
     * 
     * Finds JSON sidecar (if exists) and applies metadata to image file.
     * 
     * @param imagePath Path to image file
     * @param options Import options
     * @return true if metadata was successfully applied
     */
    static bool importSingleImage(const QString& imagePath,
                                  const ImportOptions& options);
    
    /**
     * @brief Apply Takeout metadata to image using MetadataWriter
     * 
     * Maps Google Takeout metadata to standard EXIF/IPTC/XMP fields:
     * - Description → EXIF:ImageDescription, IPTC:Caption-Abstract, XMP:Description
     * - People → IPTC:Keywords, XMP:PersonInImage, XMP:Subject
     * - Albums → IPTC:Keywords (prefixed with "Album:")
     * - Location → EXIF GPS tags, IPTC Location fields
     * - DateTime → EXIF:DateTimeOriginal
     * 
     * @param imagePath Path to image file
     * @param metadata Parsed Takeout metadata
     * @param options Import options
     * @return true if successful
     */
    static bool applyMetadataToImage(const QString& imagePath,
                                    const GoogleTakeoutParser::TakeoutMetadata& metadata,
                                    const ImportOptions& options);

private:
    // Helper to scan directory for images
    static QStringList findImagesInDirectory(const QString& directoryPath);
};

} // namespace PhotoGuru
