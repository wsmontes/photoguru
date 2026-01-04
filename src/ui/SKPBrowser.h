#pragma once

#include "../core/PhotoMetadata.h"
#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>

namespace PhotoGuru {

class SKPBrowser : public QWidget {
    Q_OBJECT
    
public:
    explicit SKPBrowser(QWidget* parent = nullptr);
    
    void loadImageKeys(const QString& filepath);
    void clear();
    
signals:
    void searchByKey(const QString& keyId);
    
private slots:
    void onSearchClicked();
    void onKeySelected(QTreeWidgetItem* item, int column);
    
private:
    void setupUI();
    void displayKeys(const PhotoMetadata& metadata);
    void addKeyToTree(const QString& category, const SemanticKeyData& key);
    
    QTreeWidget* m_keyTree;
    QLabel* m_infoLabel;
    QPushButton* m_searchButton;
    
    QString m_currentFilepath;
    QString m_selectedKeyId;
};

} // namespace PhotoGuru
