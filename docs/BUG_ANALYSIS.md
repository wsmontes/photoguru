# PhotoGuru - Análise Profunda de Bugs e Problemas

**Data:** 4 de Janeiro de 2026  
**Tipo:** Análise técnica real de código  

---

## 🐛 BUGS CRÍTICOS ENCONTRADOS

### 1. **ImageViewer::loadImage() BLOQUEIA A UI** 🔴 CRÍTICO
**Arquivo:** `src/ui/ImageViewer.cpp:21-44`

```cpp
void ImageViewer::loadImage(const QString& filepath) {
    m_isLoading = true;
    update();
    
    auto imageOpt = ImageLoader::instance().load(filepath, QSize(4000, 4000));
    // ^ BLOQUEIO: Carrega imagem de forma SÍNCRONA na thread principal!
    
    m_isLoading = false;
    // ...
}
```

**Problema:**
- Carregamento síncrono na thread de UI
- Imagens grandes (RAW, HEIF) podem levar segundos
- UI congela completamente durante carregamento
- "Loading indicator" NÃO aparece porque update() só executa DEPOIS do carregamento

**Impacto:**
- ⚠️ UX RUIM: App parece travado
- ⚠️ Sem feedback visual durante carregamento
- ⚠️ Impossível cancelar operação

**Solução Necessária:**
```cpp
// Usar QtConcurrent ou QThread
void ImageViewer::loadImage(const QString& filepath) {
    m_isLoading = true;
    update();  // Mostra spinner IMEDIATAMENTE
    
    // Carregar em background
    QFuture<std::optional<QImage>> future = QtConcurrent::run([this, filepath]() {
        return ImageLoader::instance().load(filepath, QSize(4000, 4000));
    });
    
    // Watcher para quando terminar
    auto* watcher = new QFutureWatcher<std::optional<QImage>>(this);
    connect(watcher, &QFutureWatcher::finished, this, [this, watcher, filepath]() {
        m_isLoading = false;
        auto result = watcher->result();
        if (result) {
            m_image = *result;
            m_filepath = filepath;
            zoomToFit();
            emit imageLoaded(filepath);
        }
        update();
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}
```

---

### 2. **Delete Files NÃO VERIFICA SUCESSO** 🔴 CRÍTICO
**Arquivo:** `src/ui/MainWindow.cpp:824-850`

```cpp
void MainWindow::onDeleteFiles() {
    // ...
    for (const QString& file : selected) {
        // Move to trash on macOS
        QProcess::execute("osascript", {
            "-e", QString("tell application \"Finder\" to delete (POSIX file \"%1\")").arg(file)
        });
        // ^ NÃO VERIFICA SE FUNCIONOU!
        m_imageFiles.removeAll(file);  // Remove da lista SEMPRE
        deleted++;  // Conta como deletado SEMPRE
    }
```

**Problema:**
- `QProcess::execute()` retorna exit code mas é ignorado
- Se falhar (permissões, arquivo aberto, etc), remove da lista mesmo assim
- Usuário perde referência ao arquivo mas ele ainda existe
- Estado inconsistente entre UI e filesystem

**Impacto:**
- 🐛 Arquivo "desaparece" mas ainda existe
- 🐛 Contador de "deleted" está errado
- 🐛 Sem mensagem de erro se falhar

**Solução Necessária:**
```cpp
int deleted = 0;
QStringList failed;
for (const QString& file : selected) {
    int result = QProcess::execute("osascript", {
        "-e", QString("tell application \"Finder\" to delete (POSIX file \"%1\")").arg(file)
    });
    
    if (result == 0) {
        m_imageFiles.removeAll(file);
        deleted++;
    } else {
        failed << QFileInfo(file).fileName();
    }
}

if (!failed.isEmpty()) {
    QMessageBox::warning(this, "Delete Failed", 
        QString("Failed to delete %1 file(s):\n%2")
            .arg(failed.count())
            .arg(failed.join("\n")));
}
```

---

### 3. **Rename NÃO ATUALIZA ÍNDICE ATUAL** 🟡 MÉDIO
**Arquivo:** `src/ui/MainWindow.cpp:793-822`

```cpp
void MainWindow::onRenameFile() {
    // ...
    if (QFile::rename(currentFile, newPath)) {
        m_imageFiles[m_currentIndex] = newPath;  // Atualiza lista
        m_thumbnailGrid->setImages(m_imageFiles);  // Atualiza grid
        m_imageViewer->loadImage(newPath);  // Recarrega imagem
        // ^ MAS m_currentIndex ainda aponta para o índice antigo!
        // Se o sort order mudar, índice fica errado
    }
}
```

**Problema:**
- Depois de renomear, se sorting = ByName, arquivo muda de posição
- `m_currentIndex` não é atualizado
- Navegação (next/prev) usa índice errado
- Pode pular para imagem errada

**Impacto:**
- 🐛 Navegação quebrada após rename
- 🐛 Arrow keys vão para imagem errada

**Solução Necessária:**
```cpp
if (QFile::rename(currentFile, newPath)) {
    m_imageFiles[m_currentIndex] = newPath;
    m_thumbnailGrid->setImages(m_imageFiles);
    
    // Encontrar novo índice após sort
    m_currentIndex = m_imageFiles.indexOf(newPath);
    if (m_currentIndex >= 0) {
        m_thumbnailGrid->selectImage(m_currentIndex);
        m_imageViewer->loadImage(newPath);
    }
}
```

---

### 4. **Copy/Move NÃO TEM PROGRESS BAR** 🟡 MÉDIO
**Arquivo:** `src/ui/MainWindow.cpp:735-790`

```cpp
void MainWindow::onCopyFiles() {
    // ...
    for (const QString& file : selected) {
        QString destPath = destDir + "/" + QFileInfo(file).fileName();
        QFile::copy(file, destPath);  // SÍNCRONO!
        // ^ Pode levar MUITO tempo com arquivos grandes
        copied++;
    }
}
```

**Problema:**
- Operação síncrona na UI thread
- Sem feedback de progresso
- Sem cancelamento
- UI congela durante cópia de arquivos grandes

**Impacto:**
- ⚠️ UX RUIM: App congela ao copiar RAW de 50MB
- ⚠️ Sem como cancelar
- ⚠️ Sem estimativa de tempo

**Solução Necessária:**
```cpp
// Criar QProgressDialog
auto* progress = new QProgressDialog("Copying files...", "Cancel", 0, selected.count(), this);
progress->setWindowModality(Qt::WindowModal);

// Copiar em background
QFuture<int> future = QtConcurrent::run([selected, destDir, progress]() {
    int copied = 0;
    for (const QString& file : selected) {
        if (progress->wasCanceled()) break;
        
        QString destPath = destDir + "/" + QFileInfo(file).fileName();
        if (QFile::copy(file, destPath)) {
            copied++;
        }
        QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, copied));
    }
    return copied;
});
```

---

### 5. **ThumbnailGrid GERA THUMBNAILS NA UI THREAD** 🔴 CRÍTICO
**Arquivo:** `src/ui/ThumbnailGrid.cpp:122`

```cpp
void ThumbnailGrid::loadThumbnails() {
    for (int i = 0; i < m_imagePaths.count(); ++i) {
        const QString& path = m_imagePaths[i];
        
        if (m_thumbnailCache.contains(path)) continue;
        
        QtConcurrent::run([this, path, i]() {
            QPixmap thumb = generateThumbnail(path);  // Em background ✅
            
            QMetaObject::invokeMethod(this, [this, path, i, thumb]() {
                m_thumbnailCache.insert(path, new QPixmap(thumb));
                // ^ OK, mas...
                
                if (i < count()) {
                    QListWidgetItem* item = this->item(i);
                    if (item && item->data(Qt::UserRole).toString() == path) {
                        item->setIcon(QIcon(thumb));
                        // ^ Pode estar acessando item JÁ DELETADO!
                    }
                }
            }, Qt::QueuedConnection);
        });
    }
}
```

**Problema:**
- Race condition: Item pode ser deletado antes do callback
- Sem verificação se `this` ainda é válido
- Crash potencial se grid for destruído durante load

**Impacto:**
- 🐛 Crash ao fechar janela enquanto carrega thumbnails
- 🐛 Crash ao trocar de pasta rapidamente

**Solução Necessária:**
```cpp
// Adicionar contador de tasks ativas
void ThumbnailGrid::loadThumbnails() {
    m_loadingTasks++;
    
    QtConcurrent::run([this, path, i]() {
        QPixmap thumb = generateThumbnail(path);
        
        // Usar QPointer para detectar se objeto foi destruído
        QMetaObject::invokeMethod(this, [this, path, i, thumb]() {
            m_loadingTasks--;
            
            // Verificar se ainda é válido
            if (!this || m_imagePaths.isEmpty()) return;
            
            m_thumbnailCache.insert(path, new QPixmap(thumb));
            
            if (i < count()) {
                QListWidgetItem* item = this->item(i);
                if (item && item->data(Qt::UserRole).toString() == path) {
                    item->setIcon(QIcon(thumb));
                }
            }
        }, Qt::QueuedConnection);
    });
}

// No destrutor:
~ThumbnailGrid() {
    // Esperar tasks terminarem
    while (m_loadingTasks > 0) {
        QApplication::processEvents();
    }
}
```

---

## 🎨 PROBLEMAS DE UI

### 1. **Loading Indicator NÃO APARECE** 🟡
**Arquivo:** `src/ui/ImageViewer.cpp:225-243`

```cpp
void ImageViewer::drawLoadingIndicator(QPainter& painter) {
    if (!m_isLoading) return;  // ← NUNCA É TRUE durante paint!
    
    // Desenha spinner...
}
```

**Problema:**
- `m_isLoading` é setado para `true` mas `update()` só executa DEPOIS do load
- Loading acontece na mesma thread, então paint event não é disparado
- Spinner nunca aparece

**Fix:** Usar carregamento assíncrono (já mencionado acima)

---

### 2. **Ícones SVG Faltando**
**Console:**
```
qt.svg: Cannot open file ':/icons/close.svg'
qt.svg: Cannot open file ':/icons/float.svg'
```

**Problema:**
- `resources/resources.qrc` está vazio
- Código referencia ícones que não existem
- Deixa warnings no console

**Solução:**
1. Remover referências aos ícones OU
2. Adicionar ícones ao resources.qrc

---

### 3. **Sem Feedback Visual em File Operations**

**Problema:**
- Copy/Move/Delete acontecem sem feedback imediato
- Usuário não sabe se funcionou
- statusBar() message desaparece rápido demais

**Solução:**
- Adicionar QProgressDialog
- Toast notifications
- Som de confirmação

---

## 🚨 PROBLEMAS DE UX

### 1. **Não dá para cancelar operações longas**
- Carregamento de imagem grande
- Cópia de muitos arquivos
- Geração de thumbnails

### 2. **Sem undo para delete**
- Delete move para Trash mas sem confirmação visual
- Sem como desfazer (além de ir no Finder)

### 3. **Multi-select confuso**
- Não fica claro quantos arquivos estão selecionados
- Operações afetam seleção ou arquivo atual?
- Comportamento inconsistente

### 4. **Keyboard shortcuts sem documentação**
- Nenhuma indicação visual dos shortcuts
- Sem menu Help
- Tooltips faltando

### 5. **Sem search/filter**
- Muitos arquivos = difícil de achar
- Sem busca por nome
- Sem filtro por data/tipo

---

## 📊 RESUMO DE PRIORIDADES

### 🔴 CRÍTICO - Fix Imediato
1. ✅ ImageViewer carregamento assíncrono
2. ✅ Delete verificar sucesso
3. ✅ ThumbnailGrid race conditions

### 🟡 IMPORTANTE - Fix em 1-2 dias
4. ✅ Copy/Move com progress bar
5. ✅ Rename atualizar índice
6. ✅ Loading indicator funcionar

### 🟢 MELHORIAS - Futuro
7. ⚠️ Undo/Redo system
8. ⚠️ Search/Filter
9. ⚠️ Keyboard shortcuts help
10. ⚠️ Multi-select feedback visual

---

## 🎯 PRÓXIMA AÇÃO

Implementar fixes para bugs críticos:
1. ImageViewer async loading
2. Delete error handling  
3. ThumbnailGrid safety

Tempo estimado: 3-4 horas de trabalho focado
