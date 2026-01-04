#pragma once

#include <QString>
#include <QSqlDatabase>
#include "PhotoMetadata.h"

namespace PhotoGuru {

class PhotoDatabase {
public:
    static PhotoDatabase& instance();
    
    bool initialize(const QString& dbPath);
    void close();
    
    // TODO: Implement catalog functionality
    // bool addPhoto(const PhotoMetadata& meta);
    // std::vector<PhotoMetadata> searchByKeywords(const QStringList& keywords);
    // std::vector<PhotoMetadata> searchBySKP(const QString& keyId);
    
private:
    PhotoDatabase() = default;
    ~PhotoDatabase() { close(); }
    PhotoDatabase(const PhotoDatabase&) = delete;
    PhotoDatabase& operator=(const PhotoDatabase&) = delete;
    
    QSqlDatabase m_db;
    bool m_initialized = false;
};

} // namespace PhotoGuru
