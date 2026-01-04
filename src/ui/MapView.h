#pragma once

#include <QWidget>
#include <QWebEngineView>
#include <QVBoxLayout>
#include "core/PhotoMetadata.h"

namespace PhotoGuru {

class MapView : public QWidget {
    Q_OBJECT
    
public:
    explicit MapView(QWidget* parent = nullptr);
    
    void loadPhotos(const QList<PhotoMetadata>& photos);
    void clearMap();
    void focusOnPhoto(const QString& filepath);
    
signals:
    void photoSelected(const QString& filepath);
    
private:
    void setupUI();
    void generateMapHTML(const QList<PhotoMetadata>& photos);
    
    QWebEngineView* m_webView;
    QList<PhotoMetadata> m_photos;
};

} // namespace PhotoGuru
