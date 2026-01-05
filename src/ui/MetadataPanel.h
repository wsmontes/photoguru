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

namespace PhotoGuru {

class MetadataPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MetadataPanel(QWidget* parent = nullptr);
    
    void loadMetadata(const QString& filepath);
    void clear();
    void setEditable(bool editable);
    
signals:
    void metadataChanged(const QString& filepath);
    void editModeChanged(bool editing);
    
private:
    void setupUI();
    void displayMetadata(const PhotoMetadata& metadata);
    QString formatExifInfo(const PhotoMetadata& meta);
    QString formatTechnicalInfo(const TechnicalMetadata& tech);
    QString formatAIAnalysis(const PhotoMetadata& meta);
    void saveMetadata();
    void cancelEdit();
    void updateRatingDisplay(int rating);
    
    // UI Components
    QLabel* m_filenameLabel;
    QLabel* m_ratingLabel;
    QSlider* m_ratingSlider;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_keywordsEdit;
    QLineEdit* m_categoryEdit;
    QLineEdit* m_locationEdit;
    QLabel* m_exifLabel;
    QLabel* m_technicalLabel;
    
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
    QPushButton* m_editButton;
    
    QString m_currentFilepath;
    PhotoMetadata m_currentMetadata;
    bool m_isEditing;
};

} // namespace PhotoGuru
