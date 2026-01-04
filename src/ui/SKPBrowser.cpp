#include "SKPBrowser.h"
#include "PhotoMetadata.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

namespace PhotoGuru {

SKPBrowser::SKPBrowser(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setupUI();
}

void SKPBrowser::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);
    
    // Info label
    m_infoLabel = new QLabel("Semantic Key Protocol (SKP) Browser");
    m_infoLabel->setStyleSheet("font-weight: bold; padding: 5px; background-color: #2a2a2a;");
    m_infoLabel->setWordWrap(true);
    mainLayout->addWidget(m_infoLabel);
    
    // Tree widget for keys
    m_keyTree = new QTreeWidget();
    m_keyTree->setHeaderLabels({"Key Type", "ID", "Role"});
    m_keyTree->setColumnWidth(0, 150);
    m_keyTree->setAlternatingRowColors(true);
    m_keyTree->setRootIsDecorated(true);
    m_keyTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_keyTree->setMinimumHeight(200);
    mainLayout->addWidget(m_keyTree);
    
    // Search button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_searchButton = new QPushButton("Search Similar Images");
    m_searchButton->setEnabled(false);
    buttonLayout->addWidget(m_searchButton);
    mainLayout->addLayout(buttonLayout);
    
    // Info text
    QLabel* helpLabel = new QLabel(
        "<small>"
        "<b>Semantic Keys</b> represent stable fields of meaning:<br/>"
        "• <b>Anchor:</b> Identity/context (person, place, event)<br/>"
        "• <b>Gate:</b> Filter/modulator (mood, atmosphere)<br/>"
        "• <b>Link:</b> Connection between keys (group, relation)<br/>"
        "• <b>Composite:</b> Combined from multiple keys<br/><br/>"
        "Select a key and click 'Search' to find similar images."
        "</small>"
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("padding: 8px; background-color: #2a2a2a; border-radius: 3px;");
    mainLayout->addWidget(helpLabel);
    
    // Connect signals
    connect(m_keyTree, &QTreeWidget::itemClicked, 
            this, &SKPBrowser::onKeySelected);
    connect(m_searchButton, &QPushButton::clicked,
            this, &SKPBrowser::onSearchClicked);
}

void SKPBrowser::loadImageKeys(const QString& filepath) {
    m_currentFilepath = filepath;
    clear();
    
    auto metaOpt = MetadataReader::instance().read(filepath);
    if (!metaOpt) {
        m_infoLabel->setText("No semantic keys found");
        return;
    }
    
    displayKeys(*metaOpt);
}

void SKPBrowser::displayKeys(const PhotoMetadata& metadata) {
    m_keyTree->clear();
    
    int keyCount = 0;
    
    // Image key
    if (metadata.skp_image_key) {
        addKeyToTree("Image Key", *metadata.skp_image_key);
        keyCount++;
    }
    
    // Person keys
    if (!metadata.skp_person_keys.empty()) {
        QTreeWidgetItem* personRoot = new QTreeWidgetItem(m_keyTree, 
            {"Person Keys", "", ""});
        personRoot->setExpanded(true);
        
        for (const auto& key : metadata.skp_person_keys) {
            QTreeWidgetItem* item = new QTreeWidgetItem(personRoot);
            item->setText(0, "Person");
            item->setText(1, key.key_id);
            item->setText(2, key.role);
            item->setData(0, Qt::UserRole, key.key_id);
            keyCount++;
        }
    }
    
    // Group keys
    if (!metadata.skp_group_keys.isEmpty()) {
        QTreeWidgetItem* groupRoot = new QTreeWidgetItem(m_keyTree,
            {"Group Keys", "", ""});
        groupRoot->setExpanded(true);
        
        for (const QString& keyId : metadata.skp_group_keys) {
            QTreeWidgetItem* item = new QTreeWidgetItem(groupRoot);
            item->setText(0, "Group");
            item->setText(1, keyId);
            item->setText(2, "link");
            item->setData(0, Qt::UserRole, keyId);
            keyCount++;
        }
    }
    
    // Global key
    if (!metadata.skp_global_key.isEmpty()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_keyTree);
        item->setText(0, "Global Key");
        item->setText(1, metadata.skp_global_key);
        item->setText(2, "anchor");
        item->setData(0, Qt::UserRole, metadata.skp_global_key);
        keyCount++;
    }
    
    if (keyCount == 0) {
        m_infoLabel->setText("No semantic keys found - run AI analysis first");
    } else {
        m_infoLabel->setText(QString("Found %1 semantic key(s)").arg(keyCount));
    }
}

void SKPBrowser::addKeyToTree(const QString& category, const SemanticKeyData& key) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_keyTree);
    item->setText(0, category);
    item->setText(1, key.key_id);
    item->setText(2, key.role);
    item->setData(0, Qt::UserRole, key.key_id);
}

void SKPBrowser::onKeySelected(QTreeWidgetItem* item, int column) {
    if (!item) return;
    
    QString keyId = item->data(0, Qt::UserRole).toString();
    if (!keyId.isEmpty()) {
        m_selectedKeyId = keyId;
        m_searchButton->setEnabled(true);
        m_searchButton->setText(QString("Search by Key: %1").arg(keyId.left(8)));
    }
}

void SKPBrowser::onSearchClicked() {
    if (m_selectedKeyId.isEmpty()) return;
    
    emit searchByKey(m_selectedKeyId);
    
    // Show info dialog with next steps
    QMessageBox info(this);
    info.setWindowTitle("Semantic Search");
    info.setIcon(QMessageBox::Information);
    info.setText(QString("Searching for images similar to key: %1").arg(m_selectedKeyId));
    info.setInformativeText(
        "This will use vector similarity (cosine distance) to find images \n"
        "with semantic keys close to this reference.\n\n"
        "Implementation: The viewer will scan all images with SKP metadata,\n"
        "extract their semantic key vectors, and rank by similarity.\n\n"
        "For production: This would integrate with the agent_v2.py\n"
        "semantic search capabilities."
    );
    info.setDetailedText(
        "Technical Details:\n"
        "- Uses CLIP embeddings stored in SKP metadata\n"
        "- Cosine similarity threshold: 0.8+\n"
        "- Results sorted by alignment score\n"
        "- Can filter by key role (anchor/variant/reference)"
    );
    info.exec();
}

void SKPBrowser::clear() {
    m_keyTree->clear();
    m_selectedKeyId.clear();
    m_searchButton->setEnabled(false);
    m_searchButton->setText("Search Similar Images");
    m_infoLabel->setText("Semantic Key Protocol (SKP) Browser");
}

} // namespace PhotoGuru
