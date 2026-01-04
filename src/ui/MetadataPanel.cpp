#include "MetadataPanel.h"
#include "PhotoMetadata.h"
#include <QScrollArea>
#include <QFileInfo>
#include <QDateTime>

namespace PhotoGuru {

MetadataPanel::MetadataPanel(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setupUI();
}

void MetadataPanel::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);
    
    // Scroll area for content
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(10);
    
    // File info section
    QGroupBox* fileGroup = new QGroupBox("File Info");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    m_filenameLabel = new QLabel("No image loaded");
    m_filenameLabel->setWordWrap(true);
    fileLayout->addWidget(m_filenameLabel);
    contentLayout->addWidget(fileGroup);
    
    // Rating section
    QGroupBox* ratingGroup = new QGroupBox("Rating");
    QVBoxLayout* ratingLayout = new QVBoxLayout(ratingGroup);
    m_ratingLabel = new QLabel("★★★★★");
    m_ratingLabel->setStyleSheet("font-size: 16px; color: #1f91ff;");
    ratingLayout->addWidget(m_ratingLabel);
    contentLayout->addWidget(ratingGroup);
    
    // AI Analysis section
    QGroupBox* aiGroup = new QGroupBox("AI Analysis");
    QVBoxLayout* aiLayout = new QVBoxLayout(aiGroup);
    
    QLabel* titleLbl = new QLabel("Title:");
    titleLbl->setStyleSheet("font-weight: bold;");
    aiLayout->addWidget(titleLbl);
    m_titleEdit = new QTextEdit();
    m_titleEdit->setMaximumHeight(60);
    m_titleEdit->setReadOnly(true);
    aiLayout->addWidget(m_titleEdit);
    
    QLabel* descLbl = new QLabel("Description:");
    descLbl->setStyleSheet("font-weight: bold;");
    aiLayout->addWidget(descLbl);
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(100);
    m_descriptionEdit->setReadOnly(true);
    aiLayout->addWidget(m_descriptionEdit);
    
    QLabel* kwLbl = new QLabel("Keywords:");
    kwLbl->setStyleSheet("font-weight: bold;");
    aiLayout->addWidget(kwLbl);
    m_keywordsLabel = new QLabel("-");
    m_keywordsLabel->setWordWrap(true);
    aiLayout->addWidget(m_keywordsLabel);
    
    contentLayout->addWidget(aiGroup);
    
    // Location section
    QGroupBox* locationGroup = new QGroupBox("Location");
    QVBoxLayout* locationLayout = new QVBoxLayout(locationGroup);
    m_locationLabel = new QLabel("-");
    m_locationLabel->setWordWrap(true);
    locationLayout->addWidget(m_locationLabel);
    contentLayout->addWidget(locationGroup);
    
    // EXIF section
    QGroupBox* exifGroup = new QGroupBox("Camera Settings");
    QVBoxLayout* exifLayout = new QVBoxLayout(exifGroup);
    m_exifLabel = new QLabel("-");
    m_exifLabel->setWordWrap(true);
    exifLayout->addWidget(m_exifLabel);
    contentLayout->addWidget(exifGroup);
    
    // Technical Analysis section
    QGroupBox* techGroup = new QGroupBox("Technical Analysis");
    QVBoxLayout* techLayout = new QVBoxLayout(techGroup);
    m_technicalLabel = new QLabel("-");
    m_technicalLabel->setWordWrap(true);
    techLayout->addWidget(m_technicalLabel);
    contentLayout->addWidget(techGroup);
    
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void MetadataPanel::loadMetadata(const QString& filepath) {
    m_currentFilepath = filepath;
    
    auto metaOpt = MetadataReader::instance().read(filepath);
    
    if (!metaOpt) {
        clear();
        m_filenameLabel->setText(QFileInfo(filepath).fileName() + "\n(No metadata available)");
        return;
    }
    
    displayMetadata(*metaOpt);
}

void MetadataPanel::displayMetadata(const PhotoMetadata& metadata) {
    // Filename
    QFileInfo info(metadata.filepath);
    m_filenameLabel->setText(QString("<b>%1</b><br/>%2")
        .arg(info.fileName())
        .arg(info.path()));
    
    // Rating
    QString stars;
    for (int i = 0; i < 5; ++i) {
        stars += (i < metadata.rating) ? "★" : "☆";
    }
    m_ratingLabel->setText(stars + QString(" (%1/5)").arg(metadata.rating));
    
    // AI Analysis
    if (metadata.hasPhotoGuruMetadata()) {
        m_titleEdit->setText(metadata.llm_title);
        m_descriptionEdit->setText(metadata.llm_description);
        m_keywordsLabel->setText(metadata.llm_keywords.join(", "));
    } else {
        m_titleEdit->setText("(Not analyzed yet)");
        m_descriptionEdit->setText("Run AI analysis to generate title, description, and keywords.");
        m_keywordsLabel->setText("-");
    }
    
    // Location
    if (!metadata.location_name.isEmpty()) {
        QString locText = metadata.location_name;
        if (metadata.gps_lat != 0.0 && metadata.gps_lon != 0.0) {
            locText += QString("\nGPS: %1, %2")
                .arg(metadata.gps_lat, 0, 'f', 6)
                .arg(metadata.gps_lon, 0, 'f', 6);
        }
        m_locationLabel->setText(locText);
    } else {
        m_locationLabel->setText("-");
    }
    
    // EXIF
    m_exifLabel->setText(formatExifInfo(metadata));
    
    // Technical
    m_technicalLabel->setText(formatTechnicalInfo(metadata.technical));
}

QString MetadataPanel::formatExifInfo(const PhotoMetadata& meta) {
    QStringList parts;
    
    if (!meta.camera_make.isEmpty()) {
        parts << QString("<b>Camera:</b> %1 %2")
            .arg(meta.camera_make).arg(meta.camera_model);
    }
    
    if (meta.datetime_original.isValid()) {
        parts << QString("<b>Date:</b> %1")
            .arg(meta.datetime_original.toString("yyyy-MM-dd hh:mm:ss"));
    }
    
    if (meta.aperture > 0) {
        parts << QString("<b>Aperture:</b> f/%1").arg(meta.aperture, 0, 'f', 1);
    }
    
    if (meta.shutter_speed > 0) {
        parts << QString("<b>Shutter:</b> 1/%1s").arg(int(1.0 / meta.shutter_speed));
    }
    
    if (meta.iso > 0) {
        parts << QString("<b>ISO:</b> %1").arg(meta.iso);
    }
    
    if (meta.focal_length > 0) {
        parts << QString("<b>Focal Length:</b> %1mm").arg(meta.focal_length, 0, 'f', 0);
    }
    
    return parts.isEmpty() ? "-" : parts.join("<br/>");
}

QString MetadataPanel::formatTechnicalInfo(const TechnicalMetadata& tech) {
    QStringList parts;
    
    parts << QString("<b>Overall Quality:</b> %1/100")
        .arg(int(tech.overall_quality * 100));
    
    parts << QString("<b>Sharpness:</b> %1/100")
        .arg(int(tech.sharpness_score * 100));
    
    parts << QString("<b>Exposure Quality:</b> %1/100")
        .arg(int(tech.exposure_quality * 100));
    
    parts << QString("<b>Aesthetic Score:</b> %1/100")
        .arg(int(tech.aesthetic_score * 100));
    
    if (tech.face_count > 0) {
        parts << QString("<b>Faces Detected:</b> %1").arg(tech.face_count);
    }
    
    if (tech.blur_detected) {
        parts << "<span style='color: #ff6b6b;'><b>⚠ Blur detected</b></span>";
    }
    
    if (tech.highlights_clipped) {
        parts << "<span style='color: #ffa500;'><b>⚠ Highlights clipped</b></span>";
    }
    
    if (tech.shadows_blocked) {
        parts << "<span style='color: #ffa500;'><b>⚠ Shadows blocked</b></span>";
    }
    
    if (!tech.burst_group.isEmpty()) {
        QString burstInfo = QString("<b>Burst Group:</b> %1 (position %2)")
            .arg(tech.burst_group).arg(tech.burst_position);
        if (tech.is_best_in_burst) {
            burstInfo += " <span style='color: #51cf66;'>★ BEST</span>";
        }
        parts << burstInfo;
    }
    
    if (!tech.duplicate_group.isEmpty()) {
        parts << QString("<span style='color: #ffa500;'><b>⚠ Possible duplicate:</b> %1</span>")
            .arg(tech.duplicate_group);
    }
    
    return parts.join("<br/>");
}

void MetadataPanel::clear() {
    m_filenameLabel->setText("No image loaded");
    m_ratingLabel->setText("☆☆☆☆☆");
    m_titleEdit->clear();
    m_descriptionEdit->clear();
    m_keywordsLabel->setText("-");
    m_exifLabel->setText("-");
    m_technicalLabel->setText("-");
    m_locationLabel->setText("-");
    m_currentFilepath.clear();
}

} // namespace PhotoGuru
