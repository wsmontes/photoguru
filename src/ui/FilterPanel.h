#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QComboBox>
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
    
    // Category filters
    QStringList categories;
    QStringList scenes;
    
    // Date range
    QDateTime startDate;
    QDateTime endDate;
    
    // Location
    bool onlyWithGPS = false;
    
    bool matches(const PhotoMetadata& photo) const;
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
    
    // Quality sliders
    QSlider* m_qualitySlider;
    QSlider* m_sharpnessSlider;
    QSlider* m_aestheticSlider;
    
    QLabel* m_qualityLabel;
    QLabel* m_sharpnessLabel;
    QLabel* m_aestheticLabel;
    
    // Content checkboxes
    QCheckBox* m_facesCheckbox;
    QCheckBox* m_bestBurstCheckbox;
    QCheckBox* m_noDuplicatesCheckbox;
    QCheckBox* m_noBlurCheckbox;
    QCheckBox* m_gpsCheckbox;
    
    // Category filters
    QComboBox* m_categoryCombo;
    QComboBox* m_sceneCombo;
};

} // namespace PhotoGuru
