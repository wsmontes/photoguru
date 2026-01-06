#include "MetadataPanel.h"
#include "PhotoMetadata.h"
#include "NotificationManager.h"
#include "ExifToolDaemon.h"
#include <QScrollArea>
#include <QFileInfo>
#include <QDateTime>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>
#include <QFrame>

namespace PhotoGuru {

// ========== CollapsibleGroupBox Implementation ==========

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Toggle button with title
    m_toggleButton = new QPushButton(QString("▶ %1").arg(title), this);
    m_toggleButton->setStyleSheet(R"(
        QPushButton {
            text-align: left;
            padding: 8px;
            background: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background: #353535; }
    )");
    connect(m_toggleButton, &QPushButton::clicked, this, &CollapsibleGroupBox::toggleExpanded);
    mainLayout->addWidget(m_toggleButton);
    
    // Content widget (initially hidden)
    m_contentWidget = new QWidget(this);
    m_contentWidget->setVisible(false);
    mainLayout->addWidget(m_contentWidget);
}

void CollapsibleGroupBox::setContentLayout(QLayout* layout) {
    // Remove old layout if exists
    if (m_contentWidget->layout()) {
        QLayout* oldLayout = m_contentWidget->layout();
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete oldLayout;
    }
    m_contentWidget->setLayout(layout);
}

void CollapsibleGroupBox::toggleExpanded() {
    m_expanded = !m_expanded;
    m_contentWidget->setVisible(m_expanded);
    
    QString buttonText = m_toggleButton->text();
    if (m_expanded) {
        buttonText.replace("▶", "▼");
    } else {
        buttonText.replace("▼", "▶");
    }
    m_toggleButton->setText(buttonText);
}

// ========== MetadataFieldWidget Implementation ==========

MetadataFieldWidget::MetadataFieldWidget(const QString& key, const QString& value, 
                                         bool editable, QWidget* parent)
    : QWidget(parent)
    , m_key(key)
    , m_originalValue(value)
    , m_valueEdit(nullptr)
    , m_textEdit(nullptr)
    , m_removeButton(nullptr)
    , m_modified(false)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);
    
    // Key label (fixed width for alignment)
    QLabel* keyLabel = new QLabel(key + ":", this);
    keyLabel->setStyleSheet("color: #aaa; font-weight: bold;");
    keyLabel->setMinimumWidth(150);
    keyLabel->setMaximumWidth(150);
    layout->addWidget(keyLabel);
    
    // Determine if we need multiline edit
    m_multiline = value.length() > 100 || value.contains('\n');
    
    if (m_multiline) {
        m_textEdit = new QTextEdit(this);
        m_textEdit->setPlainText(value);
        m_textEdit->setReadOnly(!editable);
        m_textEdit->setMaximumHeight(80);
        connect(m_textEdit, &QTextEdit::textChanged, this, [this]() {
            m_modified = true;
            emit valueChanged(m_key, m_textEdit->toPlainText());
        });
        layout->addWidget(m_textEdit);
    } else {
        m_valueEdit = new QLineEdit(value, this);
        m_valueEdit->setReadOnly(!editable);
        connect(m_valueEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_modified = true;
            emit valueChanged(m_key, text);
        });
        layout->addWidget(m_valueEdit);
    }
    
    // Remove button (only for custom fields in edit mode)
    if (editable && key.startsWith("Custom:")) {
        m_removeButton = new QPushButton("✕", this);
        m_removeButton->setMaximumWidth(30);
        m_removeButton->setStyleSheet("QPushButton { color: #ff6b6b; font-weight: bold; }");
        connect(m_removeButton, &QPushButton::clicked, this, [this]() {
            emit removeRequested(m_key);
        });
        layout->addWidget(m_removeButton);
    }
}

QString MetadataFieldWidget::value() const {
    if (m_textEdit) {
        return m_textEdit->toPlainText();
    } else if (m_valueEdit) {
        return m_valueEdit->text();
    }
    return QString();
}

void MetadataFieldWidget::setValue(const QString& value) {
    if (m_textEdit) {
        m_textEdit->setPlainText(value);
    } else if (m_valueEdit) {
        m_valueEdit->setText(value);
    }
    m_originalValue = value;
    m_modified = false;
}

void MetadataFieldWidget::setEditable(bool editable) {
    if (m_textEdit) {
        m_textEdit->setReadOnly(!editable);
    } else if (m_valueEdit) {
        m_valueEdit->setReadOnly(!editable);
    }
    
    if (m_removeButton) {
        m_removeButton->setVisible(editable);
    }
}

// ========== MetadataPanel Implementation ==========

MetadataPanel::MetadataPanel(QWidget* parent)
    : QWidget(parent)
    , m_isEditing(false)
    , m_autoSaveMode(true)  // Default: auto-save enabled
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
    
    // Setup metadata content directly (no tabs)
    setupMetadataTab();
    mainLayout->addWidget(m_metadataScrollArea);
    
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
    
    m_addFieldButton = new QPushButton("+ Add Field", this);
    m_addFieldButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background: #17a2b8;
            border: none;
            border-radius: 4px;
            color: white;
        }
        QPushButton:hover { background: #138496; }
    )");
    m_addFieldButton->setVisible(false);
    connect(m_addFieldButton, &QPushButton::clicked, this, &MetadataPanel::addNewField);
    buttonLayout->addWidget(m_addFieldButton);
    
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // Pending changes section (initially hidden)
    QHBoxLayout* pendingLayout = new QHBoxLayout();
    
    m_pendingLabel = new QLabel("No pending changes", this);
    m_pendingLabel->setStyleSheet("color: #888; font-style: italic;");
    pendingLayout->addWidget(m_pendingLabel);
    
    pendingLayout->addStretch();
    
    m_commitButton = new QPushButton("💾 Commit Changes", this);
    m_commitButton->setStyleSheet(R"(
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
    m_commitButton->setVisible(false);
    connect(m_commitButton, &QPushButton::clicked, this, &MetadataPanel::onCommitChanges);
    pendingLayout->addWidget(m_commitButton);
    
    m_discardButton = new QPushButton("🗑️ Discard", this);
    m_discardButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background: #dc3545;
            border: none;
            border-radius: 4px;
            color: white;
        }
        QPushButton:hover { background: #c82333; }
    )");
    m_discardButton->setVisible(false);
    connect(m_discardButton, &QPushButton::clicked, this, &MetadataPanel::onDiscardChanges);
    pendingLayout->addWidget(m_discardButton);
    
    mainLayout->addLayout(pendingLayout);
}

void MetadataPanel::setupMetadataTab() {
    // Create scroll area for content
    m_metadataScrollArea = new QScrollArea();
    m_metadataScrollArea->setWidgetResizable(true);
    m_metadataScrollArea->setFrameShape(QFrame::NoFrame);
    
    m_metadataContent = new QWidget();
    m_metadataLayout = new QVBoxLayout(m_metadataContent);
    m_metadataLayout->setSpacing(12);
    
    // File info section (always visible at top)
    QGroupBox* fileGroup = new QGroupBox("File Info");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    m_filenameLabel = new QLabel("No image loaded");
    m_filenameLabel->setWordWrap(true);
    m_filenameLabel->setStyleSheet("color: #aaa;");
    fileLayout->addWidget(m_filenameLabel);
    m_metadataLayout->addWidget(fileGroup);
    
    // Quick Edit Section (commonly used fields)
    QGroupBox* quickEditGroup = new QGroupBox("Quick Edit");
    QVBoxLayout* quickEditLayout = new QVBoxLayout(quickEditGroup);
    
    // Rating
    QHBoxLayout* ratingLayout = new QHBoxLayout();
    ratingLayout->addWidget(new QLabel("Rating:", this));
    m_ratingLabel = new QLabel("☆☆☆☆☆ (0/5)");
    m_ratingLabel->setStyleSheet("font-size: 16px; color: #1f91ff;");
    ratingLayout->addWidget(m_ratingLabel);
    ratingLayout->addStretch();
    quickEditLayout->addLayout(ratingLayout);
    
    m_ratingSlider = new QSlider(Qt::Horizontal, this);
    m_ratingSlider->setRange(0, 5);
    m_ratingSlider->setValue(0);
    m_ratingSlider->setEnabled(false);
    connect(m_ratingSlider, &QSlider::valueChanged, this, &MetadataPanel::updateRatingDisplay);
    quickEditLayout->addWidget(m_ratingSlider);
    
    // Title
    quickEditLayout->addWidget(new QLabel("Title:", this));
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("Enter title...");
    m_titleEdit->setReadOnly(true);
    quickEditLayout->addWidget(m_titleEdit);
    
    // Description
    quickEditLayout->addWidget(new QLabel("Description:", this));
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText("Enter description...");
    m_descriptionEdit->setMaximumHeight(100);
    m_descriptionEdit->setReadOnly(true);
    quickEditLayout->addWidget(m_descriptionEdit);
    
    // Keywords
    quickEditLayout->addWidget(new QLabel("Keywords:", this));
    m_keywordsEdit = new QLineEdit(this);
    m_keywordsEdit->setPlaceholderText("Comma-separated keywords...");
    m_keywordsEdit->setReadOnly(true);
    quickEditLayout->addWidget(m_keywordsEdit);
    
    // Category
    quickEditLayout->addWidget(new QLabel("Category:", this));
    m_categoryEdit = new QLineEdit(this);
    m_categoryEdit->setPlaceholderText("e.g., landscape, portrait...");
    m_categoryEdit->setReadOnly(true);
    quickEditLayout->addWidget(m_categoryEdit);
    
    // Location
    quickEditLayout->addWidget(new QLabel("Location:", this));
    m_locationEdit = new QLineEdit(this);
    m_locationEdit->setPlaceholderText("City, State, Country");
    m_locationEdit->setReadOnly(true);
    quickEditLayout->addWidget(m_locationEdit);
    
    m_metadataLayout->addWidget(quickEditGroup);
    
    // Create collapsible sections for different metadata groups
    m_exifSection = new CollapsibleGroupBox("EXIF Data", this);
    m_metadataLayout->addWidget(m_exifSection);
    
    m_iptcSection = new CollapsibleGroupBox("IPTC Data", this);
    m_metadataLayout->addWidget(m_iptcSection);
    
    m_xmpSection = new CollapsibleGroupBox("XMP Data", this);
    m_metadataLayout->addWidget(m_xmpSection);
    
    m_fileSection = new CollapsibleGroupBox("File Data", this);
    m_metadataLayout->addWidget(m_fileSection);
    
    m_technicalSection = new CollapsibleGroupBox("Quality Analysis", this);
    m_metadataLayout->addWidget(m_technicalSection);
    
    m_customSection = new CollapsibleGroupBox("Custom Fields", this);
    m_metadataLayout->addWidget(m_customSection);
    
    m_metadataLayout->addStretch();
    
    m_metadataScrollArea->setWidget(m_metadataContent);
}

void MetadataPanel::loadMetadata(const QString& filepath) {
    qDebug() << "[MetadataPanel] loadMetadata called for:" << filepath;
    
    m_currentFilepath = filepath;
    
    // Read structured metadata (may block if ExifToolDaemon is busy)
    auto metaOpt = MetadataReader::instance().read(filepath);
    
    if (!metaOpt) {
        clear();
        m_filenameLabel->setText(QFileInfo(filepath).fileName() + " (No metadata available)");
        m_editButton->setEnabled(false);
        return;
    }
    
    m_currentMetadata = *metaOpt;
    
    // Read all metadata (including all EXIF/IPTC/XMP fields)
    m_allMetadata = readAllMetadata(filepath);
    
    displayMetadata(m_currentMetadata);
    displayAllMetadata(m_allMetadata);
    
    m_editButton->setEnabled(true);
}

void MetadataPanel::loadMetadata(const QString& filepath, const PhotoMetadata& metadata) {
    qDebug() << "[MetadataPanel] loadMetadata from cache for:" << filepath;
    
    m_currentFilepath = filepath;
    m_currentMetadata = metadata;
    
    // Read all metadata (including all EXIF/IPTC/XMP fields) 
    // This is still needed for the "All Metadata" tab
    m_allMetadata = readAllMetadata(filepath);
    
    displayMetadata(m_currentMetadata);
    displayAllMetadata(m_allMetadata);
    
    m_editButton->setEnabled(true);
}

QJsonObject MetadataPanel::readAllMetadata(const QString& filepath) {
    // Use ExifTool to get ALL metadata fields
    QStringList args = {"-json", "-a", "-G", filepath};
    QString output = ExifToolDaemon::instance().executeCommand(args);
    
    if (output.isEmpty()) {
        return QJsonObject();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    if (!doc.isArray() || doc.array().isEmpty()) {
        return QJsonObject();
    }
    
    return doc.array()[0].toObject();
}

void MetadataPanel::displayMetadata(const PhotoMetadata& metadata) {
    // Filename
    QFileInfo info(metadata.filepath);
    m_filenameLabel->setText(QString("<b>%1</b><br/><span style='color:#888;'>%2</span>")
        .arg(info.fileName())
        .arg(info.path()));
    
    // Quick edit fields
    m_ratingSlider->setValue(metadata.rating);
    updateRatingDisplay(metadata.rating);
    
    m_titleEdit->setText(metadata.llm_title);
    m_descriptionEdit->setText(metadata.llm_description);
    m_keywordsEdit->setText(metadata.llm_keywords.join(", "));
    m_categoryEdit->setText(metadata.llm_category);
    m_locationEdit->setText(metadata.location_name);
    
    // Update technical analysis section
    QVBoxLayout* techLayout = new QVBoxLayout();
    QString techInfo = formatTechnicalInfo(metadata.technical);
    QLabel* techLabel = new QLabel(techInfo);
    techLabel->setWordWrap(true);
    techLabel->setStyleSheet("color: #aaa; padding: 8px;");
    techLayout->addWidget(techLabel);
    m_technicalSection->setContentLayout(techLayout);
}

void MetadataPanel::displayAllMetadata(const QJsonObject& allMetadata) {
    // Clear existing field widgets
    m_fieldWidgets.clear();
    
    // Separate metadata by group
    QStringList exifKeys, iptcKeys, xmpKeys, fileKeys;
    
    for (auto it = allMetadata.begin(); it != allMetadata.end(); ++it) {
        QString key = it.key();
        if (key.startsWith("EXIF:")) {
            exifKeys << key;
        } else if (key.startsWith("IPTC:")) {
            iptcKeys << key;
        } else if (key.startsWith("XMP") || key.startsWith("XMP-")) {
            xmpKeys << key;
        } else if (key.startsWith("File:")) {
            fileKeys << key;
        }
    }
    
    // Sort keys alphabetically
    exifKeys.sort();
    iptcKeys.sort();
    xmpKeys.sort();
    fileKeys.sort();
    
    // Populate sections
    populateMetadataSection(m_exifSection, exifKeys, allMetadata, false);
    populateMetadataSection(m_iptcSection, iptcKeys, allMetadata, false);
    populateMetadataSection(m_xmpSection, xmpKeys, allMetadata, false);
    populateMetadataSection(m_fileSection, fileKeys, allMetadata, false);
    
    // Populate custom fields section
    QVBoxLayout* customLayout = new QVBoxLayout();
    if (m_customFields.isEmpty()) {
        QLabel* emptyLabel = new QLabel("No custom fields. Click 'Add Field' to create one.");
        emptyLabel->setStyleSheet("color: #888; font-style: italic; padding: 8px;");
        customLayout->addWidget(emptyLabel);
    } else {
        for (auto it = m_customFields.begin(); it != m_customFields.end(); ++it) {
            MetadataFieldWidget* widget = new MetadataFieldWidget(it.key(), it.value(), m_isEditing, this);
            connect(widget, &MetadataFieldWidget::removeRequested, this, &MetadataPanel::removeField);
            m_fieldWidgets[it.key()] = widget;
            customLayout->addWidget(widget);
        }
    }
    customLayout->addStretch();
    m_customSection->setContentLayout(customLayout);
}

void MetadataPanel::populateMetadataSection(CollapsibleGroupBox* section, 
                                            const QStringList& keys, 
                                            const QJsonObject& metadata,
                                            bool editable) {
    QVBoxLayout* layout = new QVBoxLayout();
    
    if (keys.isEmpty()) {
        QLabel* emptyLabel = new QLabel("No data available");
        emptyLabel->setStyleSheet("color: #888; font-style: italic; padding: 8px;");
        layout->addWidget(emptyLabel);
    } else {
        for (const QString& key : keys) {
            QJsonValue value = metadata[key];
            QString valueStr;
            
            if (value.isArray()) {
                QJsonArray arr = value.toArray();
                QStringList items;
                for (const QJsonValue& v : arr) {
                    items << v.toString();
                }
                valueStr = items.join(", ");
            } else {
                valueStr = value.toString();
            }
            
            if (!valueStr.isEmpty()) {
                MetadataFieldWidget* widget = new MetadataFieldWidget(key, valueStr, editable, this);
                m_fieldWidgets[key] = widget;
                layout->addWidget(widget);
            }
        }
    }
    
    layout->addStretch();
    section->setContentLayout(layout);
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

void MetadataPanel::setEditable(bool editable) {
    m_isEditing = editable;
    
    // Toggle edit mode for quick edit fields
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
    m_addFieldButton->setVisible(editable);
    
    // Change background color to indicate edit mode
    QString editStyle = editable 
        ? "background: #2a2a2a; border: 1px solid #1f91ff;" 
        : "background: #1e1e1e;";
    
    m_titleEdit->setStyleSheet(editStyle);
    m_descriptionEdit->setStyleSheet(editStyle);
    m_keywordsEdit->setStyleSheet(editStyle);
    m_categoryEdit->setStyleSheet(editStyle);
    m_locationEdit->setStyleSheet(editStyle);
    
    // Toggle editable state for all field widgets
    for (auto widget : m_fieldWidgets) {
        widget->setEditable(editable);
    }
    
    emit editModeChanged(editable);
}

void MetadataPanel::addNewField() {
    bool ok;
    QString fieldName = QInputDialog::getText(this, "Add Custom Field",
                                              "Enter field name (e.g., 'Event', 'Project', 'Copyright'):",
                                              QLineEdit::Normal, "", &ok);
    
    if (ok && !fieldName.isEmpty()) {
        // Sanitize field name
        fieldName = fieldName.trimmed();
        QString key = "Custom:" + fieldName;
        
        // Check if already exists
        if (m_customFields.contains(key)) {
            QMessageBox::warning(this, "Field Exists", 
                               QString("A custom field named '%1' already exists.").arg(fieldName));
            return;
        }
        
        QString value = QInputDialog::getText(this, "Field Value",
                                              QString("Enter value for '%1':").arg(fieldName),
                                              QLineEdit::Normal, "", &ok);
        
        if (ok) {
            m_customFields[key] = value;
            
            // Refresh display
            displayAllMetadata(m_allMetadata);
            
            NotificationManager::instance().showInfo(QString("Added custom field '%1'").arg(fieldName));
        }
    }
}

void MetadataPanel::removeField(const QString& key) {
    if (m_customFields.contains(key)) {
        m_customFields.remove(key);
        displayAllMetadata(m_allMetadata);
        NotificationManager::instance().showInfo("Custom field removed");
    }
}

void MetadataPanel::saveMetadata() {
    if (m_currentFilepath.isEmpty()) {
        return;
    }
    
    // Check if in pending mode (not auto-save)
    if (!m_autoSaveMode) {
        // Mark as pending change
        m_pendingChanges[m_currentFilepath] = "modified";
        
        // Highlight edited fields
        QString highlightStyle = "background-color: #fffacd; border: 1px solid #ffa500;";
        m_titleEdit->setStyleSheet(highlightStyle);
        m_descriptionEdit->setStyleSheet(highlightStyle);
        m_keywordsEdit->setStyleSheet(highlightStyle);
        m_categoryEdit->setStyleSheet(highlightStyle);
        m_locationEdit->setStyleSheet(highlightStyle);
        
        updatePendingUI();
        NotificationManager::instance().showInfo("Changes marked as pending. Use 'Commit' to save.");
        return;
    }
    
    // Auto-save mode: save immediately
    // Update current metadata with edited values
    m_currentMetadata.rating = m_ratingSlider->value();
    m_currentMetadata.llm_title = m_titleEdit->text().trimmed();
    m_currentMetadata.llm_description = m_descriptionEdit->toPlainText().trimmed();
    
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
    
    // Also save custom fields using ExifTool directly
    if (success && !m_customFields.isEmpty()) {
        QStringList args;
        args << "-overwrite_original";
        
        for (auto it = m_customFields.begin(); it != m_customFields.end(); ++it) {
            QString fieldName = it.key().mid(7);  // Remove "Custom:" prefix
            QString value = it.value();
            // Write to XMP custom namespace
            args << QString("-XMP-photoguru:%1=%2").arg(fieldName).arg(value);
        }
        
        args << m_currentFilepath;
        ExifToolDaemon::instance().executeCommand(args);
    }
    
    if (success) {
        setEditable(false);
        emit metadataChanged(m_currentFilepath);
        NotificationManager::instance().showSuccess("Metadata saved successfully! Changes written to file.");
    } else {
        NotificationManager::instance().showError("Failed to save metadata. Make sure the file is writable and ExifTool is installed.");
    }
}

void MetadataPanel::cancelEdit() {
    // Reload original metadata
    displayMetadata(m_currentMetadata);
    displayAllMetadata(m_allMetadata);
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
    m_allMetadata = QJsonObject();
    m_fieldWidgets.clear();
    m_customFields.clear();
    
    m_filenameLabel->setText("No image loaded");
    m_ratingSlider->setValue(0);
    m_titleEdit->clear();
    m_descriptionEdit->clear();
    m_keywordsEdit->clear();
    m_categoryEdit->clear();
    m_locationEdit->clear();
    
    m_editButton->setEnabled(false);
    setEditable(false);
}

void MetadataPanel::setAutoSaveMode(bool autoSave) {
    m_autoSaveMode = autoSave;
    updatePendingUI();
}

void MetadataPanel::clearPendingChanges() {
    m_pendingChanges.clear();
    updatePendingUI();
}

void MetadataPanel::onCommitChanges() {
    if (m_pendingChanges.isEmpty()) return;
    
    // Save all pending changes
    int count = m_pendingChanges.size();
    m_pendingChanges.clear();
    
    // Trigger actual save
    saveMetadata();
    
    updatePendingUI();
    NotificationManager::instance().showSuccess(
        QString("Committed %1 pending change(s)").arg(count));
}

void MetadataPanel::onDiscardChanges() {
    if (m_pendingChanges.isEmpty()) return;
    
    int count = m_pendingChanges.size();
    m_pendingChanges.clear();
    
    // Reload original metadata
    if (!m_currentFilepath.isEmpty()) {
        loadMetadata(m_currentFilepath);
    }
    
    updatePendingUI();
    NotificationManager::instance().showInfo(
        QString("Discarded %1 pending change(s)").arg(count));
}

void MetadataPanel::updatePendingUI() {
    int pendingCount = m_pendingChanges.size();
    bool hasPending = pendingCount > 0;
    
    if (hasPending) {
        m_pendingLabel->setText(QString("%1 pending change(s)").arg(pendingCount));
        m_pendingLabel->setStyleSheet("color: #ffa500; font-weight: bold;");
    } else {
        m_pendingLabel->setText("No pending changes");
        m_pendingLabel->setStyleSheet("color: #888; font-style: italic;");
    }
    
    // Show/hide commit/discard buttons based on mode and pending status
    bool showButtons = !m_autoSaveMode && hasPending;
    m_commitButton->setVisible(showButtons);
    m_discardButton->setVisible(showButtons);
}

} // namespace PhotoGuru

