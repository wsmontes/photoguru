#include "TimelineView.h"
#include <QGridLayout>
#include <QFrame>
#include <algorithm>

namespace PhotoGuru {

TimelineGroupWidget::TimelineGroupWidget(const TimelineGroup& group, QWidget* parent)
    : QWidget(parent), m_group(group)
{
    setupUI(group);
}

void TimelineGroupWidget::setupUI(const TimelineGroup& group) {
    setStyleSheet(R"(
        TimelineGroupWidget {
            background: #2b2b2b;
            border-radius: 8px;
            padding: 16px;
        }
    )");
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Header
    QLabel* dateLabel = new QLabel(group.start_time.toString("MMMM d, yyyy"), this);
    dateLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #51cf66;");
    layout->addWidget(dateLabel);
    
    // Event type
    if (!group.event_type.isEmpty()) {
        QLabel* eventLabel = new QLabel(group.event_type, this);
        eventLabel->setStyleSheet("font-size: 14px; color: #ffa500; font-weight: bold;");
        layout->addWidget(eventLabel);
    }
    
    // Summary
    if (!group.summary.isEmpty()) {
        QLabel* summaryLabel = new QLabel(group.summary, this);
        summaryLabel->setWordWrap(true);
        summaryLabel->setStyleSheet("font-size: 12px; color: #aaa; margin: 4px 0 8px 0;");
        layout->addWidget(summaryLabel);
    }
    
    // Stats
    QString stats = QString("%1 photos • %2 minutes")
        .arg(group.photos.size())
        .arg(group.duration_minutes);
    QLabel* statsLabel = new QLabel(stats, this);
    statsLabel->setStyleSheet("font-size: 11px; color: #777;");
    layout->addWidget(statsLabel);
    
    // Photo grid (thumbnails)
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(4);
    
    int maxPhotos = std::min(6, static_cast<int>(group.photos.size()));
    for (int i = 0; i < maxPhotos; i++) {
        QPushButton* thumbBtn = new QPushButton(this);
        thumbBtn->setFixedSize(80, 80);
        thumbBtn->setStyleSheet(R"(
            QPushButton {
                background: #1e1e1e;
                border: 2px solid #333;
                border-radius: 4px;
            }
            QPushButton:hover {
                border-color: #51cf66;
            }
        )");
        
        const QString filepath = group.photos[i].filepath;
        connect(thumbBtn, &QPushButton::clicked, this, [this, filepath]() {
            emit photoClicked(filepath);
        });
        
        gridLayout->addWidget(thumbBtn, i / 3, i % 3);
    }
    
    layout->addLayout(gridLayout);
    
    // More button
    if (group.photos.size() > maxPhotos) {
        QPushButton* moreBtn = new QPushButton(
            QString("+%1 more photos").arg(group.photos.size() - maxPhotos), this);
        moreBtn->setStyleSheet(R"(
            QPushButton {
                background: #333;
                color: #51cf66;
                border: none;
                padding: 8px;
                border-radius: 4px;
            }
            QPushButton:hover { background: #444; }
        )");
        connect(moreBtn, &QPushButton::clicked, this, [this]() {
            emit groupClicked(m_group.group_id);
        });
        layout->addWidget(moreBtn);
    }
}

TimelineView::TimelineView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void TimelineView::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setSpacing(16);
    m_contentLayout->setContentsMargins(16, 16, 16, 16);
    
    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);
}

void TimelineView::loadPhotos(const QList<PhotoMetadata>& photos) {
    m_photos = photos;
    buildTimeline();
}

void TimelineView::clear() {
    // Remove all widgets
    while (m_contentLayout->count() > 0) {
        QLayoutItem* item = m_contentLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_photos.clear();
}

void TimelineView::buildTimeline() {
    clear();
    
    if (m_photos.isEmpty()) {
        QLabel* emptyLabel = new QLabel("No photos to display", m_contentWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: #666; font-size: 14px; padding: 40px;");
        m_contentLayout->addWidget(emptyLabel);
        return;
    }
    
    // Create groups
    QList<TimelineGroup> groups = createGroups(m_photos);
    
    // Add group widgets
    for (const auto& group : groups) {
        TimelineGroupWidget* groupWidget = new TimelineGroupWidget(group, m_contentWidget);
        connect(groupWidget, &TimelineGroupWidget::photoClicked,
                this, &TimelineView::photoSelected);
        connect(groupWidget, &TimelineGroupWidget::groupClicked,
                this, &TimelineView::groupSelected);
        m_contentLayout->addWidget(groupWidget);
    }
    
    m_contentLayout->addStretch();
}

QList<TimelineGroup> TimelineView::createGroups(const QList<PhotoMetadata>& photos) {
    QList<TimelineGroup> groups;
    
    // Group photos by group_id
    QMap<QString, QList<PhotoMetadata>> groupMap;
    for (const auto& photo : photos) {
        QString groupId = photo.group_id.isEmpty() ? "ungrouped" : photo.group_id;
        groupMap[groupId].append(photo);
    }
    
    // Create TimelineGroup for each
    for (auto it = groupMap.begin(); it != groupMap.end(); ++it) {
        QList<PhotoMetadata> groupPhotos = it.value();
        
        // Sort by date
        std::sort(groupPhotos.begin(), groupPhotos.end(), 
                  [](const PhotoMetadata& a, const PhotoMetadata& b) {
            return a.datetime_original < b.datetime_original;
        });
        
        TimelineGroup group;
        group.group_id = it.key();
        group.photos = groupPhotos;
        
        // Find first photo with metadata for group context
        for (const auto& photo : groupPhotos) {
            if (!photo.group_context.isEmpty()) {
                group.event_type = photo.group_context["event_type"].toString();
                group.summary = photo.group_context["summary"].toString();
                break;
            }
        }
        
        // Calculate time range
        QList<QDateTime> times;
        for (const auto& photo : groupPhotos) {
            if (photo.datetime_original.isValid()) {
                times.append(photo.datetime_original);
            }
        }
        
        if (!times.isEmpty()) {
            std::sort(times.begin(), times.end());
            group.start_time = times.first();
            group.end_time = times.last();
            group.duration_minutes = group.start_time.secsTo(group.end_time) / 60;
        }
        
        groups.append(group);
    }
    
    // Sort groups by start time (newest first)
    std::sort(groups.begin(), groups.end(), 
              [](const TimelineGroup& a, const TimelineGroup& b) {
        return a.start_time > b.start_time;
    });
    
    return groups;
}

} // namespace PhotoGuru
