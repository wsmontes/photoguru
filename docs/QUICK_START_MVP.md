# Quick Start - Implementação MVP

## 🚀 COMECE HOJE - CHECKLIST EXECUTÁVEL

Esta é a versão "executável" do plano. Copie e cole os comandos.

---

## PASSO 1: Backup e Limpeza (30 minutos)

```bash
cd /Users/wagnermontes/Documents/GitHub/_TESTS/Photoguru-viewer

# Criar backup do estado atual
git checkout -b backup-full-version
git commit -am "Backup before MVP refactoring"
git push origin backup-full-version

# Criar branch MVP
git checkout -b mvp-phase1

# Criar branch para features futuras
git checkout -b future-features
git checkout mvp-phase1
```

---

## PASSO 2: Simplificar Python Backend (2 horas)

### 2.1: Criar novo agent simplificado

```bash
# Renomear agent antigo
mv agent_v2.py agent_v2_full.py.backup

# Criar novo agent MVP
cat > agent_mvp.py << 'EOF'
#!/usr/bin/env python3
"""PhotoGuru MVP - Simple AI Analysis"""

import os
import sys
import json
import base64
import subprocess
from io import BytesIO
from pathlib import Path
from PIL import Image

def analyze_photo(filepath: str, api_key: str = None) -> dict:
    """Analisa foto usando GPT-4 Vision API"""
    
    # Verificar se API key existe
    if not api_key:
        api_key = os.getenv("OPENAI_API_KEY")
    
    if not api_key:
        return {
            "error": "API key not configured",
            "title": Path(filepath).stem,
            "description": "Configure OpenAI API key to enable AI analysis",
            "tags": ["unanalyzed"]
        }
    
    try:
        # Importar OpenAI
        import openai
        
        # Carregar e redimensionar imagem
        img = Image.open(filepath)
        if max(img.size) > 2048:
            img.thumbnail((2048, 2048), Image.Resampling.LANCZOS)
        
        # Converter para base64
        buffered = BytesIO()
        img.save(buffered, format="JPEG", quality=85)
        img_base64 = base64.b64encode(buffered.getvalue()).decode()
        
        # Chamar API
        client = openai.OpenAI(api_key=api_key)
        
        response = client.chat.completions.create(
            model="gpt-4-vision-preview",
            messages=[{
                "role": "user",
                "content": [
                    {
                        "type": "text",
                        "text": """Analyze this photo. Provide:
1. A descriptive title (max 60 characters)
2. A brief description (max 150 characters)  
3. 5 relevant searchable tags

Return ONLY valid JSON:
{"title": "...", "description": "...", "tags": ["...", "...", "...", "...", "..."]}"""
                    },
                    {
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/jpeg;base64,{img_base64}",
                            "detail": "low"  # Cheaper
                        }
                    }
                ]
            }],
            max_tokens=300
        )
        
        # Parse resposta
        content = response.choices[0].message.content
        
        # Extrair JSON se vier com texto extra
        if "{" in content and "}" in content:
            start = content.index("{")
            end = content.rindex("}") + 1
            content = content[start:end]
        
        result = json.loads(content)
        
        # Validar estrutura
        if "title" not in result:
            result["title"] = Path(filepath).stem
        if "description" not in result:
            result["description"] = ""
        if "tags" not in result:
            result["tags"] = []
        
        return result
        
    except ImportError:
        return {
            "error": "openai package not installed",
            "title": Path(filepath).stem,
            "description": "Run: pip install openai",
            "tags": []
        }
    except Exception as e:
        return {
            "error": str(e),
            "title": Path(filepath).stem,
            "description": f"Analysis failed: {str(e)[:100]}",
            "tags": []
        }


def write_metadata(filepath: str, metadata: dict):
    """Grava metadados usando exiftool"""
    
    cmd = ["exiftool", "-overwrite_original"]
    
    if "title" in metadata and metadata["title"]:
        cmd.extend([f"-Title={metadata['title']}"])
        cmd.extend([f"-XMP:Headline={metadata['title']}"])
    
    if "description" in metadata and metadata["description"]:
        cmd.extend([f"-Description={metadata['description']}"])
        cmd.extend([f"-XMP:Description={metadata['description']}"])
    
    if "tags" in metadata and metadata["tags"]:
        # Limpar tags existentes primeiro
        cmd.extend(["-Keywords="])
        # Adicionar novas
        for tag in metadata["tags"]:
            if tag:  # Ignorar tags vazias
                cmd.extend([f"-Keywords+={tag}"])
    
    cmd.append(filepath)
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return {"success": True}
    except subprocess.CalledProcessError as e:
        return {"success": False, "error": e.stderr}


def search_photos(directory: str, query: str) -> list:
    """Busca fotos por tag ou metadata"""
    
    cmd = [
        "exiftool",
        "-r",  # Recursivo
        "-if",
        f"$Keywords =~ /{query}/i or $Title =~ /{query}/i or $Description =~ /{query}/i",
        "-filename",
        "-s3",  # Output simples
        directory
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        files = [line.strip() for line in result.stdout.split("\n") if line.strip()]
        return files
    except Exception as e:
        return []


def main():
    if len(sys.argv) < 2:
        print(json.dumps({
            "error": "Usage: agent_mvp.py <command> [args]",
            "commands": ["analyze", "write", "search"]
        }))
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "analyze":
        if len(sys.argv) < 3:
            print(json.dumps({"error": "Usage: agent_mvp.py analyze <filepath>"}))
            sys.exit(1)
        
        filepath = sys.argv[2]
        api_key = sys.argv[3] if len(sys.argv) > 3 else None
        
        result = analyze_photo(filepath, api_key)
        print(json.dumps(result, indent=2, ensure_ascii=False))
    
    elif command == "write":
        if len(sys.argv) < 4:
            print(json.dumps({"error": "Usage: agent_mvp.py write <filepath> <json>"}))
            sys.exit(1)
        
        filepath = sys.argv[2]
        metadata = json.loads(sys.argv[3])
        
        result = write_metadata(filepath, metadata)
        print(json.dumps(result, indent=2))
    
    elif command == "search":
        if len(sys.argv) < 4:
            print(json.dumps({"error": "Usage: agent_mvp.py search <directory> <query>"}))
            sys.exit(1)
        
        directory = sys.argv[2]
        query = sys.argv[3]
        
        results = search_photos(directory, query)
        print(json.dumps(results, indent=2))
    
    else:
        print(json.dumps({"error": f"Unknown command: {command}"}))
        sys.exit(1)


if __name__ == "__main__":
    main()
EOF

chmod +x agent_mvp.py
```

### 2.2: Criar requirements simplificado

```bash
cat > requirements_mvp.txt << 'EOF'
# PhotoGuru MVP - Minimal Dependencies
openai>=1.10.0
pillow>=10.2.0
EOF
```

### 2.3: Testar novo agent

```bash
# Criar foto de teste
cp ~/Pictures/*.jpg /tmp/test_photo.jpg 2>/dev/null || \
curl -o /tmp/test_photo.jpg https://picsum.photos/800/600

# Testar análise (sem API key - deve falhar graciosamente)
python3 agent_mvp.py analyze /tmp/test_photo.jpg

# Esperado:
# {
#   "error": "API key not configured",
#   "title": "test_photo",
#   ...
# }
```

---

## PASSO 3: Desabilitar Features Complexas (1 hora)

### 3.1: Editar CMakeLists.txt

```bash
# Comentar componentes não essenciais
cat > /tmp/cmake_patch.txt << 'EOF'
# Disable complex features for MVP
# add_subdirectory(src/ui/MapView)
# add_subdirectory(src/ui/TimelineView)
# add_subdirectory(src/ui/SKPBrowser)
EOF
```

### 3.2: Editar MainWindow.h

```cpp
// Em src/ui/MainWindow.h, comentar:
// class MapView;
// class TimelineView;
// class SKPBrowser;

// E os membros:
// MapView* m_mapView;
// TimelineView* m_timelineView;
```

Manual: abra [src/ui/MainWindow.h](src/ui/MainWindow.h) e comente as linhas 17-19 e 89-91.

### 3.3: Editar MainWindow.cpp

Manual: comente qualquer referência a MapView, TimelineView, SKPBrowser em [src/ui/MainWindow.cpp](src/ui/MainWindow.cpp).

---

## PASSO 4: Adicionar Operações de Arquivo (4 horas)

### 4.1: Criar novo arquivo FileOperations.h

```bash
cat > src/core/FileOperations.h << 'EOF'
#pragma once

#include <QString>
#include <QList>

namespace PhotoGuru {

class FileOperations {
public:
    // Copiar arquivo para área de transferência
    static bool copyToClipboard(const QString& filepath);
    
    // Colar arquivo da área de transferência para diretório
    static QString pasteFromClipboard(const QString& destDir);
    
    // Mover arquivo
    static bool moveFile(const QString& source, const QString& dest);
    
    // Renomear arquivo
    static bool renameFile(const QString& filepath, const QString& newName);
    
    // Mover para lixeira (não deletar permanentemente)
    static bool moveToTrash(const QString& filepath);
    
    // Revelar arquivo no Finder/Explorer
    static void revealInFileManager(const QString& filepath);
    
    // Abrir com aplicação externa
    static void openWithExternal(const QString& filepath);
    
    // Operações em lote
    static int moveToTrashBatch(const QList<QString>& filepaths);
    static int renameBatch(const QList<QString>& filepaths, 
                          const QString& pattern);
};

} // namespace PhotoGuru
EOF
```

### 4.2: Criar implementação FileOperations.cpp

```bash
cat > src/core/FileOperations.cpp << 'EOF'
#include "FileOperations.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QDebug>

#ifdef Q_OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <objc/objc.h>
#include <objc/message.h>
#endif

namespace PhotoGuru {

bool FileOperations::copyToClipboard(const QString& filepath) {
    QClipboard* clipboard = QApplication::clipboard();
    QMimeData* mimeData = new QMimeData();
    
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(filepath);
    mimeData->setUrls(urls);
    
    clipboard->setMimeData(mimeData);
    return true;
}

QString FileOperations::pasteFromClipboard(const QString& destDir) {
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    
    if (!mimeData->hasUrls()) {
        return QString();
    }
    
    QList<QUrl> urls = mimeData->urls();
    if (urls.isEmpty()) {
        return QString();
    }
    
    QString sourceFile = urls.first().toLocalFile();
    QFileInfo fileInfo(sourceFile);
    QString destFile = destDir + "/" + fileInfo.fileName();
    
    if (QFile::copy(sourceFile, destFile)) {
        return destFile;
    }
    
    return QString();
}

bool FileOperations::moveFile(const QString& source, const QString& dest) {
    // Primeiro tentar renomear (mais rápido se no mesmo filesystem)
    if (QFile::rename(source, dest)) {
        return true;
    }
    
    // Se falhar, copiar e deletar
    if (QFile::copy(source, dest)) {
        return QFile::remove(source);
    }
    
    return false;
}

bool FileOperations::renameFile(const QString& filepath, const QString& newName) {
    QFileInfo fileInfo(filepath);
    QString newPath = fileInfo.dir().absolutePath() + "/" + newName;
    
    return QFile::rename(filepath, newPath);
}

bool FileOperations::moveToTrash(const QString& filepath) {
#ifdef Q_OS_MACOS
    // Usar NSFileManager no macOS
    QProcess process;
    process.start("osascript", QStringList() 
        << "-e"
        << QString("tell application \"Finder\" to delete POSIX file \"%1\"")
           .arg(filepath));
    
    process.waitForFinished();
    return process.exitCode() == 0;
    
#elif defined(Q_OS_LINUX)
    // Usar gio trash no Linux
    QProcess process;
    process.start("gio", QStringList() << "trash" << filepath);
    process.waitForFinished();
    return process.exitCode() == 0;
    
#elif defined(Q_OS_WIN)
    // Windows - usar IFileOperation
    // TODO: Implementar para Windows
    return QFile::remove(filepath);
#else
    return QFile::remove(filepath);
#endif
}

void FileOperations::revealInFileManager(const QString& filepath) {
#ifdef Q_OS_MACOS
    QProcess::execute("open", QStringList() << "-R" << filepath);
#elif defined(Q_OS_LINUX)
    // Tentar diferentes file managers
    QProcess::execute("nautilus", QStringList() << "--select" << filepath);
#elif defined(Q_OS_WIN)
    QProcess::execute("explorer", QStringList() << "/select," << filepath);
#else
    // Fallback: abrir diretório pai
    QFileInfo fileInfo(filepath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().absolutePath()));
#endif
}

void FileOperations::openWithExternal(const QString& filepath) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(filepath));
}

int FileOperations::moveToTrashBatch(const QList<QString>& filepaths) {
    int count = 0;
    for (const QString& filepath : filepaths) {
        if (moveToTrash(filepath)) {
            count++;
        }
    }
    return count;
}

int FileOperations::renameBatch(const QList<QString>& filepaths, 
                                const QString& pattern) {
    int count = 0;
    int index = 1;
    
    for (const QString& filepath : filepaths) {
        QFileInfo fileInfo(filepath);
        QString newName = pattern;
        
        // Substituir placeholders
        newName.replace("{n}", QString::number(index));
        newName.replace("{N}", QString::number(index).rightJustified(4, '0'));
        newName.replace("{ext}", fileInfo.suffix());
        
        if (!newName.contains('.')) {
            newName += "." + fileInfo.suffix();
        }
        
        if (renameFile(filepath, newName)) {
            count++;
        }
        
        index++;
    }
    
    return count;
}

} // namespace PhotoGuru
EOF
```

### 4.3: Adicionar ao CMakeLists.txt

```bash
# Editar CMakeLists.txt, adicionar na seção SOURCES:
# src/core/FileOperations.cpp
```

Manual: abra [CMakeLists.txt](CMakeLists.txt) e adicione `src/core/FileOperations.cpp` na lista de SOURCES (linha ~50).

---

## PASSO 5: Adicionar Atalhos de Teclado (2 horas)

### 5.1: Editar MainWindow.h - adicionar slots

```cpp
// Em src/ui/MainWindow.h, adicionar:

private slots:
    // ... slots existentes ...
    
    // FILE OPERATIONS (MVP)
    void onCopyFile();
    void onPasteFile();
    void onMoveFile();
    void onRenameFile();
    void onDeleteFile();
    void onRevealInFinder();
    void onOpenWithExternal();
```

### 5.2: Editar MainWindow.cpp - implementar slots

```cpp
// No arquivo src/ui/MainWindow.cpp, adicionar:

#include "core/FileOperations.h"

void MainWindow::onCopyFile() {
    QString currentFile = m_imageViewer->currentImage();
    if (currentFile.isEmpty()) return;
    
    FileOperations::copyToClipboard(currentFile);
    m_statusBar->showMessage("File copied to clipboard", 2000);
}

void MainWindow::onPasteFile() {
    QString currentDir = m_currentDirectory;
    if (currentDir.isEmpty()) return;
    
    QString pastedFile = FileOperations::pasteFromClipboard(currentDir);
    if (!pastedFile.isEmpty()) {
        m_statusBar->showMessage("File pasted: " + QFileInfo(pastedFile).fileName(), 2000);
        refreshViews();
    }
}

void MainWindow::onMoveFile() {
    // TODO: Dialog para escolher destino
    m_statusBar->showMessage("Move file - TODO", 2000);
}

void MainWindow::onRenameFile() {
    QString currentFile = m_imageViewer->currentImage();
    if (currentFile.isEmpty()) return;
    
    QFileInfo fileInfo(currentFile);
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename File",
                                           "New name:",
                                           QLineEdit::Normal,
                                           fileInfo.fileName(),
                                           &ok);
    
    if (ok && !newName.isEmpty()) {
        if (FileOperations::renameFile(currentFile, newName)) {
            m_statusBar->showMessage("File renamed", 2000);
            refreshViews();
        } else {
            QMessageBox::warning(this, "Error", "Failed to rename file");
        }
    }
}

void MainWindow::onDeleteFile() {
    QString currentFile = m_imageViewer->currentImage();
    if (currentFile.isEmpty()) return;
    
    QFileInfo fileInfo(currentFile);
    
    int ret = QMessageBox::question(this, "Move to Trash",
                                    QString("Move '%1' to trash?").arg(fileInfo.fileName()),
                                    QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (FileOperations::moveToTrash(currentFile)) {
            m_statusBar->showMessage("File moved to trash", 2000);
            onNextImage();  // Ir para próxima foto
            refreshViews();
        } else {
            QMessageBox::warning(this, "Error", "Failed to move file to trash");
        }
    }
}

void MainWindow::onRevealInFinder() {
    QString currentFile = m_imageViewer->currentImage();
    if (currentFile.isEmpty()) return;
    
    FileOperations::revealInFileManager(currentFile);
}

void MainWindow::onOpenWithExternal() {
    QString currentFile = m_imageViewer->currentImage();
    if (currentFile.isEmpty()) return;
    
    FileOperations::openWithExternal(currentFile);
}

// No construtor, adicionar atalhos:
void MainWindow::createMenuBar() {
    // ... código existente ...
    
    // FILE OPERATIONS
    QAction* copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::onCopyFile);
    
    QAction* pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::onPasteFile);
    
    QAction* renameAction = new QAction("Rename...", this);
    renameAction->setShortcut(Qt::Key_F2);
    connect(renameAction, &QAction::triggered, this, &MainWindow::onRenameFile);
    
    QAction* deleteAction = new QAction("Move to Trash", this);
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteFile);
    
    QAction* revealAction = new QAction("Reveal in Finder", this);
    revealAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_R);
    connect(revealAction, &QAction::triggered, this, &MainWindow::onRevealInFinder);
    
    // Adicionar ao menu File
    fileMenu->addSeparator();
    fileMenu->addAction(copyAction);
    fileMenu->addAction(pasteAction);
    fileMenu->addAction(renameAction);
    fileMenu->addAction(deleteAction);
    fileMenu->addSeparator();
    fileMenu->addAction(revealAction);
}
```

---

## PASSO 6: Compilar e Testar (30 min)

```bash
# Limpar build anterior
rm -rf build
mkdir build
cd build

# Configurar
cmake ..

# Compilar
make -j$(sysctl -n hw.ncpu)

# Testar
./PhotoGuruViewer
```

### Testes Manuais:

1. ✅ Abrir pasta com fotos
2. ✅ Navegar com setas
3. ✅ Pressionar Cmd+C (copiar)
4. ✅ Abrir nova pasta
5. ✅ Pressionar Cmd+V (colar)
6. ✅ Selecionar foto, pressionar F2 (renomear)
7. ✅ Pressionar Delete (mover para lixeira)
8. ✅ Verificar se foto foi para lixeira
9. ✅ Pressionar Cmd+Shift+R (revelar no Finder)

---

## PRÓXIMOS PASSOS

Após esses passos funcionarem:

1. **Sprint 2**: Implementar filtros e ordenação
2. **Sprint 3**: Melhorar MetadataPanel
3. **Sprint 4**: Integrar agent_mvp.py simplificado
4. **Sprint 5**: Beta testing

---

## ❌ TROUBLESHOOTING

### Problema: "FileOperations.h: No such file"
```bash
# Verificar se arquivo foi criado
ls -la src/core/FileOperations.*

# Se não existir, criar manualmente
```

### Problema: "Undefined reference to FileOperations"
```bash
# Verificar se está no CMakeLists.txt
grep FileOperations CMakeLists.txt

# Adicionar manualmente se necessário
```

### Problema: Build falha
```bash
# Ver erros completos
cd build
make VERBOSE=1 2>&1 | tee build.log
cat build.log
```

---

## 📊 PROGRESSO

Marque conforme completar:

- [ ] Passo 1: Backup e branches
- [ ] Passo 2: Agent MVP criado
- [ ] Passo 3: Features complexas desabilitadas
- [ ] Passo 4: FileOperations implementado
- [ ] Passo 5: Atalhos adicionados
- [ ] Passo 6: Build e teste OK

**Tempo estimado total:** 6-8 horas de trabalho focado

---

🎯 **Lembre-se:** O objetivo é fazer funcionar, não perfeito. Commit frequente!

```bash
git add .
git commit -m "MVP Phase 1: Basic file operations implemented"
git push origin mvp-phase1
```
