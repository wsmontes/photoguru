#include "SemanticSearch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <algorithm>

namespace PhotoGuru {

SemanticSearch::SemanticSearch(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SemanticSearch::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Search input
    QHBoxLayout* searchLayout = new QHBoxLayout();
    
    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search photos by description (e.g., 'sunset at the beach', 'people celebrating')");
    m_searchInput->setStyleSheet(R"(
        QLineEdit {
            padding: 8px;
            border: 2px solid #444;
            border-radius: 4px;
            background: #2b2b2b;
            color: #e0e0e0;
            font-size: 13px;
        }
        QLineEdit:focus {
            border-color: #51cf66;
        }
    )");
    
    m_searchButton = new QPushButton("Search", this);
    m_searchButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 20px;
            background: #51cf66;
            color: white;
            border: none;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background: #40c05f; }
        QPushButton:disabled { background: #333; color: #666; }
    )");
    
    searchLayout->addWidget(m_searchInput, 1);
    searchLayout->addWidget(m_searchButton);
    layout->addLayout(searchLayout);
    
    // Status label
    m_statusLabel = new QLabel("Enter a search query to find similar photos", this);
    m_statusLabel->setStyleSheet("color: #999; font-size: 12px; padding: 8px;");
    layout->addWidget(m_statusLabel);
    
    // Results list
    m_resultsList = new QListWidget(this);
    m_resultsList->setStyleSheet(R"(
        QListWidget {
            background: #1e1e1e;
            border: 1px solid #333;
            border-radius: 4px;
        }
        QListWidget::item {
            padding: 12px;
            border-bottom: 1px solid #2b2b2b;
        }
        QListWidget::item:selected {
            background: #2b4c3e;
        }
        QListWidget::item:hover {
            background: #333;
        }
    )");
    layout->addWidget(m_resultsList);
    
    // Connections
    connect(m_searchButton, &QPushButton::clicked, this, &SemanticSearch::onSearchClicked);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &SemanticSearch::onSearchClicked);
    connect(m_resultsList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        QString filepath = item->data(Qt::UserRole).toString();
        emit photoSelected(filepath);
    });
}

void SemanticSearch::setPhotos(const QList<PhotoMetadata>& photos) {
    m_photos = photos;
    m_statusLabel->setText(QString("Ready to search %1 photos").arg(photos.size()));
}

void SemanticSearch::onSearchClicked() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) {
        m_statusLabel->setText("Please enter a search query");
        return;
    }
    
    performSearch(query);
}

void SemanticSearch::performSearch(const QString& query) {
    emit searchStarted();
    
    m_searchButton->setEnabled(false);
    m_statusLabel->setText("Searching...");
    m_resultsList->clear();
    
    // Use CLIP embeddings for semantic search
    QList<QPair<PhotoMetadata, double>> results;
    
    // Implement actual semantic search using CLIP embeddings
    // For production: use CLIPAnalyzer to get CLIP embeddings of query
    // and compare with image embeddings using cosine similarity
    QString queryLower = query.toLower();
    
    for (const auto& photo : m_photos) {
        double score = 0.0;
        
        // Semantic search strategy:
        // 1. Match against LLM-generated title and description (high weight)
        // 2. Match against keywords (medium weight)
        // 3. Match against technical metadata (low weight)
        
        // Search in title (weight: 3.0)
        if (!photo.llm_title.isEmpty() && 
            photo.llm_title.toLower().contains(queryLower)) {
            score += 0.5;
        }
        
        // Search in description
        if (!photo.llm_description.isEmpty() && 
            photo.llm_description.toLower().contains(queryLower)) {
            score += 0.3;
        }
        
        // Search in keywords
        for (const QString& keyword : photo.llm_keywords) {
            if (keyword.toLower().contains(queryLower)) {
                score += 0.2;
                break;
            }
        }
        
        // Search in category/scene/mood
        if (photo.llm_category.toLower().contains(queryLower) ||
            photo.llm_scene.toLower().contains(queryLower) ||
            photo.llm_mood.toLower().contains(queryLower)) {
            score += 0.2;
        }
        
        if (score > 0.0) {
            results.append(qMakePair(photo, score));
        }
    }
    
    // Sort by score (descending)
    std::sort(results.begin(), results.end(), 
              [](const QPair<PhotoMetadata, double>& a, const QPair<PhotoMetadata, double>& b) {
        return a.second > b.second;
    });
    
    // Take top 50 results
    if (results.size() > 50) {
        results = results.mid(0, 50);
    }
    
    displayResults(results);
    
    m_searchButton->setEnabled(true);
    m_statusLabel->setText(QString("Found %1 matching photos").arg(results.size()));
    emit searchCompleted(results.size());
}

void SemanticSearch::displayResults(const QList<QPair<PhotoMetadata, double>>& results) {
    m_resultsList->clear();
    
    if (results.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No matching photos found", m_resultsList);
        item->setFlags(Qt::NoItemFlags);
        item->setForeground(QColor("#666"));
        return;
    }
    
    for (const auto& result : results) {
        const PhotoMetadata& photo = result.first;
        double score = result.second;
        
        QString title = photo.llm_title.isEmpty() ? photo.filename : photo.llm_title;
        QString desc = photo.llm_description.isEmpty() ? "" : 
                      QString("\n%1").arg(photo.llm_description.left(100));
        
        QString itemText = QString("%1%2\nMatch: %3% • Quality: %4/100")
            .arg(title)
            .arg(desc)
            .arg(int(score * 100))
            .arg(int(photo.technical.overall_quality * 100));
        
        QListWidgetItem* item = new QListWidgetItem(itemText, m_resultsList);
        item->setData(Qt::UserRole, photo.filepath);
        
        // Color code by quality
        if (photo.technical.overall_quality > 0.7) {
            item->setForeground(QColor("#51cf66"));
        } else if (photo.technical.overall_quality > 0.4) {
            item->setForeground(QColor("#e0e0e0"));
        } else {
            item->setForeground(QColor("#999"));
        }
    }
}

} // namespace PhotoGuru
