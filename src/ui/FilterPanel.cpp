#include "FilterPanel.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>

namespace PhotoGuru {

bool FilterCriteria::matches(const PhotoMetadata& photo) const {
    // Quality filters
    if (photo.technical.overall_quality < minQuality) return false;
    if (photo.technical.sharpness_score < minSharpness) return false;
    if (photo.technical.aesthetic_score < minAesthetic) return false;
    
    // Content filters
    if (onlyWithFaces && photo.face_count == 0) return false;
    if (onlyBestInBurst && !photo.technical.is_best_in_burst) return false;
    if (excludeDuplicates && !photo.technical.duplicate_group.isEmpty()) return false;
    if (excludeBlurry && photo.technical.blur_detected) return false;
    
    // GPS filter
    if (onlyWithGPS && (photo.gps_lat == 0.0 || photo.gps_lon == 0.0)) return false;
    
    // Category filter
    if (!categories.isEmpty() && !categories.contains(photo.llm_category)) return false;
    if (!scenes.isEmpty() && !scenes.contains(photo.llm_scene)) return false;
    
    // Date range
    if (startDate.isValid() && photo.datetime_original < startDate) return false;
    if (endDate.isValid() && photo.datetime_original > endDate) return false;
    
    return true;
}

FilterPanel::FilterPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void FilterPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    
    // Quality filters group
    QGroupBox* qualityGroup = new QGroupBox("Quality Filters", this);
    QFormLayout* qualityLayout = new QFormLayout(qualityGroup);
    
    m_qualityLabel = new QLabel("0", this);
    m_qualitySlider = new QSlider(Qt::Horizontal, this);
    m_qualitySlider->setRange(0, 100);
    m_qualitySlider->setValue(0);
    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_qualityLabel->setText(QString::number(value));
        onFilterChanged();
    });
    QHBoxLayout* qualityH = new QHBoxLayout();
    qualityH->addWidget(m_qualitySlider);
    qualityH->addWidget(m_qualityLabel);
    qualityLayout->addRow("Min Overall:", qualityH);
    
    m_sharpnessLabel = new QLabel("0", this);
    m_sharpnessSlider = new QSlider(Qt::Horizontal, this);
    m_sharpnessSlider->setRange(0, 100);
    m_sharpnessSlider->setValue(0);
    connect(m_sharpnessSlider, &QSlider::valueChanged, this, [this](int value) {
        m_sharpnessLabel->setText(QString::number(value));
        onFilterChanged();
    });
    QHBoxLayout* sharpnessH = new QHBoxLayout();
    sharpnessH->addWidget(m_sharpnessSlider);
    sharpnessH->addWidget(m_sharpnessLabel);
    qualityLayout->addRow("Min Sharpness:", sharpnessH);
    
    m_aestheticLabel = new QLabel("0", this);
    m_aestheticSlider = new QSlider(Qt::Horizontal, this);
    m_aestheticSlider->setRange(0, 100);
    m_aestheticSlider->setValue(0);
    connect(m_aestheticSlider, &QSlider::valueChanged, this, [this](int value) {
        m_aestheticLabel->setText(QString::number(value));
        onFilterChanged();
    });
    QHBoxLayout* aestheticH = new QHBoxLayout();
    aestheticH->addWidget(m_aestheticSlider);
    aestheticH->addWidget(m_aestheticLabel);
    qualityLayout->addRow("Min Aesthetic:", aestheticH);
    
    mainLayout->addWidget(qualityGroup);
    
    // Content filters group
    QGroupBox* contentGroup = new QGroupBox("Content Filters", this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentGroup);
    
    m_facesCheckbox = new QCheckBox("Only photos with faces", this);
    connect(m_facesCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    contentLayout->addWidget(m_facesCheckbox);
    
    m_bestBurstCheckbox = new QCheckBox("Only best in burst", this);
    connect(m_bestBurstCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    contentLayout->addWidget(m_bestBurstCheckbox);
    
    m_noDuplicatesCheckbox = new QCheckBox("Exclude duplicates", this);
    connect(m_noDuplicatesCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    contentLayout->addWidget(m_noDuplicatesCheckbox);
    
    m_noBlurCheckbox = new QCheckBox("Exclude blurry photos", this);
    connect(m_noBlurCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    contentLayout->addWidget(m_noBlurCheckbox);
    
    m_gpsCheckbox = new QCheckBox("Only photos with GPS", this);
    connect(m_gpsCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    contentLayout->addWidget(m_gpsCheckbox);
    
    mainLayout->addWidget(contentGroup);
    
    // Category filters
    QGroupBox* categoryGroup = new QGroupBox("Category Filters", this);
    QFormLayout* categoryLayout = new QFormLayout(categoryGroup);
    
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem("All Categories", "");
    m_categoryCombo->addItem("People", "people");
    m_categoryCombo->addItem("Landscape", "landscape");
    m_categoryCombo->addItem("Architecture", "architecture");
    m_categoryCombo->addItem("Food", "food");
    m_categoryCombo->addItem("Technology", "technology");
    m_categoryCombo->addItem("Event", "event");
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterPanel::onFilterChanged);
    categoryLayout->addRow("Category:", m_categoryCombo);
    
    m_sceneCombo = new QComboBox(this);
    m_sceneCombo->addItem("All Scenes", "");
    m_sceneCombo->addItem("Indoor", "indoor");
    m_sceneCombo->addItem("Outdoor", "outdoor");
    m_sceneCombo->addItem("Urban", "urban");
    m_sceneCombo->addItem("Nature", "nature");
    m_sceneCombo->addItem("Street", "street");
    connect(m_sceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterPanel::onFilterChanged);
    categoryLayout->addRow("Scene:", m_sceneCombo);
    
    mainLayout->addWidget(categoryGroup);
    
    // Reset button
    QPushButton* resetBtn = new QPushButton("Reset Filters", this);
    resetBtn->setStyleSheet(R"(
        QPushButton {
            padding: 8px;
            background: #333;
            border: none;
            border-radius: 4px;
        }
        QPushButton:hover { background: #444; }
    )");
    connect(resetBtn, &QPushButton::clicked, this, &FilterPanel::reset);
    mainLayout->addWidget(resetBtn);
    
    mainLayout->addStretch();
}

FilterCriteria FilterPanel::getCriteria() const {
    FilterCriteria criteria;
    
    criteria.minQuality = m_qualitySlider->value() / 100.0;
    criteria.minSharpness = m_sharpnessSlider->value() / 100.0;
    criteria.minAesthetic = m_aestheticSlider->value() / 100.0;
    
    criteria.onlyWithFaces = m_facesCheckbox->isChecked();
    criteria.onlyBestInBurst = m_bestBurstCheckbox->isChecked();
    criteria.excludeDuplicates = m_noDuplicatesCheckbox->isChecked();
    criteria.excludeBlurry = m_noBlurCheckbox->isChecked();
    criteria.onlyWithGPS = m_gpsCheckbox->isChecked();
    
    QString category = m_categoryCombo->currentData().toString();
    if (!category.isEmpty()) {
        criteria.categories << category;
    }
    
    QString scene = m_sceneCombo->currentData().toString();
    if (!scene.isEmpty()) {
        criteria.scenes << scene;
    }
    
    return criteria;
}

void FilterPanel::reset() {
    m_qualitySlider->setValue(0);
    m_sharpnessSlider->setValue(0);
    m_aestheticSlider->setValue(0);
    
    m_facesCheckbox->setChecked(false);
    m_bestBurstCheckbox->setChecked(false);
    m_noDuplicatesCheckbox->setChecked(false);
    m_noBlurCheckbox->setChecked(false);
    m_gpsCheckbox->setChecked(false);
    
    m_categoryCombo->setCurrentIndex(0);
    m_sceneCombo->setCurrentIndex(0);
    
    onFilterChanged();
}

void FilterPanel::onFilterChanged() {
    emit filterChanged(getCriteria());
}

} // namespace PhotoGuru
