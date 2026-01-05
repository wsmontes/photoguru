# Google Takeout Import Feature

## Visão Geral

O PhotoGuru agora detecta automaticamente pastas exportadas do Google Takeout e permite importar metadados enriquecidos (descrições, pessoas, álbuns, localização, datas) de volta para os arquivos de imagem.

## Motivação

**Problema**: O Google Fotos mantém metadados enriquecidos (reconhecimento facial, álbuns, descrições) na nuvem, sem gravá-los nos arquivos EXIF/IPTC/XMP das imagens. Ao exportar via Google Takeout, os metadados vêm em arquivos JSON separados (.jpg.json), tornando as fotos "portáteis" mas sem seus metadados.

**Solução**: Detectar automaticamente pastas do Google Takeout, parsear os arquivos JSON e aplicar os metadados aos arquivos de imagem usando campos EXIF/IPTC/XMP padrões, garantindo compatibilidade com Lightroom, Bridge e outros softwares.

## Estrutura do Google Takeout

### Arquivos
```
Google Fotos/
├── IMG_0001.jpg              # Imagem
├── IMG_0001.jpg.json        # Metadados JSON (sidecar)
├── IMG_0002.heic
├── IMG_0002.heic.json
└── ...
```

### Formato JSON
```json
{
  "title": "IMG_0001.jpg",
  "description": "Sunset at the beach",
  "albumNames": ["Vacation 2025", "Summer Photos"],
  "people": [
    {"name": "Alice"},
    {"name": "Bob"}
  ],
  "geoData": {
    "latitude": 37.7749,
    "longitude": -122.4194,
    "altitude": 10.5
  },
  "geoDataExif": {
    "latitude": 37.7749,
    "longitude": -122.4194
  },
  "location": "San Francisco, CA, USA",
  "photoTakenTime": {
    "timestamp": "1655305845",
    "formatted": "Jun 15, 2022, 6:30:45 PM UTC"
  },
  "creationTime": {...},
  "modificationTime": {...},
  "googlePhotosOrigin": {
    "mobileUpload": {
      "deviceType": "IOS_PHONE"
    }
  }
}
```

## Implementação

### Arquivos Criados

#### 1. GoogleTakeoutParser (`src/core/GoogleTakeoutParser.h/.cpp`)

**Responsabilidades**:
- Detectar pastas do Google Takeout
- Parsear arquivos JSON
- Extrair metadados estruturados

**Funções principais**:
```cpp
// Detecta se diretório é Google Takeout (amostra JSONs, valida estrutura)
bool isGoogleTakeoutDirectory(const QString& directoryPath);

// Parsea arquivo JSON e retorna metadados
TakeoutMetadata parseJsonFile(const QString& jsonPath);

// Encontra JSON para uma imagem (IMG.jpg → IMG.jpg.json)
QString findJsonForImage(const QString& imagePath);
```

**Estrutura de dados**:
```cpp
struct TakeoutMetadata {
    QString description;
    QStringList albumNames;
    QStringList people;
    std::optional<QGeoCoordinate> geoData;        // GPS final (editado)
    std::optional<QGeoCoordinate> geoDataExif;    // GPS original
    QString locationName;
    QDateTime photoTakenTime;
    QDateTime photoTakenTimeOriginal;
    QDateTime creationTime;
    QDateTime modificationTime;
    QString googlePhotosOrigin;
    QString deviceType;
    bool isValid;
    
    bool hasMetadataToApply() const;
};
```

**Detecção inteligente**:
- Amostra até 5 arquivos JSON
- Valida presença de campos signature: `photoTakenTime`, `creationTime`, `geoData`, `googlePhotosOrigin`
- Retorna `true` se 50%+ dos JSONs amostrados são válidos

#### 2. GoogleTakeoutImporter (`src/core/GoogleTakeoutImporter.h/.cpp`)

**Responsabilidades**:
- Coordenar importação em lote
- Aplicar metadados via MetadataWriter
- Rastrear estatísticas e erros

**Funções principais**:
```cpp
// Importa diretório completo
ImportResult importDirectory(const QString& directoryPath, 
                             const ImportOptions& options);

// Importa imagem individual
bool importSingleImage(const QString& imagePath, 
                      const ImportOptions& options);

// Aplica metadados parseados à imagem
bool applyMetadataToImage(const QString& imagePath,
                         const TakeoutMetadata& metadata,
                         const ImportOptions& options);
```

**Estruturas de controle**:
```cpp
struct ImportOptions {
    bool applyDescription = true;          // Aplicar caption
    bool applyPeopleAsKeywords = true;     // Pessoas → Keywords
    bool applyAlbumsAsKeywords = true;     // Álbuns → Keywords
    bool applyLocation = true;             // GPS + nome da localização
    bool applyDateTime = true;             // Data/hora captura
    bool overwriteExisting = true;         // Sobrescrever metadados existentes
    bool createBackup = false;             // Criar backup antes
};

struct ImportResult {
    int totalImages = 0;
    int withJson = 0;
    int metadataApplied = 0;
    int errors = 0;
    QStringList errorMessages;
    
    QString summary() const;
};
```

### Mapeamento de Metadados

O importer mapeia campos do Google Takeout para tags EXIF/IPTC/XMP padrões:

| Google Takeout | EXIF/IPTC/XMP |
|---------------|---------------|
| `description` | `EXIF:ImageDescription`<br>`IPTC:Caption-Abstract`<br>`XMP:Description` |
| `people[].name` | `IPTC:Keywords`<br>`XMP:PersonInImage`<br>`XMP:Subject` |
| `albumNames[]` | `IPTC:Keywords` (prefixo "Album:")<br>`XMP:Subject` |
| `geoData` | `EXIF:GPSLatitude`<br>`EXIF:GPSLongitude`<br>`EXIF:GPSAltitude`<br>+ referências (N/S, E/W) |
| `location` | `IPTC:City`<br>`IPTC:Province-State`<br>`IPTC:Country-PrimaryLocationName` |
| `photoTakenTime` | `EXIF:DateTimeOriginal`<br>`EXIF:CreateDate` |

**Estratégia de compatibilidade**:
- Escreve em **múltiplos campos** (EXIF + IPTC + XMP) para máxima compatibilidade
- Segue best practices do Adobe Lightroom e outros softwares profissionais
- Preserva metadados existentes quando não sobrescritos

### Integração com MainWindow

#### Detecção Automática

Ao carregar um diretório (`loadDirectory()`), o MainWindow:
1. Chama `GoogleTakeoutParser::isGoogleTakeoutDirectory()`
2. Se detectado, mostra notificação: *"Google Takeout folder detected! Would you like to import metadata?"*
3. Usuário pode importar manualmente via menu

```cpp
void MainWindow::loadDirectory(const QString& path) {
    m_currentDirectory = path;
    
    // Check if this is a Google Takeout directory
    checkAndOfferGoogleTakeoutImport(path);
    
    // Continue with normal loading...
}

void MainWindow::checkAndOfferGoogleTakeoutImport(const QString& directoryPath) {
    if (!GoogleTakeoutParser::isGoogleTakeoutDirectory(directoryPath)) {
        return;
    }
    
    LOG_INFO("MainWindow", "Google Takeout directory detected: " + directoryPath);
    
    NotificationManager::instance().showInfo(
        "Google Takeout folder detected! Would you like to import metadata?",
        10000  // 10 segundos
    );
}
```

#### Importação Manual

Menu: **Metadata → Import Google Takeout...** (atalho: `Ctrl+Shift+G`)

```cpp
void MainWindow::onImportGoogleTakeout() {
    // Configura opções de importação
    GoogleTakeoutImporter::ImportOptions options;
    options.applyDescription = true;
    options.applyPeopleAsKeywords = true;
    options.applyAlbumsAsKeywords = true;
    options.applyLocation = true;
    options.applyDateTime = true;
    options.overwriteExisting = true;
    options.createBackup = false;
    
    // Executa importação
    GoogleTakeoutImporter importer;
    auto result = importer.importDirectory(m_currentDirectory, options);
    
    // Mostra resultado
    QString message = QString(
        "Google Takeout import complete!\n\n"
        "Images processed: %1\n"
        "With JSON metadata: %2\n"
        "Metadata applied: %3\n"
        "Errors: %4"
    ).arg(result.totalImages)
     .arg(result.withJson)
     .arg(result.metadataApplied)
     .arg(result.errors);
    
    if (result.metadataApplied > 0) {
        NotificationManager::instance().showSuccess(message);
        // Refresh current image to show updated metadata
        m_metadataPanel->loadMetadata(m_imageFiles[m_currentIndex]);
    }
}
```

## Uso

### Via Interface Gráfica

1. **Detecção automática**:
   - Abra um diretório exportado do Google Takeout (`File → Open Directory`)
   - O PhotoGuru detecta automaticamente e mostra notificação
   - Use `Metadata → Import Google Takeout...` para importar

2. **Importação manual**:
   - Abra qualquer diretório
   - Use `Metadata → Import Google Takeout...` (Ctrl+Shift+G)
   - Se o diretório for válido, metadados serão aplicados

### Via Código

```cpp
#include "core/GoogleTakeoutParser.h"
#include "core/GoogleTakeoutImporter.h"

// Verificar se é Takeout
if (GoogleTakeoutParser::isGoogleTakeoutDirectory("/path/to/folder")) {
    // Configurar opções
    GoogleTakeoutImporter::ImportOptions options;
    options.applyDescription = true;
    options.applyPeopleAsKeywords = true;
    // ...
    
    // Importar
    GoogleTakeoutImporter importer;
    auto result = importer.importDirectory("/path/to/folder", options);
    
    qDebug() << result.summary();
    // Output: "Google Takeout Import: 150 images processed, 148 with JSON, 145 metadata applied, 3 errors"
}

// Parsear JSON individual
auto metadata = GoogleTakeoutParser::parseJsonFile("/path/IMG_001.jpg.json");
if (metadata.isValid && metadata.hasMetadataToApply()) {
    qDebug() << "Description:" << metadata.description;
    qDebug() << "People:" << metadata.people;
    qDebug() << "Albums:" << metadata.albumNames;
    qDebug() << "GPS:" << metadata.geoData->latitude() << metadata.geoData->longitude();
}
```

## Logging

Toda a operação é registrada via `Logger`:

```
[INFO] MainWindow: Google Takeout directory detected: /Users/user/Photos
[INFO] GoogleTakeoutImporter: === Starting Google Takeout import ===
[INFO] GoogleTakeoutImporter: Directory: /Users/user/Photos
[INFO] GoogleTakeoutImporter: Found 150 images
[DEBUG] GoogleTakeoutImporter: Applying metadata to: IMG_001.jpg
[DEBUG] GoogleTakeoutImporter:   Description: 42 chars
[DEBUG] GoogleTakeoutImporter:   People: Alice, Bob
[DEBUG] GoogleTakeoutImporter:   Albums: Vacation 2025
[DEBUG] GoogleTakeoutImporter:   GPS: 37.774900, -122.419400
[INFO] GoogleTakeoutImporter: ✅ Applied metadata to: IMG_001.jpg
...
[INFO] GoogleTakeoutImporter: === Import complete ===
[INFO] GoogleTakeoutImporter: Google Takeout Import: 150 images processed, 148 with JSON, 145 metadata applied, 3 errors
```

## Compatibilidade

### Formatos de Imagem Suportados
- JPEG (.jpg, .jpeg)
- PNG (.png)
- HEIC/HEIF (.heic, .heif)
- TIFF (.tif, .tiff)
- WebP (.webp)

### Compatibilidade com Software
Metadados gravados são compatíveis com:
- **Adobe Lightroom** (lê EXIF + IPTC + XMP)
- **Adobe Bridge** (lê todos os campos)
- **Apple Photos** (lê EXIF + algumas tags XMP)
- **Google Photos** (lê EXIF:DateTimeOriginal, GPS)
- **ExifTool** (lê e valida tudo)

## Limitações Conhecidas

1. **Face Labels sem coordenadas**: O Google Takeout exporta apenas nomes de pessoas, sem coordenadas faciais. Armazenamos como Keywords/PersonInImage.

2. **Dois tipos de GPS**:
   - `geoData`: Localização final (possivelmente editada pelo usuário ou estimada pelo Google)
   - `geoDataExif`: GPS original do EXIF
   - O importer usa `geoData` (preferência do usuário/Google)

3. **Timestamps múltiplos**:
   - `photoTakenTime`: Data da captura (possivelmente editada)
   - `photoTakenTimeOriginal`: Data original (se editada)
   - `creationTime`: Data de upload
   - O importer usa `photoTakenTime` (preferência do Google Photos)

4. **Sobrescrita**: Por padrão, sobrescreve metadados existentes. Use `options.overwriteExisting = false` para preservar.

## Testes

Os parsers e importers são testados via unidade e integração:

```bash
cd build
./PhotoGuruTests --gtest_filter="*GoogleTakeout*"
```

## Roadmap Futuro

### Curto Prazo
- [ ] UI para configurar ImportOptions (checkboxes no dialog)
- [ ] Preview de metadados antes de aplicar
- [ ] Botão de "Import" direto na notificação
- [ ] Progress bar com cancelamento

### Médio Prazo
- [ ] Suporte a vídeos (.mp4, .mov) com sidecars JSON
- [ ] Detecção de conflitos (metadados já existentes diferentes)
- [ ] Log detalhado por arquivo (CSV export)
- [ ] Undo/rollback de importação

### Longo Prazo
- [ ] Importação inteligente de face regions (usar ML para re-detectar faces e vincular aos nomes)
- [ ] Sincronização bidirecional (exportar de volta para Takeout format)
- [ ] Suporte a outros formatos de exportação (Apple Photos, Adobe Lightroom)

## Referências

- [Documentação Google Takeout](../Google_Fotos.txt)
- [ExifTool Documentation](https://exiftool.org/)
- [IPTC Photo Metadata Standard](https://www.iptc.org/standards/photo-metadata/)
- [XMP Specification](https://www.adobe.com/devnet/xmp.html)

## Contribuindo

Para melhorias ou correções no Google Takeout import:

1. Teste com exports reais do Google Fotos
2. Valide metadados com ExifTool: `exiftool -a -G1 IMG_001.jpg`
3. Verifique compatibilidade com Lightroom/Bridge
4. Adicione testes para novos casos
5. Documente limitações conhecidas

---

**Status**: ✅ Implementado e testado (Build 100% sucesso)  
**Autor**: PhotoGuru Team  
**Data**: 2025-01-20
