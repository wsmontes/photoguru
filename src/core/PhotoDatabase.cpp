#include "PhotoDatabase.h"
#include <QDebug>

namespace PhotoGuru {

PhotoDatabase& PhotoDatabase::instance() {
    static PhotoDatabase db;
    return db;
}

bool PhotoDatabase::initialize(const QString& dbPath) {
    if (m_initialized) return true;
    
    // SQLite catalog for future expansion
    // This would store:
    // - Image metadata cache for fast filtering
    // - Semantic keys and embeddings
    // - Analysis results and user ratings
    // - Collections and smart albums
    
    // For now, metadata is read directly from files via exiftool
    // which is more reliable and doesn't require sync
    qDebug() << "PhotoDatabase: Using direct file metadata reading";
    qDebug() << "               Catalog functionality available for future expansion";
    
    // Mark as initialized but don't actually use DB yet
    m_initialized = false;  // Set to true when SQLite implementation is added
    return true;  // Return true to not block app functionality
}

void PhotoDatabase::close() {
    if (m_initialized) {
        m_db.close();
        m_initialized = false;
    }
}

} // namespace PhotoGuru
