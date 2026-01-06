#pragma once

#include "../core/PhotoMetadata.h"
#include "../core/MetadataWriter.h"
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QMap>
#include <QScrollArea>
#include <QJsonObject>

namespace PhotoGuru {

// Collapsible group box for metadata sections
class CollapsibleGroupBox : public QWidget {
    Q_OBJECT
public:
    explicit CollapsibleGroupBox(const QString& title, QWidget* parent = nullptr);
    void setContentLayout(QLayout* layout);
    bool isExpanded() const { return m_expanded; }
    
private slots:
    void toggleExpanded();
    
private:
    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    bool m_expanded;
};

// Widget for editing a single metadata field
class MetadataFieldWidget : public QWidget {
    Q_OBJECT
public:
    explicit MetadataFieldWidget(const QString& key, const QString& value, 
                                 bool editable, QWidget* parent = nullptr);
    QString key() const { return m_key; }
    QString value() const;
    void setValue(const QString& value);
    void setEditable(bool editable);
    bool isModified() const { return m_modified; }
    
signals:
    void valueChanged(const QString& key, const QString& value);
    void removeRequested(const QString& key);
    
private:
    QString m_key;
    QString m_originalValue;
    QLineEdit* m_valueEdit;
    QTextEdit* m_textEdit;
    QPushButton* m_removeButton;
    bool m_multiline;
    bool m_modified;
};

class MetadataPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MetadataPanel(QWidget* parent = nullptr);
    
    void loadMetadata(const QString& filepath);
    void loadMetadata(const QString& filepath, const PhotoMetadata& metadata);  // From cache
    void clear();
    void setEditable(bool editable);
    void setAutoSaveMode(bool autoSave);  // true = save immediately, false = pending
    bool hasPendingChanges() const { return !m_pendingChanges.isEmpty(); }
    void clearPendingChanges();
    
signals:
    void metadataChanged(const QString& filepath);
    void editModeChanged(bool editing);
    
private:
    void setupUI();
    void setupMetadataTab();
    void displayMetadata(const PhotoMetadata& metadata);
    void displayAllMetadata(const QJsonObject& allMetadata);
    void populateMetadataSection(CollapsibleGroupBox* section, 
                                 const QStringList& keys, 
                                 const QJsonObject& metadata,
                                 bool editable);
    QString formatExifInfo(const PhotoMetadata& meta);
    QString formatTechnicalInfo(const TechnicalMetadata& tech);
    void saveMetadata();
    void cancelEdit();
    void updateRatingDisplay(int rating);
    void addNewField();
    void removeField(const QString& key);
    QJsonObject readAllMetadata(const QString& filepath);
    
    // Metadata content widgets
    QScrollArea* m_metadataScrollArea;
    QWidget* m_metadataContent;
    QVBoxLayout* m_metadataLayout;
    
    // Quick edit section (rating, title, description, keywords)
    QLabel* m_filenameLabel;
    QLabel* m_ratingLabel;
    QSlider* m_ratingSlider;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_keywordsEdit;
    QLineEdit* m_categoryEdit;
    QLineEdit* m_locationEdit;
    
    // Dynamic metadata sections
    CollapsibleGroupBox* m_exifSection;
    CollapsibleGroupBox* m_iptcSection;
    CollapsibleGroupBox* m_xmpSection;
    CollapsibleGroupBox* m_fileSection;
    CollapsibleGroupBox* m_technicalSection;
    CollapsibleGroupBox* m_customSection;
    
    // Control buttons
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QPushButton* m_editButton;
    QPushButton* m_addFieldButton;
    
    // Data storage
    QString m_currentFilepath;
    PhotoMetadata m_currentMetadata;
    QJsonObject m_allMetadata;
    QMap<QString, MetadataFieldWidget*> m_fieldWidgets;
    QMap<QString, QString> m_customFields;  // New custom fields added by user
    bool m_isEditing;
    
    // Pending changes system
    QMap<QString, QString> m_pendingChanges;  // filepath -> changed fields (for display)
    bool m_autoSaveMode;  // true = save immediately, false = accumulate pending
    QPushButton* m_commitButton;
    QPushButton* m_discardButton;
    QLabel* m_pendingLabel;
    
private slots:
    void onCommitChanges();
    void onDiscardChanges();
    void updatePendingUI();
};

} // namespace PhotoGuru
