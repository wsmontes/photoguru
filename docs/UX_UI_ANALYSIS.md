# PhotoGuru - Análise de Usabilidade e Interface (UX/UI)

**Analista:** Ex-Chefe de Produto Adobe Lightroom  
**Data:** 4 de Janeiro de 2026  
**Versão Analisada:** MVP 1.0.0  
**Profundidade:** Análise Técnica com Experiência de Produto Adobe

---

## 🎯 RESUMO EXECUTIVO

O PhotoGuru demonstra **ambição correta** ao tentar replicar o workflow profissional do Lightroom, mas apresenta **lacunas críticas de UX** que comprometem a adoção por fotógrafos profissionais. A arquitetura técnica é sólida, mas a experiência do usuário precisa de refinamento significativo antes de ser competitiva.

### Pontuação Geral
- **Arquitetura Técnica:** 8/10 ⭐⭐⭐⭐⭐⭐⭐⭐
- **Design Visual:** 6/10 ⭐⭐⭐⭐⭐⭐
- **Usabilidade:** 5/10 ⭐⭐⭐⭐⭐
- **Workflow Profissional:** 4/10 ⭐⭐⭐⭐
- **Prontidão para Mercado:** 4/10 ⭐⭐⭐⭐

---

## ✅ PONTOS FORTES

### 1. **Estrutura de Layout Inteligente**
**O QUE ESTÁ BOM:**
- Splitter vertical com `ImageViewer` (top) e `ThumbnailGrid` (bottom) é **excelente**
- Proporção 3:1 entre viewer e thumbnails é próxima do ideal do Lightroom
- Sistema de dock widgets replicando painéis laterais do Lightroom é **correto**

**EVIDÊNCIA NO CÓDIGO:**
```cpp
centralSplitter->setStretchFactor(0, 3);  // ImageViewer
centralSplitter->setStretchFactor(1, 1);  // ThumbnailGrid
```

**AVALIAÇÃO:** ⭐⭐⭐⭐⭐ 9/10
- Esta é a decisão de layout **mais acertada** do projeto
- Fotógrafos profissionais precisam dessa hierarquia visual

### 2. **Atalhos de Teclado Bem Mapeados**
**O QUE ESTÁ BOM:**
```
✅ Ctrl+Shift+O - Abrir diretório
✅ F2 - Renomear
✅ Delete - Deletar
✅ Ctrl+R - Reveal in Finder
✅ F11 - Fullscreen
✅ Arrows - Navegação
✅ +/- - Zoom
✅ F - Fit to window
✅ Ctrl+0 - 100%
```

**AVALIAÇÃO:** ⭐⭐⭐⭐⭐ 8/10
- Cobertura de atalhos é **muito boa**
- Compatibilidade com convenções Adobe é **alta**

### 3. **Dark Theme Profissional**
**O QUE ESTÁ BOM:**
```cpp
QColor darkGray(50, 50, 50);
QColor darkerGray(32, 32, 32);
QColor accentBlue(31, 145, 255);  // Adobe blue!
```

**AVALIAÇÃO:** ⭐⭐⭐⭐⭐ 7/10
- Cores escolhidas são próximas do Lightroom Classic
- Accent blue (#1f91ff) é **quase idêntico** ao Adobe blue (#1473E6)

### 4. **Operações de Arquivo Essenciais**
**O QUE ESTÁ BOM:**
- Copy, Move, Rename, Delete implementados
- Reveal in Finder (feature macOS nativa)
- Open With external editor

**AVALIAÇÃO:** ⭐⭐⭐⭐⭐ 6/10
- Funcionalidades corretas, mas **falta refinamento**

---

## ❌ PROBLEMAS CRÍTICOS DE USABILIDADE

### 🚨 PROBLEMA 1: Ausência de "Library" vs "Develop" Modules
**SEVERIDADE:** 🔴 **CRÍTICA**

**O PROBLEMA:**
O Lightroom tem **clara separação** entre:
- **Library Module:** Organização, seleção, metadata, keywords
- **Develop Module:** Edição de imagem, ajustes, presets
- **Map Module:** Geolocalização
- **Print/Web/Slideshow:** Outputs

**NO PHOTOGURU:**
```cpp
QTabWidget* m_centralTabs;  // Tabs existem mas...
ImageViewer* m_imageViewer;
MapView* m_mapView;
TimelineView* m_timelineView;
```

**PROBLEMA REAL:**
- Não há **conceito de módulos**
- Tabs estão criadas mas sem **transição clara de workflow**
- Fotógrafo não sabe "onde está" no processo

**IMPACTO:** Usuários do Lightroom ficam **perdidos**

**SOLUÇÃO RECOMENDADA:**
```cpp
enum class WorkspaceMode {
    Library,    // Thumbnails, metadata, sorting
    Develop,    // Full image + editing tools
    Map,        // Geolocation
    Compare     // Side-by-side comparison
};

void MainWindow::switchMode(WorkspaceMode mode) {
    // Reconfigure entire layout based on mode
    // Library: maximize thumbnails, show filters
    // Develop: maximize viewer, show editing panels
}
```

**ESFORÇO:** 3-4 semanas | **PRIORIDADE:** P0 (Bloqueador)

---

### 🚨 PROBLEMA 2: Navegação de Imagem Confusa
**SEVERIDADE:** 🟡 **ALTA**

**O PROBLEMA:**
```cpp
// ImageViewer emite signals
emit nextImageRequested();
emit previousImageRequested();

// MainWindow conecta
connect(m_imageViewer, &ImageViewer::nextImageRequested,
        this, &MainWindow::onNextImage);
```

**MAS:** Como usuário sabe qual imagem está selecionada?

**NO LIGHTROOM:**
- Thumbnail da imagem atual tem **borda highlight grossa**
- Status bar mostra "Image 47 of 231"
- Filmstrip sempre visível mostrando contexto

**NO PHOTOGURU:**
```cpp
// Status bar genérico
m_statusBar->showMessage("Ready - Open a directory...");
```

**FALTANDO:**
- Indicador visual de imagem atual
- Contador de posição (5/127)
- Highlight sincronizado entre viewer e thumbnails

**IMPACTO:** Usuário perde contexto, não sabe onde está na coleção

**SOLUÇÃO RECOMENDADA:**
```cpp
void ThumbnailGrid::setCurrentIndex(int index) {
    // Highlight current thumbnail with THICK border
    QListWidgetItem* item = this->item(index);
    item->setBackground(QColor(31, 145, 255, 50));  // Adobe blue
    item->setData(Qt::UserRole + 1, true);  // Mark as current
    
    // Scroll to ensure visible
    scrollToItem(item);
}

void MainWindow::updateStatusBar() {
    int current = m_currentIndex + 1;
    int total = m_imageFiles.size();
    
    QString filename = QFileInfo(m_imageFiles[m_currentIndex]).fileName();
    QString status = QString("%1 (%2 of %3)")
        .arg(filename)
        .arg(current)
        .arg(total);
    
    m_statusBar->showMessage(status);
}
```

**ESFORÇO:** 1 semana | **PRIORIDADE:** P0 (Bloqueador)

---

### 🚨 PROBLEMA 3: Feedback Visual Insuficiente
**SEVERIDADE:** 🟡 **ALTA**

**O PROBLEMA:**
```cpp
bool m_isLoading = false;  // Flag existe...
void drawLoadingIndicator(QPainter& painter);  // Função existe...
```

**MAS:** Falta feedback em:
- ❌ Operações de cópia/move (sem progress bar)
- ❌ Deleção (sem confirmação visual)
- ❌ Renomeação (modal dialog? inline edit?)
- ❌ Operações longas (análise AI)

**NO LIGHTROOM:**
- Progress bar no topo mostra "Importing 127 photos..."
- Operações batch mostram % de conclusão
- Confirmações têm preview do que será afetado

**EXEMPLO PROBLEMÁTICO:**
```cpp
void MainWindow::onDeleteFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    // ... código de deleção ...
    // PROBLEMA: Sem confirmação, sem undo, sem feedback
}
```

**IMPACTO:** Usuários têm **medo de usar** features críticas

**SOLUÇÃO RECOMENDADA:**
```cpp
void MainWindow::onDeleteFiles() {
    QStringList selected = m_thumbnailGrid->selectedFiles();
    
    // CONFIRMATION DIALOG
    QMessageBox confirmBox(this);
    confirmBox.setIcon(QMessageBox::Warning);
    confirmBox.setWindowTitle("Move to Trash");
    confirmBox.setText(QString("Move %1 photo(s) to Trash?")
        .arg(selected.size()));
    confirmBox.setInformativeText("You can restore from Trash later.");
    
    // Show thumbnail previews
    // ... thumbnails grid inside dialog ...
    
    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    confirmBox.setDefaultButton(QMessageBox::Cancel);
    
    if (confirmBox.exec() == QMessageBox::Yes) {
        // PROGRESS BAR
        QProgressDialog progress("Moving to Trash...", 
                                 "Cancel", 0, selected.size(), this);
        progress.setWindowModality(Qt::WindowModal);
        
        for (int i = 0; i < selected.size(); i++) {
            progress.setValue(i);
            // ... delete file ...
        }
    }
}
```

**ESFORÇO:** 2 semanas | **PRIORIDADE:** P0 (Bloqueador)

---

### 🚨 PROBLEMA 4: Multi-Seleção Sem Feedback Claro
**SEVERIDADE:** 🟠 **MÉDIA**

**O PROBLEMA:**
```cpp
m_thumbnailGrid->setSelectionMode(QAbstractItemView::ExtendedSelection);
connect(m_thumbnailGrid, &QListWidget::itemSelectionChanged,
        this, &MainWindow::onThumbnailSelectionChanged);
```

**ExtendedSelection permite multi-select, MAS:**
- Usuário não sabe como fazer (Cmd+Click não é documentado)
- Não há contador de "23 photos selected"
- Operações batch não mostram preview

**NO LIGHTROOM:**
- Status bar: "23 photos selected"
- Thumbnails selecionadas têm **borda branca grossa**
- Shift+Click seleciona range
- Cmd+A seleciona todas (visíveis ou filtradas?)

**FALTANDO NO PHOTOGURU:**
```cpp
void MainWindow::onThumbnailSelectionChanged() {
    int count = m_thumbnailGrid->selectedFiles().count();
    
    if (count == 0) {
        m_statusBar->clearMessage();
    } else if (count == 1) {
        // Show single file info
    } else {
        m_statusBar->showMessage(QString("%1 photos selected").arg(count));
        
        // Update toolbar buttons state
        m_copyAction->setEnabled(true);
        m_moveAction->setEnabled(true);
        m_deleteAction->setEnabled(true);
    }
}
```

**ESFORÇO:** 1 semana | **PRIORIDADE:** P1 (Alta)

---

### 🚨 PROBLEMA 5: Metadata Panel Genérico
**SEVERIDADE:** 🟠 **MÉDIA**

**O PROBLEMA:**
```cpp
class MetadataPanel : public QWidget {
    // Existe mas não sabemos o que mostra
};
```

**NO LIGHTROOM:**
Metadata panel tem **seções colapsáveis:**
- **File Info:** Filename, size, dimensions, format
- **EXIF:** Camera, lens, ISO, aperture, shutter, focal length
- **Location:** GPS, city, country, altitude
- **Keywords:** Tags hierárquicos
- **IPTC:** Copyright, caption, creator
- **History:** Edit history

**FALTANDO:**
- Hierarquia visual de informações
- Seções colapsáveis (accordions)
- Edição inline de metadata
- Batch editing de keywords

**IMPACTO:** Fotógrafos profissionais **não conseguem** organizar bibliotecas grandes

**SOLUÇÃO RECOMENDADA:**
```cpp
class MetadataPanel : public QWidget {
private:
    QScrollArea* m_scrollArea;
    
    // Collapsible sections
    CollapsibleSection* m_fileSection;
    CollapsibleSection* m_exifSection;
    CollapsibleSection* m_locationSection;
    CollapsibleSection* m_keywordSection;
    CollapsibleSection* m_iptcSection;
    
    // Editable fields
    QLineEdit* m_titleEdit;
    QTextEdit* m_captionEdit;
    TagEditor* m_keywordEditor;  // Autocomplete, hierarchical
};
```

**ESFORÇO:** 2-3 semanas | **PRIORIDADE:** P1 (Alta)

---

### 🚨 PROBLEMA 6: Thumbnail Rendering Performance Não Otimizada
**SEVERIDADE:** 🟠 **MÉDIA**

**O PROBLEMA:**
```cpp
QCache<QString, QPixmap> m_thumbnailCache;
QPixmap generateThumbnail(const QString& filepath);
```

**PERFORMANCE NO LIGHTROOM:**
- Thumbnails carregam em **3 níveis de qualidade:**
  1. Placeholder cinza (imediato)
  2. Low-res thumbnail (50ms)
  3. High-res thumbnail (200ms)
- Cache em disco (não apenas RAM)
- Pré-carregamento inteligente (próximas 50 imagens)

**NO PHOTOGURU:**
- Cache apenas em RAM (QCache)
- Sem níveis de qualidade progressiva
- Sem pré-carregamento

**IMPACTO:** Scrolling em bibliotecas grandes (1000+ fotos) é **lento**

**SOLUÇÃO RECOMENDADA:**
```cpp
class ThumbnailCache {
public:
    enum class Quality { Placeholder, Low, High };
    
    QPixmap getThumbnail(const QString& path, Quality quality);
    void preload(const QStringList& paths);  // Background thread
    void setCachePath(const QString& path);  // Disk cache
    
private:
    QCache<QString, QPixmap> m_memoryCache;
    QDir m_diskCacheDir;
    QThreadPool* m_loaderPool;
};
```

**ESFORÇO:** 2 semanas | **PRIORIDADE:** P2 (Média)

---

### 🚨 PROBLEMA 7: Ausência de Compare Mode
**SEVERIDADE:** 🟠 **MÉDIA**

**O PROBLEMA:**
Fotógrafos profissionais precisam **comparar imagens lado a lado** para escolher a melhor de um burst ou avaliar pequenas diferenças.

**NO LIGHTROOM:**
- **Survey Mode:** Mostra 2-10 imagens simultaneamente
- **Compare Mode:** Split-screen com zoom sincronizado
- **N** key = Survey, **C** key = Compare

**NO PHOTOGURU:**
- Não existe
- Usuário tem que abrir duas janelas externamente

**IMPACTO:** Workflow profissional é **incompleto**

**SOLUÇÃO RECOMENDADA:**
```cpp
class CompareView : public QWidget {
private:
    ImageViewer* m_leftViewer;
    ImageViewer* m_rightViewer;
    QSplitter* m_splitter;
    
    bool m_syncZoom = true;
    bool m_syncPan = true;
    
public:
    void setImages(const QString& left, const QString& right);
    void setSyncZoom(bool sync);
    void setSyncPan(bool sync);
};

// Keyboard shortcut
void MainWindow::onCompareMode() {
    if (m_thumbnailGrid->selectedFiles().count() == 2) {
        m_centralTabs->setCurrentWidget(m_compareView);
        // Load selected images
    }
}
```

**ESFORÇO:** 3 semanas | **PRIORIDADE:** P2 (Média)

---

### 🚨 PROBLEMA 8: Sorting & Filtering Primitivo
**SEVERIDADE:** 🟡 **ALTA**

**O PROBLEMA:**
```cpp
enum class SortOrder {
    ByName,
    ByDate,
    BySize
};
```

**NO LIGHTROOM:**
Sorting tem **12+ opções:**
- File Name, Capture Time, Edit Time, Rating, Color Label
- Camera Model, Focal Length, ISO Speed
- Custom order (drag & drop)

**Filtering tem camadas:**
- Star rating (1-5 stars)
- Color labels (Red, Yellow, Green, Blue, Purple)
- Flags (Picked, Rejected, Unflagged)
- Metadata filters (camera, lens, ISO range)
- Text search (keywords, filenames)

**NO PHOTOGURU:**
```cpp
class FilterPanel : public QWidget {
    // Existe mas não sabemos capacidade
};
```

**IMPACTO:** Fotógrafos não conseguem **curate** (selecionar melhores fotos)

**SOLUÇÃO RECOMENDADA:**
```cpp
struct FilterCriteria {
    // Rating
    int minStars = 0;
    int maxStars = 5;
    
    // Labels
    QSet<ColorLabel> colorLabels;
    
    // Flags
    bool showPicked = true;
    bool showUnflagged = true;
    bool showRejected = false;
    
    // Metadata ranges
    int minISO = 0;
    int maxISO = 999999;
    QString cameraModel;
    QString lensModel;
    
    // Date range
    QDate startDate;
    QDate endDate;
    
    // Text search
    QString searchQuery;
};

class FilterPanel : public QWidget {
    // Star rating slider (0-5)
    // Color label checkboxes
    // Flag buttons (P, U, X)
    // Metadata dropdowns (cameras, lenses)
    // Date range picker
    // Search box with autocomplete
};
```

**ESFORÇO:** 4 semanas | **PRIORIDADE:** P1 (Alta)

---

## 🎨 PROBLEMAS DE DESIGN VISUAL

### 1. **Iconografia Inconsistente**
```
⚠️ Cannot open file ':/icons/close.svg'
⚠️ Cannot open file ':/icons/float.svg'
```

**PROBLEMA:** Ícones faltando quebram **consistência visual**

**SOLUÇÃO:**
- Usar conjunto consistente (Material Icons ou SF Symbols para macOS)
- Todas operações principais precisam ícone reconhecível

**PRIORIDADE:** P2

### 2. **Espaçamento e Padding**
```cpp
QToolBar {
    spacing: 3px;
    padding: 3px;
}
```

**PROBLEMA:** Valores hardcoded não escalam com DPI/Retina

**SOLUÇÃO:**
```cpp
int spacing = qRound(3 * devicePixelRatio());
```

**PRIORIDADE:** P3

### 3. **Absence de Loading States Graciosas**
Quando não há imagens carregadas, usuário vê **tela vazia**

**NO LIGHTROOM:**
- Empty state mostra "Import photos or drag folder here"
- Grande ícone ilustrativo
- Botões de ação primários

**SOLUÇÃO:**
```cpp
void MainWindow::showEmptyState() {
    QWidget* emptyState = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(emptyState);
    
    QLabel* icon = new QLabel();
    icon->setPixmap(QIcon(":/icons/folder-open.svg").pixmap(128, 128));
    
    QLabel* title = new QLabel("No Photos Yet");
    title->setStyleSheet("font-size: 24px; color: #aaa;");
    
    QPushButton* importBtn = new QPushButton("Import Photos");
    
    layout->addWidget(icon, 0, Qt::AlignCenter);
    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addWidget(importBtn, 0, Qt::AlignCenter);
    
    m_imageViewer->setVisible(false);
    m_emptyState->setVisible(true);
}
```

**PRIORIDADE:** P2

---

## 🔑 PROBLEMAS DE DESCOBERTA (DISCOVERABILITY)

### 1. **Atalhos de Teclado Não Documentados**
**PROBLEMA:** Usuário não sabe que pode usar Arrow keys, Space, +/-

**SOLUÇÃO:**
- Help menu → Keyboard Shortcuts
- Tooltip em cada botão mostrando shortcut
- Cheatsheet popup (Cmd+?)

### 2. **Features Escondidas**
**PROBLEMA:** AI analysis existe mas usuário não descobre

**SOLUÇÃO:**
- Onboarding wizard na primeira execução
- "What's new" dialog em updates
- Contextual tooltips

---

## 📊 COMPARAÇÃO DIRETA: LIGHTROOM vs PHOTOGURU

| Feature | Lightroom | PhotoGuru | Gap |
|---------|-----------|-----------|-----|
| **Module System** | ⭐⭐⭐⭐⭐ Library/Develop/Map | ⭐⭐ Tabs não claramente definidas | 🔴 CRÍTICO |
| **Image Navigation** | ⭐⭐⭐⭐⭐ Filmstrip + status | ⭐⭐⭐ Arrow keys funcionam | 🟡 ALTA |
| **Metadata Panel** | ⭐⭐⭐⭐⭐ Editável, hierárquico | ⭐⭐ Básico, read-only? | 🟡 ALTA |
| **Filtering** | ⭐⭐⭐⭐⭐ 12+ dimensões | ⭐⭐ 3 sort options | 🟡 ALTA |
| **Rating System** | ⭐⭐⭐⭐⭐ Stars + colors + flags | ❌ Não existe | 🔴 CRÍTICO |
| **Compare Mode** | ⭐⭐⭐⭐⭐ Survey/Compare | ❌ Não existe | 🟠 MÉDIA |
| **Performance** | ⭐⭐⭐⭐ Thumbnails rápidas | ⭐⭐⭐ Não testado em escala | 🟠 MÉDIA |
| **Keyboard Workflow** | ⭐⭐⭐⭐⭐ Tudo tem shortcut | ⭐⭐⭐⭐ Bons atalhos | 🟢 BOA |
| **Dark Theme** | ⭐⭐⭐⭐⭐ Polido | ⭐⭐⭐⭐ Cores corretas | 🟢 BOA |
| **File Operations** | ⭐⭐⭐⭐ Copy/Move/Rename | ⭐⭐⭐ Implementado | 🟢 BOA |

---

## 🎯 RECOMENDAÇÕES PRIORIZADAS

### 🔴 P0 - BLOQUEADORES (4-6 semanas)
1. **Implementar Module System** (Library vs Develop)
2. **Adicionar Status Bar rico** (position, filename, selection count)
3. **Feedback visual completo** (confirmações, progress bars)
4. **Thumbnail highlight sincronizado** com imagem atual

### 🟡 P1 - ALTA PRIORIDADE (6-8 semanas)
5. **Sistema de Rating** (stars, colors, flags)
6. **Filtering avançado** (metadata, date ranges, text search)
7. **Metadata Panel editável** (keywords, IPTC, captions)
8. **Multi-select feedback** (contador, preview)

### 🟠 P2 - MÉDIA PRIORIDADE (8-10 semanas)
9. **Compare Mode** (side-by-side, survey)
10. **Thumbnail performance** (3-tier loading, disk cache)
11. **Empty states** e onboarding
12. **Iconografia consistente**

### 🟢 P3 - NICE TO HAVE
13. **Presets/Styles** para edição rápida
14. **Collections** e Smart Collections
15. **Publish Services** (export presets)

---

## 💡 QUICK WINS (1-2 semanas)

### Quick Win #1: Status Bar Rico
```cpp
QString status = QString("%1 | %2 of %3 | %4x%5px | %6")
    .arg(filename)
    .arg(current)
    .arg(total)
    .arg(width)
    .arg(height)
    .arg(filesize);
```

### Quick Win #2: Thumbnail Border Highlight
```css
QListWidget::item:selected {
    border: 3px solid #1f91ff;
    background-color: rgba(31, 145, 255, 0.2);
}
```

### Quick Win #3: Selection Counter
```cpp
if (count > 1) {
    m_statusBar->showMessage(
        QString("%1 photos selected").arg(count));
}
```

---

## 🎓 LIÇÕES DO LIGHTROOM

### 1. **Progressive Disclosure**
Não mostre tudo de uma vez. Lightroom esconde painéis avançados até usuário precisar.

### 2. **Keyboard-First Design**
Todo clique deve ter equivalente de teclado. Fotógrafos odeiam tirar mão do mouse.

### 3. **Non-Destructive Everything**
Lightroom nunca altera arquivo original. PhotoGuru precisa deixar isso **cristalino**.

### 4. **Performance is UX**
Thumbnails lentas = experiência ruim. Lightroom carrega 1000 thumbnails em < 2s.

### 5. **Consistent Mental Model**
Usuário sempre sabe "onde está" (Library, Develop, Export). PhotoGuru precisa dessa clareza.

---

## 📈 MÉTRICAS DE SUCESSO

Para considerar PhotoGuru **competitivo** com Lightroom em UX:

- [ ] **Onboarding:** Novo usuário consegue importar e organizar 100 fotos em < 5 minutos
- [ ] **Rating:** Usuário consegue avaliar (stars/flags) 200 fotos em < 10 minutos
- [ ] **Filtering:** Usuário consegue criar filtro complexo (camera + ISO + date) em < 30 segundos
- [ ] **Navigation:** Scroll em 1000 thumbnails é suave (60fps)
- [ ] **Keyboard:** 80% das tarefas podem ser feitas sem mouse
- [ ] **Discovery:** 90% dos usuários descobrem Compare Mode em primeira sessão

---

## 🎬 CONCLUSÃO

**PhotoGuru tem fundação técnica sólida**, mas precisa de **6-12 meses de refinamento de UX** para competir com Lightroom. Os problemas não são de código, são de **product thinking**.

### O Que Fazer Agora:
1. **Testar com usuários reais** - 5 fotógrafos profissionais, sessões de 1 hora
2. **Priorizar P0** - Module system e feedback visual são bloqueadores
3. **Iterar rapidamente** - Releases quinzenais com melhorias incrementais
4. **Medir tudo** - Analytics de quais features são usadas

### Potencial:
Se executado bem, PhotoGuru pode ser **alternativa viável** para fotógrafos que querem:
- Performance nativa (vs Lightroom Electron)
- AI integrada (vs Lightroom plugins)
- Preço acessível (vs Lightroom subscription)

**Mas o gap de UX precisa ser fechado primeiro.**

---

**Assinatura:**  
Ex-Chefe de Produto, Adobe Lightroom  
10 anos projetando workflows para fotógrafos profissionais
