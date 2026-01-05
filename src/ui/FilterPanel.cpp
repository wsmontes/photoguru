#include "FilterPanel.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>

namespace PhotoGuru {

bool FilterCriteria::matchesSearch(const PhotoMetadata& photo) const {
    if (searchText.isEmpty()) return true;
    
    // Split search into multiple terms (space-separated)
    QStringList searchTerms = searchText.split(' ', Qt::SkipEmptyParts);
    if (searchTerms.isEmpty()) return true;
    
    // Helper to check if text contains ALL search terms (AND logic)
    auto containsAllTerms = [&](const QString& text) -> bool {
        if (text.isEmpty()) return false;
        QString target = searchCaseSensitive ? text : text.toLower();
        for (const QString& term : searchTerms) {
            QString searchTerm = searchCaseSensitive ? term : term.toLower();
            if (!target.contains(searchTerm)) {
                return false;  // If any term is missing, return false
            }
        }
        return true;  // All terms found
    };
    
    // Search in various metadata fields (OR logic between fields)
    // LLM-generated content (highest priority)
    if (containsAllTerms(photo.llm_title)) return true;
    if (containsAllTerms(photo.llm_description)) return true;
    if (containsAllTerms(photo.llm_category)) return true;
    if (containsAllTerms(photo.llm_scene)) return true;
    if (containsAllTerms(photo.llm_mood)) return true;
    
    // Keywords (check each keyword)
    for (const QString& kw : photo.llm_keywords) {
        if (containsAllTerms(kw)) return true;
    }
    
    // Location data
    if (containsAllTerms(photo.location_name)) return true;
    
    // Camera info
    QString cameraInfo = photo.camera_make + " " + photo.camera_model;
    if (containsAllTerms(cameraInfo)) return true;
    
    // Filename (without path)
    QString filename = photo.filename;
    if (filename.contains('/')) {
        filename = filename.mid(filename.lastIndexOf('/') + 1);
    }
    if (containsAllTerms(filename)) return true;
    
    return false;
}

bool FilterCriteria::matches(const PhotoMetadata& photo) const {
    // Text search first
    if (!matchesSearch(photo)) return false;
    
    // Quality filters
    if (photo.technical.overall_quality < minQuality) return false;
    if (photo.technical.sharpness_score < minSharpness) return false;
    if (photo.technical.aesthetic_score < minAesthetic) return false;
    
    // Rating filter
    if (photo.rating < minRating || photo.rating > maxRating) return false;
    
    // Content filters
    if (onlyWithFaces && photo.face_count == 0) return false;
    if (onlyBestInBurst && !photo.technical.is_best_in_burst) return false;
    if (excludeDuplicates && !photo.technical.duplicate_group.isEmpty()) return false;
    if (excludeBlurry && photo.technical.blur_detected) return false;
    
    // GPS filter
    if (onlyWithGPS && (photo.gps_lat == 0.0 || photo.gps_lon == 0.0)) return false;
    
    // Camera filter
    if (!cameras.isEmpty()) {
        QString fullCamera = photo.camera_make + " " + photo.camera_model;
        bool found = false;
        for (const QString& cam : cameras) {
            if (fullCamera.contains(cam, Qt::CaseInsensitive)) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    // ISO filter
    if (photo.iso > 0 && (photo.iso < minISO || photo.iso > maxISO)) return false;
    
    // Aperture filter
    if (photo.aperture > 0.0 && (photo.aperture < minAperture || photo.aperture > maxAperture)) return false;
    
    // Focal length filter
    if (photo.focal_length > 0.0 && (photo.focal_length < minFocalLength || photo.focal_length > maxFocalLength)) return false;
    
    // Keywords filter
    if (!keywords.isEmpty()) {
        bool found = false;
        for (const QString& filterKw : keywords) {
            for (const QString& photoKw : photo.llm_keywords) {
                if (photoKw.contains(filterKw, Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found) return false;
    }
    
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
    // Main layout for the entire widget
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    
    // Create scrollable area for filters
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Container widget inside scroll area
    QWidget* container = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    // Search bar at the top
    QGroupBox* searchGroup = new QGroupBox("Search & Filter", this);
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search in titles, descriptions, keywords, locations, cameras, filenames...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            padding: 10px;
            border: 2px solid #444;
            border-radius: 4px;
            background: #2b2b2b;
            color: #e0e0e0;
            font-size: 13px;
        }
        QLineEdit:focus {
            border-color: #51cf66;
        }
    )");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    searchLayout->addWidget(m_searchEdit);
    
    QHBoxLayout* searchOptionsLayout = new QHBoxLayout();
    m_caseSensitiveCheckbox = new QCheckBox("Case sensitive", this);
    m_caseSensitiveCheckbox->setToolTip("Make search case-sensitive");
    connect(m_caseSensitiveCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    searchOptionsLayout->addWidget(m_caseSensitiveCheckbox);
    searchOptionsLayout->addStretch();
    searchLayout->addLayout(searchOptionsLayout);
    
    // Quick search tips label
    QLabel* tipsLabel = new QLabel("💡 <i>Search works across all metadata fields</i>", this);
    tipsLabel->setStyleSheet("color: #888; font-size: 11px; padding: 2px 0px;");
    tipsLabel->setWordWrap(true);
    searchLayout->addWidget(tipsLabel);
    
    mainLayout->addWidget(searchGroup);
    
    // Quick filters (collapsed by default for common operations)
    QGroupBox* quickGroup = new QGroupBox("Quick Filters", this);
    QVBoxLayout* quickLayout = new QVBoxLayout(quickGroup);
    quickLayout->setSpacing(4);
    
    m_facesCheckbox = new QCheckBox("📷 Has faces", this);
    m_facesCheckbox->setToolTip("Only show photos with detected faces");
    connect(m_facesCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    quickLayout->addWidget(m_facesCheckbox);
    
    m_bestBurstCheckbox = new QCheckBox("⭐ Best in burst", this);
    m_bestBurstCheckbox->setToolTip("Show only the best photo from burst sequences");
    connect(m_bestBurstCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    quickLayout->addWidget(m_bestBurstCheckbox);
    
    m_noDuplicatesCheckbox = new QCheckBox("🚫 No duplicates", this);
    m_noDuplicatesCheckbox->setToolTip("Exclude duplicate/similar photos");
    connect(m_noDuplicatesCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    quickLayout->addWidget(m_noDuplicatesCheckbox);
    
    m_noBlurCheckbox = new QCheckBox("🎯 No blur", this);
    m_noBlurCheckbox->setToolTip("Exclude blurry/out-of-focus photos");
    connect(m_noBlurCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    quickLayout->addWidget(m_noBlurCheckbox);
    
    m_gpsCheckbox = new QCheckBox("📍 Has GPS", this);
    m_gpsCheckbox->setToolTip("Only show photos with GPS coordinates");
    connect(m_gpsCheckbox, &QCheckBox::stateChanged, this, &FilterPanel::onFilterChanged);
    quickLayout->addWidget(m_gpsCheckbox);
    
    mainLayout->addWidget(quickGroup);
    
    // Rating filter
    QGroupBox* ratingGroup = new QGroupBox("Rating", this);
    QFormLayout* ratingLayout = new QFormLayout(ratingGroup);
    
    m_minRatingLabel = new QLabel("0 ☆", this);
    m_minRatingSlider = new QSlider(Qt::Horizontal, this);
    m_minRatingSlider->setRange(0, 5);
    m_minRatingSlider->setValue(0);
    connect(m_minRatingSlider, &QSlider::valueChanged, this, [this](int value) {
        QString stars;
        for (int i = 0; i < value; ++i) stars += "★";
        for (int i = value; i < 5; ++i) stars += "☆";
        m_minRatingLabel->setText(QString::number(value) + " " + stars);
        if (value > m_maxRatingSlider->value()) {
            m_maxRatingSlider->setValue(value);
        }
        onFilterChanged();
    });
    QHBoxLayout* minRatingH = new QHBoxLayout();
    minRatingH->addWidget(m_minRatingSlider);
    minRatingH->addWidget(m_minRatingLabel);
    ratingLayout->addRow("Min Rating:", minRatingH);
    
    m_maxRatingLabel = new QLabel("5 ★", this);
    m_maxRatingSlider = new QSlider(Qt::Horizontal, this);
    m_maxRatingSlider->setRange(0, 5);
    m_maxRatingSlider->setValue(5);
    connect(m_maxRatingSlider, &QSlider::valueChanged, this, [this](int value) {
        QString stars;
        for (int i = 0; i < value; ++i) stars += "★";
        for (int i = value; i < 5; ++i) stars += "☆";
        m_maxRatingLabel->setText(QString::number(value) + " " + stars);
        if (value < m_minRatingSlider->value()) {
            m_minRatingSlider->setValue(value);
        }
        onFilterChanged();
    });
    QHBoxLayout* maxRatingH = new QHBoxLayout();
    maxRatingH->addWidget(m_maxRatingSlider);
    maxRatingH->addWidget(m_maxRatingLabel);
    ratingLayout->addRow("Max Rating:", maxRatingH);
    
    mainLayout->addWidget(ratingGroup);
    
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
    
    // Camera/Technical filters
    QGroupBox* technicalGroup = new QGroupBox("Camera & Technical", this);
    QFormLayout* technicalLayout = new QFormLayout(technicalGroup);
    
    m_cameraCombo = new QComboBox(this);
    m_cameraCombo->addItem("All Cameras", "");
    m_cameraCombo->addItem("Canon", "Canon");
    m_cameraCombo->addItem("Nikon", "Nikon");
    m_cameraCombo->addItem("Sony", "Sony");
    m_cameraCombo->addItem("Fujifilm", "Fuji");
    m_cameraCombo->addItem("Olympus", "Olympus");
    m_cameraCombo->addItem("Apple iPhone", "iPhone");
    connect(m_cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterPanel::onFilterChanged);
    technicalLayout->addRow("Camera:", m_cameraCombo);
    
    QHBoxLayout* isoLayout = new QHBoxLayout();
    m_minISOEdit = new QLineEdit("0", this);
    m_minISOEdit->setMaximumWidth(80);
    connect(m_minISOEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    isoLayout->addWidget(new QLabel("Min:", this));
    isoLayout->addWidget(m_minISOEdit);
    m_maxISOEdit = new QLineEdit("102400", this);
    m_maxISOEdit->setMaximumWidth(80);
    connect(m_maxISOEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    isoLayout->addWidget(new QLabel("Max:", this));
    isoLayout->addWidget(m_maxISOEdit);
    isoLayout->addStretch();
    technicalLayout->addRow("ISO:", isoLayout);
    
    QHBoxLayout* apertureLayout = new QHBoxLayout();
    m_minApertureEdit = new QLineEdit("0", this);
    m_minApertureEdit->setMaximumWidth(60);
    connect(m_minApertureEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    apertureLayout->addWidget(new QLabel("f/", this));
    apertureLayout->addWidget(m_minApertureEdit);
    m_maxApertureEdit = new QLineEdit("32", this);
    m_maxApertureEdit->setMaximumWidth(60);
    connect(m_maxApertureEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    apertureLayout->addWidget(new QLabel("- f/", this));
    apertureLayout->addWidget(m_maxApertureEdit);
    apertureLayout->addStretch();
    technicalLayout->addRow("Aperture:", apertureLayout);
    
    QHBoxLayout* focalLayout = new QHBoxLayout();
    m_minFocalLengthEdit = new QLineEdit("0", this);
    m_minFocalLengthEdit->setMaximumWidth(60);
    connect(m_minFocalLengthEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    focalLayout->addWidget(m_minFocalLengthEdit);
    focalLayout->addWidget(new QLabel("mm -", this));
    m_maxFocalLengthEdit = new QLineEdit("1000", this);
    m_maxFocalLengthEdit->setMaximumWidth(60);
    connect(m_maxFocalLengthEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    focalLayout->addWidget(m_maxFocalLengthEdit);
    focalLayout->addWidget(new QLabel("mm", this));
    focalLayout->addStretch();
    technicalLayout->addRow("Focal Length:", focalLayout);
    
    mainLayout->addWidget(technicalGroup);
    
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
    
    m_keywordsEdit = new QLineEdit(this);
    m_keywordsEdit->setPlaceholderText("Comma-separated keywords");
    m_keywordsEdit->setClearButtonEnabled(true);
    connect(m_keywordsEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
    categoryLayout->addRow("Keywords:", m_keywordsEdit);
    
    mainLayout->addWidget(categoryGroup);
    
    // Reset button
    QPushButton* resetBtn = new QPushButton("Reset All Filters", this);
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
    
    // Set container as scroll area widget
    scrollArea->setWidget(container);
    
    // Add scroll area to outer layout
    outerLayout->addWidget(scrollArea);
}

FilterCriteria FilterPanel::getCriteria() const {
    FilterCriteria criteria;
    
    // Search
    criteria.searchText = m_searchEdit->text().trimmed();
    criteria.searchCaseSensitive = m_caseSensitiveCheckbox->isChecked();
    
    // Quality
    criteria.minQuality = m_qualitySlider->value() / 100.0;
    criteria.minSharpness = m_sharpnessSlider->value() / 100.0;
    criteria.minAesthetic = m_aestheticSlider->value() / 100.0;
    
    // Rating
    criteria.minRating = m_minRatingSlider->value();
    criteria.maxRating = m_maxRatingSlider->value();
    
    // Content
    criteria.onlyWithFaces = m_facesCheckbox->isChecked();
    criteria.onlyBestInBurst = m_bestBurstCheckbox->isChecked();
    criteria.excludeDuplicates = m_noDuplicatesCheckbox->isChecked();
    criteria.excludeBlurry = m_noBlurCheckbox->isChecked();
    criteria.onlyWithGPS = m_gpsCheckbox->isChecked();
    
    // Camera
    QString camera = m_cameraCombo->currentData().toString();
    if (!camera.isEmpty()) {
        criteria.cameras << camera;
    }
    
    // Technical
    criteria.minISO = m_minISOEdit->text().toInt();
    criteria.maxISO = m_maxISOEdit->text().toInt();
    criteria.minAperture = m_minApertureEdit->text().toDouble();
    criteria.maxAperture = m_maxApertureEdit->text().toDouble();
    criteria.minFocalLength = m_minFocalLengthEdit->text().toDouble();
    criteria.maxFocalLength = m_maxFocalLengthEdit->text().toDouble();
    
    // Categories
    QString category = m_categoryCombo->currentData().toString();
    if (!category.isEmpty()) {
        criteria.categories << category;
    }
    
    QString scene = m_sceneCombo->currentData().toString();
    if (!scene.isEmpty()) {
        criteria.scenes << scene;
    }
    
    // Keywords
    QString keywordsText = m_keywordsEdit->text().trimmed();
    if (!keywordsText.isEmpty()) {
        criteria.keywords = keywordsText.split(',', Qt::SkipEmptyParts);
        for (QString& kw : criteria.keywords) {
            kw = kw.trimmed();
        }
    }
    
    return criteria;
}

void FilterPanel::reset() {
    m_searchEdit->clear();
    m_caseSensitiveCheckbox->setChecked(false);
    
    m_minRatingSlider->setValue(0);
    m_maxRatingSlider->setValue(5);
    
    m_qualitySlider->setValue(0);
    m_sharpnessSlider->setValue(0);
    m_aestheticSlider->setValue(0);
    
    m_facesCheckbox->setChecked(false);
    m_bestBurstCheckbox->setChecked(false);
    m_noDuplicatesCheckbox->setChecked(false);
    m_noBlurCheckbox->setChecked(false);
    m_gpsCheckbox->setChecked(false);
    
    m_cameraCombo->setCurrentIndex(0);
    m_categoryCombo->setCurrentIndex(0);
    m_sceneCombo->setCurrentIndex(0);
    
    m_minISOEdit->setText("0");
    m_maxISOEdit->setText("102400");
    m_minApertureEdit->setText("0");
    m_maxApertureEdit->setText("32");
    m_minFocalLengthEdit->setText("0");
    m_maxFocalLengthEdit->setText("1000");
    
    m_keywordsEdit->clear();
    
    onFilterChanged();
}

void FilterPanel::onFilterChanged() {
    emit filterChanged(getCriteria());
}

} // namespace PhotoGuru
