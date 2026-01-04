#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "core/PhotoMetadata.h"

namespace PhotoGuru {

struct TimelineGroup {
    QString group_id;
    QString event_type;
    QString summary;
    QDateTime start_time;
    QDateTime end_time;
    QList<PhotoMetadata> photos;
    int duration_minutes;
};

class TimelineGroupWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit TimelineGroupWidget(const TimelineGroup& group, QWidget* parent = nullptr);
    
signals:
    void photoClicked(const QString& filepath);
    void groupClicked(const QString& groupId);
    
private:
    void setupUI(const TimelineGroup& group);
    TimelineGroup m_group;
};

class TimelineView : public QWidget {
    Q_OBJECT
    
public:
    explicit TimelineView(QWidget* parent = nullptr);
    
    void loadPhotos(const QList<PhotoMetadata>& photos);
    void clear();
    
signals:
    void photoSelected(const QString& filepath);
    void groupSelected(const QString& groupId);
    
private:
    void setupUI();
    void buildTimeline();
    QList<TimelineGroup> createGroups(const QList<PhotoMetadata>& photos);
    
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QList<PhotoMetadata> m_photos;
};

} // namespace PhotoGuru
