#pragma once

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QString>

namespace PhotoGuru {

class DarkTheme {
public:
    static void apply(QApplication& app) {
        // Use Fusion style as base
        app.setStyle(QStyleFactory::create("Fusion"));
        
        // Adobe-like dark palette
        QPalette darkPalette;
        
        // Base colors - Adobe dark gray
        QColor darkGray(50, 50, 50);
        QColor darkerGray(32, 32, 32);
        QColor lightGray(100, 100, 100);
        QColor lighterGray(150, 150, 150);
        QColor accentBlue(31, 145, 255);  // Adobe blue accent
        QColor textColor(210, 210, 210);
        
        darkPalette.setColor(QPalette::Window, darkGray);
        darkPalette.setColor(QPalette::WindowText, textColor);
        darkPalette.setColor(QPalette::Base, darkerGray);
        darkPalette.setColor(QPalette::AlternateBase, darkGray);
        darkPalette.setColor(QPalette::ToolTipBase, darkerGray);
        darkPalette.setColor(QPalette::ToolTipText, textColor);
        darkPalette.setColor(QPalette::Text, textColor);
        darkPalette.setColor(QPalette::Button, darkGray);
        darkPalette.setColor(QPalette::ButtonText, textColor);
        darkPalette.setColor(QPalette::BrightText, Qt::white);
        darkPalette.setColor(QPalette::Link, accentBlue);
        darkPalette.setColor(QPalette::Highlight, accentBlue);
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);
        
        // Disabled colors
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, lightGray);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, lightGray);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, lightGray);
        
        app.setPalette(darkPalette);
        
        // Custom stylesheet for fine-tuning
        app.setStyleSheet(R"(
            QToolTip {
                color: #ffffff;
                background-color: #2a2a2a;
                border: 1px solid #555555;
                padding: 4px;
            }
            
            QMenuBar {
                background-color: #323232;
                color: #d2d2d2;
                border-bottom: 1px solid #1e1e1e;
            }
            
            QMenuBar::item:selected {
                background-color: #1f91ff;
            }
            
            QMenu {
                background-color: #323232;
                color: #d2d2d2;
                border: 1px solid #1e1e1e;
            }
            
            QMenu::item:selected {
                background-color: #1f91ff;
            }
            
            QToolBar {
                background-color: #323232;
                border: none;
                spacing: 3px;
                padding: 3px;
            }
            
            QToolButton {
                background-color: transparent;
                border: none;
                padding: 5px;
                margin: 2px;
            }
            
            QToolButton:hover {
                background-color: #505050;
                border-radius: 3px;
            }
            
            QToolButton:pressed {
                background-color: #1f91ff;
            }
            
            QDockWidget {
                titlebar-close-icon: url(:/icons/close.svg);
                titlebar-normal-icon: url(:/icons/float.svg);
            }
            
            QDockWidget::title {
                background-color: #2a2a2a;
                padding: 6px;
                text-align: left;
            }
            
            QScrollBar:vertical {
                background-color: #2a2a2a;
                width: 14px;
                margin: 0px;
            }
            
            QScrollBar::handle:vertical {
                background-color: #646464;
                min-height: 20px;
                border-radius: 7px;
                margin: 2px;
            }
            
            QScrollBar::handle:vertical:hover {
                background-color: #808080;
            }
            
            QScrollBar:horizontal {
                background-color: #2a2a2a;
                height: 14px;
                margin: 0px;
            }
            
            QScrollBar::handle:horizontal {
                background-color: #646464;
                min-width: 20px;
                border-radius: 7px;
                margin: 2px;
            }
            
            QScrollBar::handle:horizontal:hover {
                background-color: #808080;
            }
            
            QScrollBar::add-line, QScrollBar::sub-line {
                border: none;
                background: none;
            }
            
            QSplitter::handle {
                background-color: #1e1e1e;
            }
            
            QSplitter::handle:hover {
                background-color: #1f91ff;
            }
            
            QListWidget, QTreeWidget, QTableWidget {
                background-color: #2a2a2a;
                alternate-background-color: #323232;
                border: 1px solid #1e1e1e;
            }
            
            QListWidget::item:selected, QTreeWidget::item:selected, QTableWidget::item:selected {
                background-color: #1f91ff;
            }
            
            QHeaderView::section {
                background-color: #323232;
                color: #d2d2d2;
                padding: 5px;
                border: 1px solid #1e1e1e;
            }
            
            QLineEdit, QTextEdit, QPlainTextEdit {
                background-color: #2a2a2a;
                color: #d2d2d2;
                border: 1px solid #505050;
                padding: 3px;
                selection-background-color: #1f91ff;
            }
            
            QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
                border: 1px solid #1f91ff;
            }
            
            QPushButton {
                background-color: #505050;
                color: #d2d2d2;
                border: none;
                padding: 6px 12px;
                border-radius: 3px;
            }
            
            QPushButton:hover {
                background-color: #646464;
            }
            
            QPushButton:pressed {
                background-color: #1f91ff;
            }
            
            QPushButton:disabled {
                background-color: #3c3c3c;
                color: #787878;
            }
            
            QSlider::groove:horizontal {
                background-color: #2a2a2a;
                height: 4px;
                border-radius: 2px;
            }
            
            QSlider::handle:horizontal {
                background-color: #1f91ff;
                width: 14px;
                margin: -5px 0;
                border-radius: 7px;
            }
            
            QSlider::handle:horizontal:hover {
                background-color: #4fa8ff;
            }
            
            QStatusBar {
                background-color: #2a2a2a;
                color: #d2d2d2;
            }
            
            QProgressBar {
                background-color: #2a2a2a;
                border: 1px solid #505050;
                border-radius: 3px;
                text-align: center;
            }
            
            QProgressBar::chunk {
                background-color: #1f91ff;
                border-radius: 2px;
            }
        )");
    }
    
    // Icon colors for custom icons
    static QColor iconColor() { return QColor(210, 210, 210); }
    static QColor iconColorHover() { return QColor(255, 255, 255); }
    static QColor accentColor() { return QColor(31, 145, 255); }
    static QColor backgroundColor() { return QColor(50, 50, 50); }
    static QColor darkerBackgroundColor() { return QColor(32, 32, 32); }
};

} // namespace PhotoGuru
