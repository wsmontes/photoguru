#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include "core/PhotoMetadata.h"

namespace PhotoGuru {

struct FilterCriteria {
    // Quality filters
    double minQuality = 0.0;
    double minSharpness = 0.0;
    double minAesthetic = 0.0;
    
    // Content filters
    bool onlyWithFaces = false;
    bool onlyBestInBurst = false;
    bool excludeDuplicates = false;
    bool excludeBlurry = false;
    
    // Rating filter
    int minRating = 0;  // 0-5 stars
    int maxRating = 5;
    
    // Camera/Lens filters
    QStringList cameras;  // Camera make/model
    QStringList lenses;   // Lens model
    
    // Technical filters
    int minISO = 0;
    int maxISO = 102400;
    double minAperture = 0.0;  // f-stop
    double maxAperture = 32.0;
    double minFocalLength = 0.0;
    double maxFocalLength = 1000.0;
    double minShutterSpeed = 0.0;
    double maxShutterSpeed = 10000.0;
    
    // Category filters
    QStringList categories;
    QStringList scenes;
    QStringList keywords;  // Filter by keywords/tags
    
    // Date range
    QDateTime startDate;
    QDateTime endDate;
    
    // Location
    bool onlyWithGPS = false;
    
    // Text search (searches in title, description, keywords, location, camera)
    QString searchText;
    bool searchCaseSensitive = false;
    
    bool matches(const PhotoMetadata& photo) const;
    bool matchesSearch(const PhotoMetadata& photo) const;
};

class FilterPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit FilterPanel(QWidget* parent = nullptr);
    
    FilterCriteria getCriteria() const;
    void reset();
    
signals:
    void filterChanged(const FilterCriteria& criteria);
    
private:
    void setupUI();
    void onFilterChanged();
    void onSearchTextChanged();  // New: handles debounced search
    
    // Search
    QLineEdit* m_searchEdit;
    QCheckBox* m_caseSensitiveCheckbox;
    QTimer* m_searchTimer;  // New: debounce timer
    
    // Quality sliders
    QSlider* m_qualitySlider;
    QSlider* m_sharpnessSlider;
    QSlider* m_aestheticSlider;
    
    QLabel* m_qualityLabel;
    QLabel* m_sharpnessLabel;
    QLabel* m_aestheticLabel;
    
    // Rating filter
    QSlider* m_minRatingSlider;
    QSlider* m_maxRatingSlider;
    QLabel* m_minRatingLabel;
    QLabel* m_maxRatingLabel;
    
    // Content checkboxes
    QCheckBox* m_facesCheckbox;
    QCheckBox* m_bestBurstCheckbox;
    QCheckBox* m_noDuplicatesCheckbox;
    QCheckBox* m_noBlurCheckbox;
    QCheckBox* m_gpsCheckbox;
    
    // Category filters
    QComboBox* m_categoryCombo;
    QComboBox* m_sceneCombo;
    
    // Camera/Lens filters
    QComboBox* m_cameraCombo;
    QComboBox* m_lensCombo;
    
    // Technical filters
    QLineEdit* m_minISOEdit;
    QLineEdit* m_maxISOEdit;
    QLineEdit* m_minApertureEdit;
    QLineEdit* m_maxApertureEdit;
    QLineEdit* m_minFocalLengthEdit;
    QLineEdit* m_maxFocalLengthEdit;
    
    // Keywords filter
    QLineEdit* m_keywordsEdit;
};

} // namespace PhotoGuru
