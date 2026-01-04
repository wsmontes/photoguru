#pragma once

#include "../core/PhotoMetadata.h"
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QVBoxLayout>

namespace PhotoGuru {

class MetadataPanel : public QWidget {
    Q_OBJECT
    
public:
    explicit MetadataPanel(QWidget* parent = nullptr);
    
    void loadMetadata(const QString& filepath);
    void clear();
    
private:
    void setupUI();
    void displayMetadata(const PhotoMetadata& metadata);
    QString formatExifInfo(const PhotoMetadata& meta);
    QString formatTechnicalInfo(const TechnicalMetadata& tech);
    QString formatAIAnalysis(const PhotoMetadata& meta);
    
    // UI Components
    QLabel* m_filenameLabel;
    QLabel* m_ratingLabel;
    QTextEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QLabel* m_keywordsLabel;
    QLabel* m_exifLabel;
    QLabel* m_technicalLabel;
    QLabel* m_locationLabel;
    
    QString m_currentFilepath;
};

} // namespace PhotoGuru
