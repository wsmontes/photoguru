#pragma once

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <vector>
#include <optional>

namespace PhotoGuru {

struct TechnicalMetadata {
    double sharpness_score = 0.0;
    double exposure_quality = 0.0;
    double aesthetic_score = 0.0;
    double overall_quality = 0.0;
    QString duplicate_group;
    QString burst_group;
    int burst_position = -1;
    bool is_best_in_burst = false;
    int face_count = 0;
    bool blur_detected = false;
    bool highlights_clipped = false;
    bool shadows_blocked = false;
    
    static TechnicalMetadata fromJson(const QJsonObject& json);
};

struct SemanticKeyData {
    QString key_id;
    QString role;  // "anchor", "gate", "link", "composite"
    QJsonObject metadata;
};

struct PhotoMetadata {
    // File info
    QString filepath;
    QString filename;
    
    // EXIF
    QDateTime datetime_original;
    QString camera_make;
    QString camera_model;
    double gps_lat = 0.0;
    double gps_lon = 0.0;
    QString location_name;
    double aperture = 0.0;
    double shutter_speed = 0.0;
    int iso = 0;
    double focal_length = 0.0;
    
    // AI Analysis (LLM)
    QString llm_title;
    QString llm_description;
    QStringList llm_keywords;
    QString llm_category;
    QString llm_scene;
    QString llm_mood;
    
    // Technical Analysis
    TechnicalMetadata technical;
    
    // Rating (from aesthetic score)
    int rating = 0;  // 1-5 stars
    
    // Face detection
    int face_count = 0;
    
    // SKP (Semantic Key Protocol)
    std::optional<SemanticKeyData> skp_image_key;
    std::vector<SemanticKeyData> skp_person_keys;
    QStringList skp_group_keys;
    QString skp_global_key;
    
    // Group context
    QString group_id;
    QJsonObject group_context;
    
    bool hasPhotoGuruMetadata() const {
        return !llm_title.isEmpty() || !llm_keywords.isEmpty();
    }
};

class MetadataReader {
public:
    static MetadataReader& instance();
    
    // Read all metadata from image file (via exiftool)
    std::optional<PhotoMetadata> read(const QString& filePath);
    
    // Quick check if file has PhotoGuru metadata
    bool hasPhotoGuruData(const QString& filePath);
    
    // Extract only technical metadata from UserComment
    std::optional<TechnicalMetadata> readTechnicalOnly(const QString& filePath);
    
private:
    MetadataReader() = default;
    ~MetadataReader() = default;
    MetadataReader(const MetadataReader&) = delete;
    MetadataReader& operator=(const MetadataReader&) = delete;
    
    QString runExifTool(const QString& filePath, const QStringList& args);
    PhotoMetadata parseExifToolOutput(const QString& output);
    TechnicalMetadata parseTechnicalData(const QString& userComment);
};

} // namespace PhotoGuru
