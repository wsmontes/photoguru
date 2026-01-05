#include "MetadataPanel.h"
#include "PhotoMetadata.h"
#include "NotificationManager.h"
#include <QScrollArea>
#include <QFileInfo>
#include <QDateTime>
#include <QHBoxLayout>

namespace PhotoGuru {

MetadataPanel::MetadataPanel(QWidget* parent)
    : QWidget(parent)
    , m_isEditing(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setupUI();
}

void MetadataPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);
    
    // Edit button at top
    QHBoxLayout* topLayout = new QHBoxLayout();
    m_editButton = new QPushButton("Edit Metadata", this);
    m_editButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background: #1f91ff;
            border: none;
            border-radius: 4px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover { background: #1a7dd9; }
        QPushButton:disabled { background: #555; color: #999; }
    )");
    connect(m_editButton, &QPushButton::clicked, this, [this]() {
        setEditable(true);
    });
    topLayout->addWidget(m_editButton);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);
    
    // Scroll area for content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(12);
    
    // File info section
    QGroupBox* fileGroup = new QGroupBox("File Info");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    m_filenameLabel = new QLabel("No image loaded");
    m_filenameLabel->setWordWrap(true);
    m_filenameLabel->setStyleSheet("color: #aaa;");
    fileLayout->addWidget(m_filenameLabel);
    contentLayout->addWidget(fileGroup);
    
    // Rating section
    QGroupBox* ratingGroup = new QGroupBox("Rating");
    QVBoxLayout* ratingLayout = new QVBoxLayout(ratingGroup);
    
    m_ratingLabel = new QLabel("☆☆☆☆☆ (0/5)");
    m_ratingLabel->setStyleSheet("font-size: 18px; color: #1f91ff;");
    ratingLayout->addWidget(m_ratingLabel);
    
    m_ratingSlider = new QSlider(Qt::Horizontal, this);
    m_ratingSlider->setRange(0, 5);
    m_ratingSlider->setValue(0);
    m_ratingSlider->setEnabled(false);
    connect(m_ratingSlider, &QSlider::valueChanged, this, &MetadataPanel::updateRatingDisplay);
    ratingLayout->addWidget(m_ratingSlider);
    
    contentLayout->addWidget(ratingGroup);
    
    // Title section
    QGroupBox* titleGroup = new QGroupBox("Title");
    QVBoxLayout* titleLayout = new QVBoxLayout(titleGroup);
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Enter title...");
    m_titleEdit->setReadOnly(true);
    titleLayout->addWidget(m_titleEdit);
    contentLayout->addWidget(titleGroup);
    
    // Description section
    QGroupBox* descGroup = new QGroupBox("Description");
    QVBoxLayout* descLayout = new QVBoxLayout(descGroup);
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText("Enter description...");
    m_descriptionEdit->setMaximumHeight(100);
    m_descriptionEdit->setReadOnly(true);
    descLayout->addWidget(m_descriptionEdit);
    contentLayout->addWidget(descGroup);
    
    // Keywords section
    QGroupBox* keywordsGroup = new QGroupBox("Keywords");
    QVBoxLayout* keywordsLayout = new QVBoxLayout(keywordsGroup);
    m_keywordsEdit = new QLineEdit(this);
    m_keywordsEdit->setPlaceholderText("Comma-separated keywords...");
    m_keywordsEdit->setReadOnly(true);
    keywordsLayout->addWidget(m_keywordsEdit);
    contentLayout->addWidget(keywordsGroup);
    
    // Category section
    QGroupBox* categoryGroup = new QGroupBox("Category");
    QVBoxLayout* categoryLayout = new QVBoxLayout(categoryGroup);
    m_categoryEdit = new QLineEdit(this);
    m_categoryEdit->setPlaceholderText("e.g., landscape, portrait, architecture...");
    m_categoryEdit->setReadOnly(true);
    categoryLayout->addWidget(m_categoryEdit);
    contentLayout->addWidget(categoryGroup);
    
    // Location section
    QGroupBox* locationGroup = new QGroupBox("Location");
    QVBoxLayout* locationLayout = new QVBoxLayout(locationGroup);
    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText("City, State, Country");
    m_locationEdit->setReadOnly(true);
    locationLayout->addWidget(m_locationEdit);
    contentLayout->addWidget(locationGroup);
    
    // EXIF section (read-only)
    QGroupBox* exifGroup = new QGroupBox("Camera Settings (Read-Only)");
    QVBoxLayout* exifLayout = new QVBoxLayout(exifGroup);
    m_exifLabel = new QLabel("-");
    m_exifLabel->setWordWrap(true);
    m_exifLabel->setStyleSheet("color: #aaa;");
    exifLayout->addWidget(m_exifLabel);
    contentLayout->addWidget(exifGroup);
    
    // Technical Analysis section (read-only)
    QGroupBox* techGroup = new QGroupBox("Quality Analysis (Read-Only)");
    QVBoxLayout* techLayout = new QVBoxLayout(techGroup);
    m_technicalLabel = new QLabel("-");
    m_technicalLabel->setWordWrap(true);
    m_technicalLabel->setStyleSheet("color: #aaa;");
    techLayout->addWidget(m_technicalLabel);
    contentLayout->addWidget(techGroup);
    
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
    
    // Save/Cancel buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_saveButton = new QPushButton("Save Changes", this);
    m_saveButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background: #28a745;
            border: none;
            border-radius: 4px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover { background: #218838; }
    )");
    m_saveButton->setVisible(false);
    connect(m_saveButton, &QPushButton::clicked, this, &MetadataPanel::saveMetadata);
    buttonLayout->addWidget(m_saveButton);
    
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background: #6c757d;
            border: none;
            border-radius: 4px;
            color: white;
        }
        QPushButton:hover { background: #5a6268; }
    )");
    m_cancelButton->setVisible(false);
    connect(m_cancelButton, &QPushButton::clicked, this, &MetadataPanel::cancelEdit);
    buttonLayout->addWidget(m_cancelButton);
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
}

void MetadataPanel::loadMetadata(const QString& filepath) {
    qDebug() << "[MetadataPanel] loadMetadata called for:" << filepath;
    qDebug() << "[MetadataPanel] Current filepath:" << m_currentFilepath;
    
    m_currentFilepath = filepath;
    
    auto metaOpt = MetadataReader::instance().read(filepath);
    
    if (!metaOpt) {
        clear();
        m_filenameLabel->setText(QFileInfo(filepath).fileName() + " (No metadata available)");
        m_editButton->setEnabled(false);
        return;
    }
    
    m_currentMetadata = *metaOpt;
    qDebug() << "[MetadataPanel] Loaded llm_description:" << m_currentMetadata.llm_description;
    displayMetadata(m_currentMetadata);
    m_editButton->setEnabled(true);
}

void MetadataPanel::displayMetadata(const PhotoMetadata& metadata) {
    // Filename
    QFileInfo info(metadata.filepath);
    m_filenameLabel->setText(QString("<b>%1</b><br/><span style='color:#888;'>%2</span>")
        .arg(info.fileName())
        .arg(info.path()));
    
    // Rating
    m_ratingSlider->setValue(metadata.rating);
    updateRatingDisplay(metadata.rating);
    
    // Editable fields
    m_titleEdit->setText(metadata.llm_title);
    m_descriptionEdit->setText(metadata.llm_description);
    m_keywordsEdit->setText(metadata.llm_keywords.join(", "));
    m_categoryEdit->setText(metadata.llm_category);
    m_locationEdit->setText(metadata.location_name);
    
    // EXIF (read-only)
    m_exifLabel->setText(formatExifInfo(metadata));
    
    // Technical (read-only)
    m_technicalLabel->setText(formatTechnicalInfo(metadata.technical));
}

QString MetadataPanel::formatExifInfo(const PhotoMetadata& meta) {
    QStringList parts;
    
    if (!meta.camera_make.isEmpty() || !meta.camera_model.isEmpty()) {
        parts << QString("<b>Camera:</b> %1 %2")
            .arg(meta.camera_make)
            .arg(meta.camera_model);
    }
    
    if (meta.aperture > 0) {
        parts << QString("<b>Aperture:</b> f/%1").arg(meta.aperture, 0, 'f', 1);
    }
    
    if (meta.shutter_speed > 0) {
        parts << QString("<b>Shutter:</b> 1/%1s").arg(static_cast<int>(1.0 / meta.shutter_speed));
    }
    
    if (meta.iso > 0) {
        parts << QString("<b>ISO:</b> %1").arg(meta.iso);
    }
    
    if (meta.focal_length > 0) {
        parts << QString("<b>Focal Length:</b> %1mm").arg(meta.focal_length, 0, 'f', 0);
    }
    
    if (meta.datetime_original.isValid()) {
        parts << QString("<b>Date:</b> %1")
            .arg(meta.datetime_original.toString("yyyy-MM-dd HH:mm:ss"));
    }
    
    return parts.isEmpty() ? "No EXIF data" : parts.join("<br/>");
}

QString MetadataPanel::formatTechnicalInfo(const TechnicalMetadata& tech) {
    QStringList parts;
    
    if (tech.overall_quality > 0) {
        parts << QString("<b>Overall Quality:</b> %1%")
            .arg(static_cast<int>(tech.overall_quality * 100));
    }
    
    if (tech.sharpness_score > 0) {
        parts << QString("<b>Sharpness:</b> %1%")
            .arg(static_cast<int>(tech.sharpness_score * 100));
    }
    
    if (tech.aesthetic_score > 0) {
        parts << QString("<b>Aesthetic:</b> %1%")
            .arg(static_cast<int>(tech.aesthetic_score * 100));
    }
    
    if (tech.face_count > 0) {
        parts << QString("<b>Faces:</b> %1").arg(tech.face_count);
    }
    
    if (tech.blur_detected) {
        parts << "<b style='color:#ff6b6b;'>⚠ Blur detected</b>";
    }
    
    if (tech.is_best_in_burst) {
        parts << "<b style='color:#51cf66;'>✓ Best in burst</b>";
    }
    
    return parts.isEmpty() ? "No analysis data" : parts.join("<br/>");
}

QString MetadataPanel::formatAIAnalysis(const PhotoMetadata& meta) {
    QStringList parts;
    
    if (!meta.llm_title.isEmpty()) {
        parts << QString("<b>Title:</b> %1").arg(meta.llm_title);
    }
    
    if (!meta.llm_description.isEmpty()) {
        parts << QString("<b>Description:</b> %1").arg(meta.llm_description);
    }
    
    if (!meta.llm_keywords.isEmpty()) {
        parts << QString("<b>Keywords:</b> %1").arg(meta.llm_keywords.join(", "));
    }
    
    return parts.isEmpty() ? "Not analyzed yet" : parts.join("<br/>");
}

void MetadataPanel::setEditable(bool editable) {
    m_isEditing = editable;
    
    // Toggle edit mode for fields
    m_ratingSlider->setEnabled(editable);
    m_titleEdit->setReadOnly(!editable);
    m_descriptionEdit->setReadOnly(!editable);
    m_keywordsEdit->setReadOnly(!editable);
    m_categoryEdit->setReadOnly(!editable);
    m_locationEdit->setReadOnly(!editable);
    
    // Toggle buttons
    m_editButton->setVisible(!editable);
    m_saveButton->setVisible(editable);
    m_cancelButton->setVisible(editable);
    
    // Change background color to indicate edit mode
    QString editStyle = editable 
        ? "background: #2a2a2a; border: 1px solid #1f91ff;" 
        : "background: #1e1e1e;";
    
    m_titleEdit->setStyleSheet(editStyle);
    m_descriptionEdit->setStyleSheet(editStyle);
    m_keywordsEdit->setStyleSheet(editStyle);
    m_categoryEdit->setStyleSheet(editStyle);
    m_locationEdit->setStyleSheet(editStyle);
    
    emit editModeChanged(editable);
}

void MetadataPanel::saveMetadata() {
    if (m_currentFilepath.isEmpty()) {
        return;
    }
    
    qDebug() << "[MetadataPanel] BEFORE SAVE - Description:" << m_descriptionEdit->toPlainText().trimmed();
    
    // Update current metadata with edited values
    m_currentMetadata.rating = m_ratingSlider->value();
    m_currentMetadata.llm_title = m_titleEdit->text().trimmed();
    m_currentMetadata.llm_description = m_descriptionEdit->toPlainText().trimmed();
    
    qDebug() << "[MetadataPanel] m_currentMetadata.llm_description:" << m_currentMetadata.llm_description;
    
    QString keywordsText = m_keywordsEdit->text().trimmed();
    if (!keywordsText.isEmpty()) {
        m_currentMetadata.llm_keywords = keywordsText.split(',', Qt::SkipEmptyParts);
        for (QString& kw : m_currentMetadata.llm_keywords) {
            kw = kw.trimmed();
        }
    } else {
        m_currentMetadata.llm_keywords.clear();
    }
    
    m_currentMetadata.llm_category = m_categoryEdit->text().trimmed();
    m_currentMetadata.location_name = m_locationEdit->text().trimmed();
    
    // Save using MetadataWriter
    bool success = MetadataWriter::instance().write(m_currentFilepath, m_currentMetadata);
    
    qDebug() << "[MetadataPanel] Write result:" << success;
    
    if (success) {
        setEditable(false);
        qDebug() << "[MetadataPanel] AFTER SAVE - m_currentMetadata.llm_description:" << m_currentMetadata.llm_description;
        // Don't reload - keep the values we just saved in m_currentMetadata
        // Just emit the signal so other components can update
        emit metadataChanged(m_currentFilepath);
        NotificationManager::instance().showSuccess("Metadata saved successfully! Changes written to XMP/IPTC fields.");
    } else {
        NotificationManager::instance().showError("Failed to save metadata. Make sure the file is writable and ExifTool is installed.");
    }
}

void MetadataPanel::cancelEdit() {
    // Reload original metadata
    displayMetadata(m_currentMetadata);
    setEditable(false);
}

void MetadataPanel::updateRatingDisplay(int rating) {
    QString stars;
    for (int i = 0; i < rating; ++i) {
        stars += "★";
    }
    for (int i = rating; i < 5; ++i) {
        stars += "☆";
    }
    m_ratingLabel->setText(QString("%1 (%2/5)").arg(stars).arg(rating));
}

void MetadataPanel::clear() {
    m_currentFilepath.clear();
    m_filenameLabel->setText("No image loaded");
    m_ratingSlider->setValue(0);
    m_titleEdit->clear();
    m_descriptionEdit->clear();
    m_keywordsEdit->clear();
    m_categoryEdit->clear();
    m_locationEdit->clear();
    m_exifLabel->setText("-");
    m_technicalLabel->setText("-");
    m_editButton->setEnabled(false);
    setEditable(false);
}

} // namespace PhotoGuru
