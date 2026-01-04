#include "MapView.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QWebChannel>

namespace PhotoGuru {

MapView::MapView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MapView::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);
    
    // Load initial empty map
    clearMap();
}

void MapView::loadPhotos(const QList<PhotoMetadata>& photos) {
    m_photos = photos;
    generateMapHTML(photos);
}

void MapView::clearMap() {
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Photo Map</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <style>
        body { margin: 0; padding: 0; }
        #map { height: 100vh; width: 100vw; }
    </style>
</head>
<body>
    <div id="map"></div>
    <script>
        const map = L.map('map').setView([0, 0], 2);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            maxZoom: 19,
            attribution: '© OpenStreetMap contributors'
        }).addTo(map);
    </script>
</body>
</html>
    )";
    
    m_webView->setHtml(html);
}

void MapView::generateMapHTML(const QList<PhotoMetadata>& photos) {
    // Filter photos with GPS coordinates
    QList<PhotoMetadata> photosWithGPS;
    for (const auto& photo : photos) {
        if (photo.gps_lat != 0.0 && photo.gps_lon != 0.0) {
            photosWithGPS.append(photo);
        }
    }
    
    if (photosWithGPS.isEmpty()) {
        clearMap();
        return;
    }
    
    // Build markers JSON
    QJsonArray markers;
    for (const auto& photo : photosWithGPS) {
        QJsonObject marker;
        marker["lat"] = photo.gps_lat;
        marker["lng"] = photo.gps_lon;
        marker["title"] = photo.llm_title.isEmpty() ? photo.filename : photo.llm_title;
        marker["filepath"] = photo.filepath;
        marker["location"] = photo.location_name;
        marker["quality"] = photo.technical.overall_quality;
        markers.append(marker);
    }
    
    QJsonDocument doc(markers);
    QString markersJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    
    // Calculate center
    double centerLat = 0, centerLng = 0;
    for (const auto& photo : photosWithGPS) {
        centerLat += photo.gps_lat;
        centerLng += photo.gps_lon;
    }
    centerLat /= photosWithGPS.size();
    centerLng /= photosWithGPS.size();
    
    QString html = QString(R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Photo Map</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <link rel="stylesheet" href="https://unpkg.com/leaflet.markercluster@1.5.3/dist/MarkerCluster.css" />
    <link rel="stylesheet" href="https://unpkg.com/leaflet.markercluster@1.5.3/dist/MarkerCluster.Default.css" />
    <script src="https://unpkg.com/leaflet.markercluster@1.5.3/dist/leaflet.markercluster.js"></script>
    <style>
        body { margin: 0; padding: 0; background: #1e1e1e; }
        #map { height: 100vh; width: 100vw; }
        .photo-popup { min-width: 200px; }
        .photo-popup h3 { margin: 0 0 8px 0; color: #2c3e50; }
        .photo-popup p { margin: 4px 0; font-size: 12px; color: #555; }
        .quality-badge { 
            display: inline-block;
            padding: 2px 8px;
            border-radius: 3px;
            font-size: 11px;
            font-weight: bold;
        }
        .quality-high { background: #51cf66; color: white; }
        .quality-medium { background: #ffa500; color: white; }
        .quality-low { background: #ff6b6b; color: white; }
    </style>
</head>
<body>
    <div id="map"></div>
    <script>
        const map = L.map('map').setView([%1, %2], 13);
        
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            maxZoom: 19,
            attribution: '© OpenStreetMap contributors'
        }).addTo(map);
        
        const markers = %3;
        const markerCluster = L.markerClusterGroup({
            maxClusterRadius: 50,
            spiderfyOnMaxZoom: true
        });
        
        markers.forEach(m => {
            const qualityClass = m.quality > 0.7 ? 'quality-high' : 
                                 m.quality > 0.4 ? 'quality-medium' : 'quality-low';
            const qualityText = Math.round(m.quality * 100);
            
            const popup = `
                <div class="photo-popup">
                    <h3>${m.title}</h3>
                    <p><strong>Location:</strong> ${m.location || 'Unknown'}</p>
                    <p><strong>Quality:</strong> <span class="${qualityClass} quality-badge">${qualityText}/100</span></p>
                </div>
            `;
            
            const marker = L.marker([m.lat, m.lng])
                .bindPopup(popup);
            markerCluster.addLayer(marker);
        });
        
        map.addLayer(markerCluster);
        
        if (markers.length > 0) {
            const bounds = L.latLngBounds(markers.map(m => [m.lat, m.lng]));
            map.fitBounds(bounds, { padding: [50, 50] });
        }
    </script>
</body>
</html>
    )").arg(centerLat).arg(centerLng).arg(markersJson);
    
    m_webView->setHtml(html);
}

void MapView::focusOnPhoto(const QString& filepath) {
    // Find photo and center map on it
    for (const auto& photo : m_photos) {
        if (photo.filepath == filepath && photo.gps_lat != 0.0) {
            QString js = QString("map.setView([%1, %2], 16);")
                .arg(photo.gps_lat).arg(photo.gps_lon);
            m_webView->page()->runJavaScript(js);
            break;
        }
    }
}

} // namespace PhotoGuru
