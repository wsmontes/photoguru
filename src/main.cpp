#include "ui/MainWindow.h"
#include "ui/DarkTheme.h"
#include "ml/PythonBridge.h"
#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

using namespace PhotoGuru;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application info
    app.setOrganizationName("PhotoGuru");
    app.setOrganizationDomain("photoguru.ai");
    app.setApplicationName("PhotoGuru Viewer");
    app.setApplicationVersion("1.0.0");
    
    // Apply dark theme
    DarkTheme::apply(app);
    
    // Initialize Python bridge
    QString agentPath = QDir::currentPath() + "/python/agent_v2.py";
    
    // Check if agent script exists
    if (!QFileInfo::exists(agentPath)) {
        // Try parent directory
        agentPath = QDir::currentPath() + "/../python/agent_v2.py";
        
        if (!QFileInfo::exists(agentPath)) {
            qWarning() << "Warning: agent_v2.py not found. AI features will be disabled.";
            qWarning() << "Searched in:" << QDir::currentPath();
        } else {
            qDebug() << "Found agent_v2.py at:" << agentPath;
            if (!PythonBridge::instance().initialize(agentPath)) {
                qWarning() << "Failed to initialize Python bridge";
            }
        }
    } else {
        qDebug() << "Found agent_v2.py at:" << agentPath;
        if (!PythonBridge::instance().initialize(agentPath)) {
            qWarning() << "Failed to initialize Python bridge";
        }
    }
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    // If path provided as argument, open it
    if (argc > 1) {
        QString path = QString::fromUtf8(argv[1]);
        QFileInfo info(path);
        
        if (info.isDir()) {
            mainWindow.loadDirectory(path);
        } else if (info.isFile()) {
            // Load single file by loading its parent directory
            // and selecting the file
            QString directory = info.absolutePath();
            mainWindow.loadDirectory(directory);
            // The file will be visible in the grid and can be selected
        }
    }
    
    int result = app.exec();
    
    // Cleanup
    PythonBridge::instance().shutdown();
    
    return result;
}
