#pragma once

#include "PhotoMetadata.h"
#include <QString>
#include <QStringList>
#include <optional>

namespace PhotoGuru {

/**
 * @brief MetadataWriter handles writing/editing photo metadata using ExifTool
 * 
 * Supports writing to XMP, IPTC, and EXIF fields with full Lightroom compatibility.
 * All write operations are atomic and preserve existing metadata.
 */
class MetadataWriter {
public:
    static MetadataWriter& instance();
    
    // Write complete metadata structure
    bool write(const QString& filePath, const PhotoMetadata& metadata);
    
    // Update individual fields (more efficient for single changes)
    bool updateRating(const QString& filePath, int rating);
    bool updateTitle(const QString& filePath, const QString& title);
    bool updateDescription(const QString& filePath, const QString& description);
    bool updateKeywords(const QString& filePath, const QStringList& keywords);
    bool updateCategory(const QString& filePath, const QString& category);
    bool updateLocation(const QString& filePath, const QString& city, const QString& state, const QString& country);
    bool updateGPS(const QString& filePath, double lat, double lon);
    
    // Batch operations
    bool updateRatingBatch(const QStringList& filePaths, int rating);
    bool addKeywordsBatch(const QStringList& filePaths, const QStringList& keywords);
    bool removeKeywordsBatch(const QStringList& filePaths, const QStringList& keywords);
    
    // Technical metadata (stored in EXIF:UserComment in JSON format)
    bool writeTechnicalMetadata(const QString& filePath, const TechnicalMetadata& technical);
    
    // AI analysis results
    bool writeAIAnalysis(const QString& filePath, const QString& title, 
                        const QString& description, const QStringList& keywords,
                        const QString& category, const QString& scene, const QString& mood);
    
    // Backup/restore
    bool createBackup(const QString& filePath);
    bool restoreFromBackup(const QString& filePath);
    
    // Validation
    bool verifyExifToolAvailable() const;
    QString getExifToolVersion() const;
    
private:
    MetadataWriter() = default;
    ~MetadataWriter() = default;
    MetadataWriter(const MetadataWriter&) = delete;
    MetadataWriter& operator=(const MetadataWriter&) = delete;
    
    // Helper methods
    bool runExifTool(const QString& filePath, const QStringList& args, QString* output = nullptr);
    QString buildTechnicalJSON(const TechnicalMetadata& technical) const;
    QStringList buildMetadataArgs(const PhotoMetadata& metadata) const;
    bool validateFilePath(const QString& filePath) const;
    QString escapeForExifTool(const QString& value) const;
};

} // namespace PhotoGuru
