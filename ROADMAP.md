# PhotoGuru MVP - Roadmap de Implementação

## 🎯 OBJETIVO
Transformar o PhotoGuru em um produto **usável e valioso** em 4-5 semanas.

---

## 📅 SPRINT 1 (Semana 1) - FUNDAÇÃO

### Dia 1-2: Limpeza e Decisões
- [ ] Review da análise MVP com stakeholders
- [ ] Criar branch `mvp-phase1`
- [ ] Backup de features complexas para branch `future-features`
- [ ] Desabilitar temporariamente:
  - MapView
  - TimelineView
  - SKPBrowser
  - Análise complexa de IA

**Entregável:** Código limpo, aplicação ainda compila

### Dia 3-5: Operações Básicas de Arquivo

**Implementar em src/ui/MainWindow.cpp:**

```cpp
// Novos slots
void MainWindow::onCopyFile() {
    // Copiar arquivo selecionado
}

void MainWindow::onMoveFile() {
    // Mover arquivo para pasta
}

void MainWindow::onRenameFile() {
    // Dialog para renomear
}

void MainWindow::onDeleteFile() {
    // Mover para lixeira (não deletar permanente)
}

void MainWindow::onRevealInFinder() {
    // Abrir localização no Finder
}
```

**Atalhos de teclado:**
- Cmd+C: Copiar
- Cmd+V: Colar
- Cmd+Delete: Mover para lixeira
- Cmd+R: Renomear
- Cmd+Shift+R: Revelar no Finder
- Delete: Deletar
- Space: Quick Look style preview

**Testes:**
```cpp
// tests/test_file_operations.cpp
TEST(FileOperations, CopyFile)
TEST(FileOperations, MoveFile)
TEST(FileOperations, RenameFile)
TEST(FileOperations, DeleteToTrash)
```

**Entregável:** Usuário consegue gerenciar arquivos básicos

---

## 📅 SPRINT 2 (Semana 2) - NAVEGAÇÃO E FILTROS

### Dia 1-3: Melhorar ThumbnailGrid

**src/ui/ThumbnailGrid.cpp:**

```cpp
// Adicionar
void ThumbnailGrid::setSortMode(SortMode mode) {
    // SORT_BY_NAME
    // SORT_BY_DATE
    // SORT_BY_SIZE
    // SORT_BY_RATING
}

void ThumbnailGrid::setThumbnailSize(ThumbnailSize size) {
    // SMALL (128px)
    // MEDIUM (256px)
    // LARGE (512px)
}

void ThumbnailGrid::setSelectionMode(bool multiSelect) {
    // Permitir Cmd+Click para seleção múltipla
}

QList<QString> ThumbnailGrid::selectedFiles() const {
    // Retornar lista de arquivos selecionados
}
```

**UI Improvements:**
- Dropdown para ordenação no toolbar
- Slider para tamanho dos thumbnails
- Status bar mostra: "145 fotos | 12 selecionadas | 3.2 GB"

**Entregável:** Grid profissional e responsivo

### Dia 4-5: Painel de Filtros Simples

**src/ui/FilterPanel.cpp (simplificado):**

```cpp
class FilterPanel : public QWidget {
    // Filtros simples
    QComboBox* m_fileTypeFilter;    // Todos, JPEG, RAW, HEIF
    QDateEdit* m_dateFrom;          // Data inicial
    QDateEdit* m_dateTo;            // Data final
    QComboBox* m_cameraFilter;      // Lista de câmeras detectadas
    QSlider* m_ratingFilter;        // 0-5 estrelas
    QLineEdit* m_searchBox;         // Busca por nome
};
```

**Filtros aplicados em tempo real** (debounce de 300ms)

**Entregável:** Usuário consegue filtrar 1000 fotos instantaneamente

---

## 📅 SPRINT 3 (Semana 3) - METADADOS INTELIGENTES

### Dia 1-3: Melhorar MetadataPanel

**src/ui/MetadataPanel.cpp (redesign):**

```cpp
// Seções colapsáveis
┌─ 📷 Câmera & Lente ─────────────┐
│ Canon EOS R5                     │
│ RF 24-70mm f/2.8 L IS USM       │
│ 🔍 50mm · f/2.8 · 1/250s · ISO400│
└──────────────────────────────────┘

┌─ 📍 Localização ────────────────┐
│ Rio de Janeiro, Brasil           │
│ -22.9068, -43.1729              │
│ [Ver no Mapa]                   │
└──────────────────────────────────┘

┌─ 📅 Data & Hora ────────────────┐
│ 4 de Janeiro de 2026            │
│ 14:30:45 (BRT - UTC-3)          │
└──────────────────────────────────┘

┌─ 📊 Arquivo ────────────────────┐
│ IMG_5432.CR3                    │
│ RAW (Canon)                     │
│ 45.2 MB · 8192 × 5464 px       │
└──────────────────────────────────┘

┌─ 🏷️ Tags (0) ──────────────────┐
│ [Nenhuma tag]                   │
│ [+ Adicionar tags]              │
└──────────────────────────────────┘
```

**Funcionalidades:**
- Ícones visuais para cada tipo
- Cópia com clique (ex: copiar coordenadas GPS)
- Link para Google Maps
- Formatação amigável (não mostrar raw EXIF)
- Conversão de coordenadas GPS → nome da cidade (API gratuita)

**Entregável:** Metadados bonitos e úteis

### Dia 4-5: Coordenadas → Localização

**Novo: src/core/LocationService.cpp**

```cpp
class LocationService {
public:
    static QString coordinatesToLocation(double lat, double lon);
    // Usa OpenStreetMap Nominatim API (grátis)
    // Cache local em SQLite
};
```

**Cache:**
```sql
CREATE TABLE location_cache (
    lat REAL,
    lon REAL,
    city TEXT,
    state TEXT,
    country TEXT,
    timestamp INTEGER,
    PRIMARY KEY (lat, lon)
);
```

**Entregável:** GPS vira "Rio de Janeiro, Brasil" automaticamente

---

## 📅 SPRINT 4 (Semana 4) - IA SIMPLIFICADA

### Dia 1-2: Novo Backend Simples

**agent_mvp.py (substituir agent_v2.py):**

```python
#!/usr/bin/env python3
"""
PhotoGuru MVP - AI Analysis Service
Lightweight backend using OpenAI Vision API
"""

import os
import base64
import json
from pathlib import Path
from PIL import Image
import openai

def analyze_photo(filepath: str, api_key: str = None) -> dict:
    """
    Analisa foto usando GPT-4 Vision
    Returns: {
        "title": str,
        "description": str,
        "tags": [str],
        "confidence": float
    }
    """
    # Resize se necessário (max 2048px)
    img = Image.open(filepath)
    if max(img.size) > 2048:
        img.thumbnail((2048, 2048))
    
    # Encode base64
    buffered = BytesIO()
    img.save(buffered, format="JPEG")
    img_base64 = base64.b64encode(buffered.getvalue()).decode()
    
    # Call OpenAI
    client = openai.OpenAI(api_key=api_key or os.getenv("OPENAI_API_KEY"))
    
    response = client.chat.completions.create(
        model="gpt-4-vision-preview",
        messages=[{
            "role": "user",
            "content": [
                {
                    "type": "text",
                    "text": """Analyze this photo and provide:
                    1. A short descriptive title (max 60 chars)
                    2. A brief description (max 150 chars)
                    3. 5 relevant tags for searchability
                    
                    Return as JSON: {"title": "...", "description": "...", "tags": [...]}"""
                },
                {
                    "type": "image_url",
                    "image_url": {"url": f"data:image/jpeg;base64,{img_base64}"}
                }
            ]
        }],
        max_tokens=300
    )
    
    result = json.loads(response.choices[0].message.content)
    result["confidence"] = 0.95  # GPT-4V is reliable
    
    return result


def write_metadata_to_file(filepath: str, metadata: dict):
    """
    Grava metadados via exiftool
    """
    import subprocess
    
    # Construir comando exiftool
    cmd = ["exiftool", "-overwrite_original"]
    
    if "title" in metadata:
        cmd.extend(["-Title=" + metadata["title"]])
    
    if "description" in metadata:
        cmd.extend(["-Description=" + metadata["description"]])
    
    if "tags" in metadata:
        for tag in metadata["tags"]:
            cmd.extend(["-Keywords+=" + tag])
    
    cmd.append(filepath)
    
    subprocess.run(cmd, check=True)


def search_photos(directory: str, query: str) -> list:
    """
    Busca simples por tags/metadados
    """
    import subprocess
    
    # Usar exiftool para buscar
    result = subprocess.run(
        ["exiftool", "-r", "-if", f"$Keywords =~ /{query}/i", 
         "-filename", directory],
        capture_output=True,
        text=True
    )
    
    # Parse output
    files = []
    for line in result.stdout.split("\n"):
        if line.strip():
            files.append(line.split(":")[1].strip())
    
    return files


if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: agent_mvp.py <command> [args]")
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "analyze":
        filepath = sys.argv[2]
        result = analyze_photo(filepath)
        print(json.dumps(result, indent=2))
    
    elif command == "write":
        filepath = sys.argv[2]
        metadata = json.loads(sys.argv[3])
        write_metadata_to_file(filepath, metadata)
        print("Metadata written successfully")
    
    elif command == "search":
        directory = sys.argv[2]
        query = sys.argv[3]
        results = search_photos(directory, query)
        print(json.dumps(results, indent=2))
```

**Dependências:**
```txt
openai==1.10.0
pillow==10.2.0
```

**Entregável:** IA funcional via API cloud

### Dia 3-4: Integração C++

**src/ml/PythonBridge.cpp (simplificado):**

```cpp
bool PythonBridge::analyzePhoto(const QString& filepath) {
    QProcess process;
    process.start("python3", {
        m_agentPath,
        "analyze",
        filepath
    });
    
    if (!process.waitForFinished(10000)) {
        emit error("Analysis timeout");
        return false;
    }
    
    QString output = process.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (doc.isNull()) {
        emit error("Invalid JSON response");
        return false;
    }
    
    QJsonObject result = doc.object();
    
    // Gravar metadados
    writeMetadata(filepath, result);
    
    emit analysisComplete(filepath, result);
    return true;
}

void PythonBridge::writeMetadata(const QString& filepath, const QJsonObject& metadata) {
    QProcess process;
    
    QString metadataJson = QJsonDocument(metadata).toJson(QJsonDocument::Compact);
    
    process.start("python3", {
        m_agentPath,
        "write",
        filepath,
        metadataJson
    });
    
    process.waitForFinished();
}
```

**Entregável:** C++ chama Python que chama API

### Dia 5: UI para Análise

**Simplificar src/ui/AnalysisPanel.cpp:**

```cpp
┌─ 🤖 AI Analysis ─────────────────┐
│                                   │
│  [🖼️ IMG_5432.CR3]               │
│                                   │
│  Status: Ready                    │
│                                   │
│  [Analyze This Photo]             │
│  [Analyze All in Folder]          │
│                                   │
│  ⚙️ Settings:                     │
│  ☑ Auto-analyze new photos        │
│  ☐ Overwrite existing tags        │
│                                   │
│  API Key: [******************]    │
│  [Get Free Key]                   │
│                                   │
│  Usage: 12 / 50 (free tier)       │
└───────────────────────────────────┘
```

**Entregável:** Interface simples para IA

---

## 📅 SPRINT 5 (Semana 5) - POLISH & TESTE

### Dia 1-2: Coleções Inteligentes

**src/core/SmartCollection.h:**

```cpp
struct CollectionRule {
    QString field;      // "camera", "date", "tag", "rating"
    QString operation;  // "equals", "contains", "between"
    QVariant value;
};

class SmartCollection {
    QString m_name;
    QList<CollectionRule> m_rules;
    
    QList<QString> evaluate(const QList<PhotoMetadata>& photos);
};
```

**Exemplos pré-definidos:**
- "Últimas 30 dias"
- "5 estrelas"
- "RAW não processados"
- "Com localização GPS"
- "Sem tags"

**Entregável:** Filtros salvos como coleções

### Dia 3: Performance

**Otimizações:**
- [ ] Lazy loading de thumbnails
- [ ] Thread pool para decodificação RAW
- [ ] Cache em disco (~/Library/Caches/PhotoGuru/)
- [ ] Pré-carregamento (próximas 3 fotos)

**Benchmarks:**
```cpp
TEST(Performance, Load1000Thumbnails) {
    // Deve completar em < 5 segundos
}

TEST(Performance, NavigateBetweenPhotos) {
    // Deve ser < 100ms
}

TEST(Performance, ApplyFilter) {
    // Deve ser < 500ms para 10k fotos
}
```

**Entregável:** App rápido e responsivo

### Dia 4-5: Beta Testing

**Checklist:**
- [ ] Instalador para macOS (DMG)
- [ ] README atualizado
- [ ] Tutorial in-app (primeira execução)
- [ ] 5 usuários beta testando
- [ ] Google Form para feedback
- [ ] Bug tracking

**Entregável:** Feedback de usuários reais

---

## 🚀 PÓS-MVP (Semana 6+)

### Se Feedback for Positivo:

#### v1.1 - Edição Básica
- Crop
- Rotate
- Ajustes de exposição

#### v1.2 - Compartilhamento
- Exportar para pasta
- Redimensionar em lote
- Converter formatos
- Criar álbum web

#### v1.3 - Duplicatas e Bursts
- Detecção de duplicatas
- Agrupamento de bursts
- Sugestão de melhor foto

#### v2.0 - Features Avançadas
- **Agora sim:** Semantic Key Protocol
- Timeline interativa
- Visualização em mapa
- Face recognition
- LLM local (opcional)

---

## 📊 MÉTRICAS DE SUCESSO

### Técnicas
- ✅ Build time < 2 minutos
- ✅ Instalação < 5 minutos
- ✅ Tamanho do app < 100 MB (sem Python)
- ✅ RAM usage < 500 MB (idle)
- ✅ RAM usage < 2 GB (10k fotos carregadas)

### Funcionais
- ✅ Abrir 1000 fotos < 5 segundos
- ✅ Análise IA < 3 segundos/foto
- ✅ Busca < 1 segundo
- ✅ Zero crashes em 1 hora de uso

### Qualitativas
- ✅ NPS > 40 (usuários beta)
- ✅ 80% completam tutorial
- ✅ 50% usam análise IA
- ✅ 70% acham "melhor que Preview.app"

---

## 🛠️ STACK FINAL (MVP)

### C++/Qt6
```
src/
├── core/
│   ├── ImageLoader.*       (✅ já existe)
│   ├── MetadataReader.*    (✅ já existe)
│   ├── PhotoMetadata.h     (✅ já existe)
│   ├── FileOperations.*    (🆕 adicionar)
│   ├── LocationService.*   (🆕 adicionar)
│   └── SmartCollection.*   (🆕 adicionar)
├── ui/
│   ├── MainWindow.*        (♻️ simplificar)
│   ├── ImageViewer.*       (✅ já existe)
│   ├── ThumbnailGrid.*     (♻️ melhorar)
│   ├── MetadataPanel.*     (♻️ redesign)
│   ├── FilterPanel.*       (♻️ simplificar)
│   └── AnalysisPanel.*     (♻️ simplificar)
└── ml/
    └── PythonBridge.*      (♻️ simplificar)
```

### Python
```
agent_mvp.py                (🆕 substituir agent_v2.py)
requirements_mvp.txt        (🆕 3 dependências)
```

### Total LOC
- **Antes:** ~15,000 linhas
- **Depois:** ~4,000 linhas
- **Redução:** 73%

---

## ⚠️ RED FLAGS - Quando Parar

**STOP se:**
1. Após 2 sprints, app ainda não funciona básico
2. Bugs críticos não resolvidos em 3 dias
3. Feedback beta é < 30% positivo
4. API costs > $100/mês em beta

**PIVOT se:**
1. Usuários não usam IA (focar em viewer puro)
2. Usuários pedem edição (adicionar editor simples)
3. Usuários querem cloud (considerar sync)

---

## 📞 COMUNICAÇÃO

### Daily Updates
- Commit diário com progresso
- Update no README.md
- Demo sexta-feira

### Weekly Review
- O que funcionou
- O que não funcionou  
- Decisões necessárias
- Ajustes no roadmap

---

## 🎯 FOCO CONSTANTE

> **"Ship early, ship often"**

Não importa quão legal é o Semantic Key Protocol se o usuário não consegue deletar uma foto.

**Prioridade sempre:**
1. Funciona?
2. É útil?
3. É rápido?
4. É bonito?

Nessa ordem. ✅

---

**Última atualização:** 4 de Janeiro de 2026  
**Próxima revisão:** Final de cada sprint  
**Owner:** PhotoGuru Team
