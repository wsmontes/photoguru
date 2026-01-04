# PhotoGuru Viewer - Resumo Executivo para MVP

**Data:** 4 de Janeiro de 2026  
**Versão Atual:** 1.0.0 (Em desenvolvimento - 40% completo)  
**Proposta:** Pivô para MVP em 4-5 semanas

---

## 🎯 SITUAÇÃO ATUAL

### O Que Temos
✅ **Arquitetura técnica sólida**
- C++/Qt6 para UI performática
- Python para ML
- Suporte RAW/HEIF funcionando
- ~15.000 linhas de código

✅ **Features implementadas (parcialmente)**
- Visualizador de imagens básico
- Grid de thumbnails
- Leitura de metadados EXIF
- Sistema de análise IA (complexo)

### O Que Falta (Crítico)
❌ **Funcionalidades básicas de um visualizador**
- Copiar/colar/mover arquivos
- Renomear fotos
- Deletar para lixeira
- Filtros simples (data, tipo)
- Ordenação (nome, data, tamanho)

❌ **Usabilidade**
- Instalação requer 2GB de dependências Python
- Demora >30 minutos para configurar
- Muitos recursos não funcionam
- Interface sobrecarregada

### O Problema Principal
**Estamos tentando fazer TUDO que o Lightroom faz + IA avançada.**

Resultado: 70% do código é feature creep, 30% é essencial.

---

## 💡 PROPOSTA: MVP "PhotoGuru Lite"

### Filosofia
> **"O melhor visualizador de fotos para Mac com leitura inteligente de metadados"**

Fazer MUITO BEM o básico + um diferencial claro (IA simples).

### Diferencial Competitivo
Ao contrário do Lightroom que força uso de catálogo, o PhotoGuru:
- ✅ Lê metadados direto das fotos
- ✅ Não precisa de "importação"
- ✅ Funciona com qualquer pasta
- ✅ Compatível com outros apps
- ✅ IA opcional (não obrigatória)

---

## 📋 MVP SCOPE

### MUST HAVE (4 semanas)

#### Semana 1: Fundação
- Visualização sólida (zoom, pan, setas)
- Grid de thumbnails eficiente
- **Operações de arquivo**: copiar, mover, renomear, deletar
- Atalhos de teclado completos

#### Semana 2: Organização
- Filtros básicos (tipo, data, câmera)
- Ordenação (nome, data, tamanho)
- Seleção múltipla
- Coleções inteligentes

#### Semana 3: Metadados
- Painel redesenhado (bonito e útil)
- GPS → Nome da cidade (API grátis)
- Informações formatadas e copiáveis
- Ícones visuais

#### Semana 4: IA Simplificada
- Backend leve (200 linhas vs 2893)
- API cloud (OpenAI Vision)
- Gravar tags em EXIF/XMP
- Busca por texto simples

### WON'T HAVE (Por enquanto)
- ❌ Semantic Key Protocol (complexo demais)
- ❌ LLM local (instalação pesada)
- ❌ Visualização em mapa
- ❌ Timeline
- ❌ Detecção de faces
- ❌ Análise estética avançada

*(Essas features vão para v2.0 se houver demanda real)*

---

## 📊 COMPARAÇÃO

| Métrica | Atual | MVP Proposto | Melhoria |
|---------|-------|--------------|----------|
| **Linhas de código** | ~15.000 | ~4.000 | -73% |
| **Tempo de instalação** | 30+ min | 5 min | -83% |
| **Tamanho instalação** | 2 GB | 50 MB | -97% |
| **Dependências Python** | 8 pesadas | 2 leves | -75% |
| **Tempo até MVP** | 6+ meses | 4-5 semanas | -80% |
| **Features funcionais** | 40% | 100% | +150% |

---

## 💰 CUSTOS E RECURSOS

### Desenvolvimento
- **Tempo:** 4-5 semanas (1 desenvolvedor full-time)
- **Linhas para refatorar:** ~11.000 linhas removidas/simplificadas
- **Linhas novas:** ~2.000 linhas (features básicas)

### Operação (IA)
- **Modelo:** GPT-4 Vision API
- **Custo:** $0.01 por imagem analisada
- **Budget teste:** $50/mês = 5.000 análises
- **Mitigação:** 
  - Limite gratuito: 50 análises/usuário/mês
  - Cache de análises anteriores
  - Modo offline com tags manuais

---

## 🎯 MÉTRICAS DE SUCESSO

### Técnicas (Objetivas)
- ✅ Build em < 2 minutos
- ✅ Instalação em < 5 minutos
- ✅ App < 100 MB
- ✅ Carregar 1000 fotos em < 5 segundos
- ✅ Zero crashes em 1 hora de uso

### Negócio (Validação)
- ✅ 10 beta testers por 1 semana
- ✅ NPS > 40 (Net Promoter Score)
- ✅ 80% acham "útil" ou "muito útil"
- ✅ 50% usariam como visualizador principal
- ✅ 30% ativariam análise IA

### Red Flags (Quando Parar)
- ❌ Bugs críticos não resolvidos em 3 dias
- ❌ Feedback beta < 30% positivo
- ❌ Custo API > $100/mês no beta
- ❌ Performance < 30fps na navegação

---

## 📅 CRONOGRAMA

### Janeiro 2026

**Semana 2 (6-12 Jan)**
- Refatoração e limpeza
- Implementar operações de arquivo
- ✅ Deliverable: Build funcional com básicos

**Semana 3 (13-19 Jan)**
- Filtros e ordenação
- Melhorar thumbnails
- ✅ Deliverable: Organização funcional

**Semana 4 (20-26 Jan)**
- Redesign do painel de metadados
- GPS → localização
- ✅ Deliverable: UI profissional

**Semana 5 (27 Jan - 2 Fev)**
- Backend IA simplificado
- Integração C++/Python
- ✅ Deliverable: IA funcional

### Fevereiro 2026

**Semana 1 (3-9 Fev)**
- Polish e otimização
- Beta testing (10 usuários)
- ✅ Deliverable: MVP completo

**Semana 2 (10-16 Fev)**
- Correções baseadas em feedback
- Documentação final
- ✅ Deliverable: Release candidate

---

## ⚠️ RISCOS

| Risco | Probabilidade | Impacto | Mitigação |
|-------|--------------|---------|-----------|
| Resistência à simplificação | Média | Alto | Código vai para branch "future", não é deletado |
| API cloud cara | Baixa | Médio | Modo offline + limite gratuito + cache |
| Perda de diferencial técnico | Média | Baixo | MVP valida necessidade primeiro, SKP entra v2.0 |
| Usuários não usam IA | Alta | Baixo | App é útil sem IA (viewer puro funciona) |
| Bugs de integração | Média | Médio | Testes automatizados + beta extensivo |

---

## 💼 DECISÕES NECESSÁRIAS

### Imediatas (Esta Semana)
1. ✅/❌ **Aprovar pivô para MVP?**
2. ✅/❌ **Aceitar remoção temporária de SKP?**
3. ✅/❌ **Aceitar uso de API cloud (custo)?**

### Próximas 2 Semanas
4. Definir budget máximo para API IA
5. Selecionar beta testers (perfil)
6. Decidir plataformas (só macOS ou multi?)

---

## 🚀 PRÓXIMOS PASSOS IMEDIATOS

Se aprovado:

### Hoje
1. Criar branches: `backup-full-version` e `mvp-phase1`
2. Ler [QUICK_START_MVP.md](QUICK_START_MVP.md)
3. Começar Passo 1-2 (backup + agent simplificado)

### Esta Semana
4. Implementar operações de arquivo (Passo 4-5)
5. Build e teste inicial
6. Commit: "MVP Phase 1 - Basic operations"

### Próxima Semana
7. Sprint 2 (filtros e ordenação)
8. Daily updates no README
9. Demo sexta-feira

---

## 📚 DOCUMENTAÇÃO

### Criada
- ✅ [MVP_ANALYSIS.md](MVP_ANALYSIS.md) - Análise crítica completa
- ✅ [ROADMAP.md](ROADMAP.md) - Roadmap detalhado (5 sprints)
- ✅ [QUICK_START_MVP.md](QUICK_START_MVP.md) - Guia executável
- ✅ Este documento (resumo executivo)

### Existente
- README.md (atualizar após MVP)
- PROJECT_SUMMARY.md (atualizar após MVP)
- GETTING_STARTED.md (manter atualizado)

---

## 🎓 LIÇÕES APRENDIDAS

### O Que Fizemos Bem
1. Arquitetura técnica bem pensada
2. Escolha de tecnologias (Qt6, pybind11)
3. Suporte RAW/HEIF funcionando
4. Documentação inicial boa

### O Que Podemos Melhorar
1. **Validação antes de implementação**
   - SKP não foi validado com usuários
   - Features complexas sem demanda comprovada
   
2. **Scope management**
   - Tentamos fazer tudo de uma vez
   - Faltou MVP thinking desde o início
   
3. **User testing early**
   - Nenhum usuário real testou ainda
   - Assumimos necessidades sem validar

### Para o Futuro
- ✅ MVP primeiro, features depois
- ✅ Testar com usuários a cada sprint
- ✅ Medir antes de construir
- ✅ Simplicidade > Sofisticação

---

## 📞 CONTATO E SUPORTE

**Documentos de Referência:**
- Análise técnica: [MVP_ANALYSIS.md](MVP_ANALYSIS.md)
- Plano de execução: [ROADMAP.md](ROADMAP.md)
- Quick start: [QUICK_START_MVP.md](QUICK_START_MVP.md)

**Próxima Revisão:** Final do Sprint 1 (12 Jan 2026)

---

## ✅ RECOMENDAÇÃO FINAL

**APROVAR o pivô para MVP.**

**Razões:**
1. Código atual tem 40% de completude
2. Faltam funcionalidades básicas críticas
3. MVP pode ser entregue em 1/6 do tempo
4. Validação de mercado antes de investir em features complexas
5. Produto utilizável desde semana 2
6. Baixo risco (código complexo preservado em branch)

**Alternativa (se rejeitar):**
Continuar com plano atual = 6+ meses para produto completo, risco alto de nunca finalizar.

---

**Decisão necessária até:** 6 de Janeiro de 2026  
**Primeira demo MVP esperada:** 19 de Janeiro de 2026  
**Beta release estimado:** 9 de Fevereiro de 2026

---

*Preparado pela equipe de análise de produto*  
*Para questões: veja documentação completa em MVP_ANALYSIS.md*
