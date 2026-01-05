# Testing Strategy - PhotoGuru

## 🎯 Filosofia de Testes com IA

Seguimos a **pirâmide de testes** padrão da indústria, adaptada para componentes de AI/ML:

```
        /\
       /E2E\      ← 2-3 testes (10%) - Fluxo completo, LENTOS
      /------\
     /  INT  \    ← 5-10 testes (20%) - AI real, MÉDIOS
    /----------\
   /   UNIT    \  ← 70+ testes (70%) - Mocks, RÁPIDOS
  /--------------\
```

## 📊 Status Atual

### ✅ Testes Unitários (Implementados)
- **74 testes** do AnalysisPanel
- **258 testes totais** no projeto
- **99ms** de execução (sem carregar modelos)
- **100% autônomos** - sem GUI, sem interação manual
- **Mocks**: CLIP e VLM retornam dados fake

**Por que funciona:**
- Valida toda a lógica de negócio
- Valida UI states, botões, checkboxes
- Valida fluxos de controle
- Execução instantânea no CI/CD

### ⏳ Testes de Integração (A implementar)
**Objetivo**: Validar que os modelos AI funcionam REALMENTE

#### Estrutura Proposta:
```
tests/integration/
├── fixtures/
│   ├── test_images/
│   │   ├── cat.jpg              # Imagem conhecida
│   │   ├── landscape.jpg        # Imagem conhecida
│   │   └── portrait.jpg         # Imagem conhecida
│   └── expected_outputs/
│       ├── cat_embedding.bin    # Baseline CLIP
│       ├── cat_caption.txt      # Baseline VLM
│       └── landscape_embedding.bin
├── test_clip_integration.cpp
├── test_vlm_integration.cpp
└── test_metadata_integration.cpp
```

#### Exemplo de Teste de Integração:
```cpp
// tests/integration/test_clip_integration.cpp
#include <gtest/gtest.h>
#include "../../src/ml/CLIPAnalyzer.h"

class CLIPIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Carrega modelo UMA VEZ para todos os testes
        // (compartilhado entre testes = rápido)
        if (!clip) {
            clip = std::make_unique<CLIPAnalyzer>();
            clip->initialize("models/clip-vit-base-patch32.onnx", true);
        }
    }
    
    static std::unique_ptr<CLIPAnalyzer> clip;
};

std::unique_ptr<CLIPAnalyzer> CLIPIntegrationTest::clip = nullptr;

TEST_F(CLIPIntegrationTest, ComputeEmbedding_ReturnsCorrectDimension) {
    QImage img("tests/integration/fixtures/test_images/cat.jpg");
    ASSERT_FALSE(img.isNull());
    
    auto embedding = clip->computeEmbedding(img);
    ASSERT_TRUE(embedding.has_value());
    
    // CLIP ViT-B/32 sempre retorna 512 dimensões
    EXPECT_EQ(embedding->size(), 512);
}

TEST_F(CLIPIntegrationTest, SimilarImages_HighSimilarity) {
    QImage cat1("tests/integration/fixtures/test_images/cat.jpg");
    QImage cat2("tests/integration/fixtures/test_images/cat_similar.jpg");
    
    auto emb1 = clip->computeEmbedding(cat1);
    auto emb2 = clip->computeEmbedding(cat2);
    
    ASSERT_TRUE(emb1.has_value() && emb2.has_value());
    
    float similarity = clip->computeSimilarity(*emb1, *emb2);
    
    // Imagens similares devem ter similaridade > 0.8
    EXPECT_GT(similarity, 0.8f) << "Similar cat images should be >80% similar";
}

TEST_F(CLIPIntegrationTest, DifferentImages_LowSimilarity) {
    QImage cat("tests/integration/fixtures/test_images/cat.jpg");
    QImage landscape("tests/integration/fixtures/test_images/landscape.jpg");
    
    auto emb1 = clip->computeEmbedding(cat);
    auto emb2 = clip->computeEmbedding(landscape);
    
    ASSERT_TRUE(emb1.has_value() && emb2.has_value());
    
    float similarity = clip->computeSimilarity(*emb1, *emb2);
    
    // Imagens diferentes devem ter similaridade < 0.5
    EXPECT_LT(similarity, 0.5f) << "Cat vs landscape should be <50% similar";
}

TEST_F(CLIPIntegrationTest, SameImage_PerfectSimilarity) {
    QImage img("tests/integration/fixtures/test_images/cat.jpg");
    
    auto emb1 = clip->computeEmbedding(img);
    auto emb2 = clip->computeEmbedding(img);
    
    ASSERT_TRUE(emb1.has_value() && emb2.has_value());
    
    float similarity = clip->computeSimilarity(*emb1, *emb2);
    
    // Mesma imagem = 100% similar (ou muito próximo devido a floating point)
    EXPECT_NEAR(similarity, 1.0f, 0.01f);
}

TEST_F(CLIPIntegrationTest, RegressionTest_KnownEmbedding) {
    // Testa que o modelo não mudou (regression test)
    QImage cat("tests/integration/fixtures/test_images/cat.jpg");
    
    auto embedding = clip->computeEmbedding(cat);
    ASSERT_TRUE(embedding.has_value());
    
    // Carrega baseline pré-computado
    std::vector<float> expected = loadBaselineEmbedding("fixtures/expected_outputs/cat_embedding.bin");
    
    // Verifica que embedding é similar ao baseline (tolerância pequena)
    float diff = computeEmbeddingDifference(*embedding, expected);
    EXPECT_LT(diff, 0.01f) << "Embedding should match baseline (model consistency)";
}
```

#### Exemplo de Teste VLM:
```cpp
// tests/integration/test_vlm_integration.cpp
TEST_F(VLMIntegrationTest, GenerateCaption_ContainsKeywords) {
    QImage cat("tests/integration/fixtures/test_images/cat.jpg");
    
    auto caption = vlm->generateCaption(cat);
    ASSERT_TRUE(caption.has_value());
    
    QString text = caption->toLower();
    
    // Caption de gato deve conter palavras relacionadas
    bool hasRelevantWord = text.contains("cat") || 
                           text.contains("feline") || 
                           text.contains("kitten") ||
                           text.contains("animal");
    
    EXPECT_TRUE(hasRelevantWord) 
        << "Caption should contain cat-related keywords. Got: " << text.toStdString();
}

TEST_F(VLMIntegrationTest, GenerateCaption_ReasonableLength) {
    QImage img("tests/integration/fixtures/test_images/landscape.jpg");
    
    auto caption = vlm->generateCaption(img);
    ASSERT_TRUE(caption.has_value());
    
    // Caption deve ter entre 10 e 200 caracteres (razoável)
    int len = caption->length();
    EXPECT_GE(len, 10) << "Caption too short";
    EXPECT_LE(len, 200) << "Caption too long";
}

TEST_F(VLMIntegrationTest, RegressionTest_ConsistentOutput) {
    // VLMs podem ter variação, mas devem ser consistentes em múltiplas execuções
    QImage img("tests/integration/fixtures/test_images/portrait.jpg");
    
    auto caption1 = vlm->generateCaption(img);
    auto caption2 = vlm->generateCaption(img);
    auto caption3 = vlm->generateCaption(img);
    
    ASSERT_TRUE(caption1.has_value() && caption2.has_value() && caption3.has_value());
    
    // Captions devem ser idênticos (ou muito similares)
    // Para modelos determinísticos: devem ser exatamente iguais
    EXPECT_EQ(*caption1, *caption2);
    EXPECT_EQ(*caption2, *caption3);
}
```

### 🎯 Testes E2E (A implementar)
**Objetivo**: Validar fluxo completo como usuário real

```cpp
// tests/e2e/test_full_workflow.cpp
TEST_F(E2ETest, FullWorkflow_AnalyzeAndSave) {
    // 1. Setup: cria diretório temporário com imagens
    QTemporaryDir tempDir;
    copyTestImages(tempDir.path());
    
    // 2. Cria MainWindow (com AI inicializada)
    MainWindow window;
    ASSERT_TRUE(window.waitForAIReady(30000)); // 30s timeout
    
    // 3. Abre diretório
    window.openDirectory(tempDir.path());
    ASSERT_EQ(window.imageCount(), 10);
    
    // 4. Seleciona primeira imagem
    window.selectImage(0);
    
    // 5. Clica em Analyze
    window.clickAnalyzeButton();
    ASSERT_TRUE(window.waitForAnalysisComplete(60000)); // 60s timeout
    
    // 6. Verifica caption gerada
    QString caption = window.getGeneratedCaption();
    EXPECT_FALSE(caption.isEmpty());
    
    // 7. Verifica metadata escrita
    QString imagePath = window.currentImagePath();
    PhotoMetadata meta = MetadataReader::read(imagePath);
    EXPECT_EQ(meta.llm_title, caption);
    EXPECT_FALSE(meta.llm_keywords.isEmpty());
    
    // 8. Testa batch
    window.clickBatchAnalyzeButton();
    ASSERT_TRUE(window.waitForBatchComplete(300000)); // 5min timeout
    
    // 9. Verifica que todas as 10 imagens foram processadas
    int processed = window.processedImageCount();
    EXPECT_EQ(processed, 10);
}
```

## 🚀 Estratégia de Execução

### No Desenvolvimento Local:
```bash
# Rápido: apenas unit tests (99ms)
./PhotoGuruTests --gtest_filter="*Test*"

# Médio: unit + integration (30s - 2min)
./PhotoGuruTests --gtest_filter="*Test*:*Integration*"

# Completo: tudo incluindo E2E (5-10min)
./PhotoGuruTests
```

### No CI/CD (GitHub Actions):
```yaml
# .github/workflows/tests.yml
jobs:
  unit-tests:
    runs-on: macos-latest
    steps:
      - run: ./PhotoGuruTests --gtest_filter="*Test*"
    # Executa SEMPRE (rápido)
  
  integration-tests:
    runs-on: macos-latest
    steps:
      - name: Cache models
        uses: actions/cache@v3
        with:
          path: models/
          key: ai-models-v1
      - run: ./PhotoGuruTests --gtest_filter="*Integration*"
    # Executa em PRs e merge (médio)
  
  e2e-tests:
    runs-on: macos-latest
    steps:
      - run: ./PhotoGuruTests --gtest_filter="*E2E*"
    # Executa apenas em main branch (lento)
```

## 📊 Métricas de Qualidade

### Coverage Targets:
- **Unit Tests**: 80%+ code coverage
- **Integration Tests**: 100% dos componentes AI validados
- **E2E Tests**: Top 3 user workflows cobertos

### Performance Targets:
| Tipo | Tempo Máximo | Frequência |
|------|--------------|------------|
| Unit | < 1s | Toda commit |
| Integration | < 2min | Pull Request |
| E2E | < 10min | Merge main |

## 🔧 Ferramentas Usadas

### Empresas Grandes (Google, Microsoft, Meta):
1. **Mocks/Fakes**: Para unit tests
2. **Golden Tests**: Comparar outputs com baselines conhecidas
3. **A/B Testing**: Comparar modelos em produção
4. **Shadow Testing**: Novo modelo roda paralelo ao antigo
5. **Canary Deployments**: Rollout gradual

### Nosso Approach (Pragmático):
1. ✅ **Mocks**: Unit tests rápidos
2. 🔄 **Integration**: Modelos reais, fixtures conhecidas
3. 🔄 **Regression**: Baselines de embeddings/captions
4. 🔄 **E2E**: Workflows reais, timeouts generosos

## 📝 Próximos Passos

1. **Criar fixtures** (5 imagens de teste conhecidas)
2. **Gerar baselines** (rodar uma vez, salvar outputs)
3. **Implementar integration tests** (5-10 testes)
4. **Implementar E2E tests** (2-3 workflows)
5. **Configurar CI/CD** com cache de modelos
6. **Documentar baselines** (como atualizar quando modelos mudarem)

## 🎯 Quando Atualizar Baselines

Baselines devem ser atualizados quando:
- ✅ Modelo é intencionalmente atualizado (ex: CLIP v2)
- ✅ Parâmetros são ajustados (ex: temperature)
- ❌ NUNCA atualizar só porque teste falhou!

**Processo**:
1. Revisar porque baseline mudou
2. Validar manualmente os novos outputs
3. Atualizar baseline com aprovação em PR
4. Documentar mudança no CHANGELOG

## 📚 Referências

- [Google Testing Blog](https://testing.googleblog.com/)
- [Martin Fowler - Testing Strategies](https://martinfowler.com/articles/practical-test-pyramid.html)
- [ML Testing Best Practices](https://developers.google.com/machine-learning/testing-debugging)
