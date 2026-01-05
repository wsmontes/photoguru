#include "MetadataPanel.h"
#include "PhotoMetadata.h"
#include "NotificationManager.h"
#include "ExifToolDaemon.h"
#include <QScrollArea>
#include <QFileInfo>
#include <QDateTime>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QInputDialog>
#include <QMessageBox>
#include <QToolButton>
#include <QFrame>

namespace PhotoGuru {

// ========== CollapsibleGroupBox Implementation ==========

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent)
    : QWidget(parent)
    , m_expanded(false)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Toggle button with title
    m_toggleButton = new QPushButton(QString("▶ %1").arg(title), this);
    m_toggleButton->setStyleSheet(R"(
        QPushButton {
            text-align: left;
            padding: 8px;
            background: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover { background: #353535; }
    )");
    connect(m_toggleButton, &QPushButton::clicked, this, &CollapsibleGroupBox::toggleExpanded);
    mainLayout->addWidget(m_toggleButton);
    
    // Content widget (initially hidden)
    m_contentWidget = new QWidget(this);
    m_contentWidget->setVisible(false);
    mainLayout->addWidget(m_contentWidget);
}

void CollapsibleGroupBox::setContentLayout(QLayout* layout) {
    // Remove old layout if exists
    if (m_contentWidget->layout()) {
        QLayout* oldLayout = m_contentWidget->layout();
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete oldLayout;
    }
    m_contentWidget->setLayout(layout);
}

void CollapsibleGroupBox::toggleExpanded() {
    m_expanded = !m_expanded;
    m_contentWidget->setVisible(m_expanded);
    
    QString buttonText = m_toggleButton->text();
    if (m_expanded) {
        buttonText.replace("▶", "▼");
    } else {
        buttonText.replace("▼", "▶");
    }
    m_toggleButton->setText(buttonText);
}

// ========== MetadataFieldWidget Implementation ==========

MetadataFieldWidget::MetadataFieldWidget(const QString& key, const QString& value, 
                                         bool editable, QWidget* parent)
    : QWidget(parent)
    , m_key(key)
    , m_originalValue(value)
    , m_valueEdit(nullptr)
    , m_textEdit(nullptr)
    , m_removeButton(nullptr)
    , m_modified(false)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);
    
    // Key label (fixed width for alignment)
    QLabel* keyLabel = new QLabel(key + ":", this);
    keyLabel->setStyleSheet("color: #aaa; font-weight: bold;");
    keyLabel->setMinimumWidth(150);
    keyLabel->setMaximumWidth(150);
    layout->addWidget(keyLabel);
    
    // Determine if we need multiline edit
    m_multiline = value.length() > 100 || value.contains('\n');
    
    if (m_multiline) {
        m_textEdit = new QTextEdit(this);
        m_textEdit->setPlainText(value);
        m_textEdit->setReadOnly(!editable);
        m_textEdit->setMaximumHeight(80);
        connect(m_textEdit, &QTextEdit::textChanged, this, [this]() {
            m_modified = true;
            emit valueChanged(m_key, m_textEdit->toPlainText());
        });
        layout->addWidget(m_textEdit);
    } else {
        m_valueEdit = new QLineEdit(value, this);
        m_valueEdit->setReadOnly(!editable);
        connect(m_valueEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            m_modified = true;
            emit valueChanged(m_key, text);
        });
        layout->addWidget(m_valueEdit);
    }
    
    // Remove button (only for custom fields in edit mode)
    if (editable && key.startsWith("Custom:")) {
        m_removeButton = new QPushButton("✕", this);
        m_removeButton->setMaximumWidth(30);
        m_removeButton->setStyleSheet("QPushButton { color: #ff6b6b; font-weight: bold; }");
        connect(m_removeButton, &QPushButton::clicked, this, [this]() {
            emit removeRequested(m_key);
        });
        layout->addWidget(m_removeButton);
    }
}

QString MetadataFieldWidget::value() const {
    if (m_textEdit) {
        return m_textEdit->toPlainText();
    } else if (m_valueEdit) {
        return m_valueEdit->text();
    }
    return QString();
}

void MetadataFieldWidget::setValue(const QString& value) {
    if (m_textEdit) {
        m_textEdit->setPlainText(value);
    } else if (m_valueEdit) {
        m_valueEdit->setText(value);
    }
    m_originalValue = value;
    m_modified = false;
}

void MetadataFieldWidget::setEditable(bool editable) {
    if (m_textEdit) {
        m_textEdit->setReadOnly(!editable);
    } else if (m_valueEdit) {
        m_valueEdit->setReadOnly(!editable);
    }
    
    if (m_removeButton) {
        m_removeButton->setVisible(editable);
    }
}

// ========== MetadataPanel Implementation ==========

MetadataPanel::MetadataPanel(QWidget* parent)
    : QWidget(parent)
    , m_isEditing(false)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setupUI();
}

