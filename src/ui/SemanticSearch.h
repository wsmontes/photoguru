#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include "core/PhotoMetadata.h"

namespace PhotoGuru {

class SemanticSearch : public QWidget {
    Q_OBJECT
    
public:
    explicit SemanticSearch(QWidget* parent = nullptr);
    
    void setPhotos(const QList<PhotoMetadata>& photos);
    void performSearch(const QString& query);
    
signals:
    void photoSelected(const QString& filepath);
    void searchStarted();
    void searchCompleted(int resultsCount);
    
private:
    void setupUI();
    void onSearchClicked();
    void displayResults(const QList<QPair<PhotoMetadata, double>>& results);
    
    QLineEdit* m_searchInput;
    QPushButton* m_searchButton;
    QListWidget* m_resultsList;
    QLabel* m_statusLabel;
    QList<PhotoMetadata> m_photos;
};

} // namespace PhotoGuru
