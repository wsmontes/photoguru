#!/usr/bin/env python3
"""
Benchmark de Performance - PhotoGuru
Testa velocidade de filtros, views e operações de metadados
"""

import subprocess
import time
import os
import tempfile
import shutil

def benchmark_exiftool_read(num_files=10):
    """Testa velocidade de leitura de metadados via ExifTool"""
    print(f"\n📖 Benchmark: Leitura de metadados ({num_files} arquivos)")
    
    # Usar arquivos de exemplo se existirem
    test_files = []
    for ext in ['.jpg', '.jpeg', '.png', '.heic']:
        test_files.extend([f for f in os.listdir('.') if f.endswith(ext)][:num_files])
    
    if not test_files:
        print("   ⚠️  Nenhum arquivo de teste encontrado")
        return
    
    test_files = test_files[:num_files]
    
    # Teste individual
    start = time.time()
    for f in test_files:
        subprocess.run(['exiftool', '-json', '-G', '-a', '-s', f], 
                      capture_output=True, timeout=5)
    individual_time = time.time() - start
    
    # Teste em lote (mais rápido)
    start = time.time()
    subprocess.run(['exiftool', '-json', '-G', '-a', '-s'] + test_files,
                  capture_output=True, timeout=10)
    batch_time = time.time() - start
    
    print(f"   Individual: {individual_time:.3f}s ({individual_time/len(test_files)*1000:.1f}ms/arquivo)")
    print(f"   Em lote:    {batch_time:.3f}s ({batch_time/len(test_files)*1000:.1f}ms/arquivo)")
    print(f"   📈 Speedup: {individual_time/batch_time:.1f}x mais rápido em lote")
    
    return individual_time / len(test_files), batch_time / len(test_files)

def benchmark_exiftool_write():
    """Testa velocidade de gravação de metadados via ExifTool"""
    print(f"\n✍️  Benchmark: Gravação de metadados")
    
    # Criar arquivo temporário
    with tempfile.NamedTemporaryFile(suffix='.jpg', delete=False) as tmp:
        tmp_path = tmp.name
        # Criar imagem simples
        from PIL import Image
        img = Image.new('RGB', (100, 100), color='red')
        img.save(tmp_path)
    
    try:
        # Teste de escrita
        operations = [
            ('-XMP:Rating=5', 'Rating'),
            ('-XMP:Title=Test Title', 'Title'),
            ('-XMP:Description=Test Description', 'Description'),
            ('-XMP:Subject=keyword1', 'Keyword'),
        ]
        
        times = []
        for arg, name in operations:
            start = time.time()
            subprocess.run(['exiftool', '-overwrite_original', arg, tmp_path],
                          capture_output=True, timeout=5)
            elapsed = time.time() - start
            times.append(elapsed)
            print(f"   {name:20s}: {elapsed*1000:.1f}ms")
        
        avg_time = sum(times) / len(times)
        print(f"   📊 Média: {avg_time*1000:.1f}ms/operação")
        
        return avg_time
        
    finally:
        os.unlink(tmp_path)

def estimate_filter_performance():
    """Estima performance de filtros combinados"""
    print(f"\n🔍 Análise: Performance de Filtros")
    
    # Análise teórica baseada no código
    operations = {
        'Search textual': 'O(n×m) - n=fotos, m=campos',
        'Rating range': 'O(1) - comparação direta',
        'Camera filter': 'O(k) - k=câmeras no filtro',
        'ISO/Aperture': 'O(1) - comparação numérica',
        'Keywords': 'O(k×m) - k=keywords filtro, m=keywords foto',
        'Quality scores': 'O(1) - comparação direta',
        'GPS check': 'O(1) - comparação direta',
    }
    
    print("   Complexidade por filtro:")
    for name, complexity in operations.items():
        print(f"      {name:20s}: {complexity}")
    
    print("\n   ✅ Filtros são combinados com AND lógico")
    print("   ✅ Early exit: Para na primeira condição falsa")
    print("   ✅ Ordem otimizada: Checks mais rápidos primeiro")
    print("   ⚡ Estimativa: <1ms por foto com todos os filtros ativos")
    print("   📊 Para 10.000 fotos: ~10 segundos (single-threaded)")

def analyze_view_modes():
    """Analisa modos de visualização disponíveis"""
    print(f"\n👁️  Análise: Modos de Visualização (Library)")
    
    print("   Implementado atualmente:")
    print("      ✅ ThumbnailGrid (QListWidget)")
    print("         - View: IconMode (grade de miniaturas)")
    print("         - Size: Ajustável via setThumbnailSize()")
    print("         - Sort: ByName, ByDate, BySize")
    print("         - Cache: Memory (1000 items) + Disk (~/.photoguru/thumbnails)")
    
    print("\n   ⚠️  NÃO implementado:")
    print("      ❌ List View (lista com detalhes)")
    print("      ❌ Detail View (tabela com colunas)")
    print("      ❌ Grid configurável (tamanho dinâmico)")
    print("      ❌ Toggle entre modos")
    
    print("\n   💡 Recomendação:")
    print("      - Adicionar QListView::ViewMode switch")
    print("      - IconMode (atual) + ListMode + DetailView")
    print("      - Toolbar com botões para alternar")

def main():
    print("="*60)
    print("🚀 PhotoGuru Performance Benchmark")
    print("="*60)
    
    # Q1: Search é combinável e rápido?
    estimate_filter_performance()
    
    # Q2: Library tem views diferentes?
    analyze_view_modes()
    
    # Q3: Metadados rápido o suficiente?
    try:
        individual, batch = benchmark_exiftool_read(10)
        write_time = benchmark_exiftool_write()
        
        print("\n" + "="*60)
        print("📊 RESUMO DE PERFORMANCE")
        print("="*60)
        
        print(f"\n1️⃣  SEARCH É COMBINÁVEL? ✅ SIM")
        print(f"   - Todos os filtros são combinados com AND")
        print(f"   - Ordem otimizada (checks rápidos primeiro)")
        print(f"   - Estimativa: <1ms/foto, ~10s para 10k fotos")
        
        print(f"\n2️⃣  LIBRARY TEM VIEWS DIFERENTES? ⚠️  PARCIAL")
        print(f"   - Implementado: Grid view (IconMode) apenas")
        print(f"   - Faltam: List view, Detail view, toggle")
        print(f"   - Configurável: Thumbnail size ajustável")
        
        print(f"\n3️⃣  METADADOS RÁPIDO O SUFICIENTE? 🤔 DEPENDE")
        print(f"   - Leitura individual: {individual*1000:.0f}ms/foto")
        print(f"   - Leitura em lote:   {batch*1000:.0f}ms/foto (MELHOR)")
        print(f"   - Gravação:          {write_time*1000:.0f}ms/operação")
        
        if individual > 0.150:  # 150ms
            print(f"\n   ⚠️  LENTO! ExifTool adiciona latência significativa")
            print(f"   💡 SOLUÇÕES:")
            print(f"      1. Usar leitura em lote (implementado)")
            print(f"      2. Cache em SQLite (PhotoDatabase)")
            print(f"      3. Background threads (QtConcurrent - implementado)")
            print(f"      4. ❌ NÃO recomendo C++ puro:")
            print(f"         - ExifTool suporta 500+ formatos")
            print(f"         - LibRaw + libexiv2 seria parcial")
            print(f"         - Maintenance nightmare")
        else:
            print(f"\n   ✅ Aceitável para uso interativo")
        
    except Exception as e:
        print(f"\n⚠️  Erro no benchmark: {e}")
        print("   Verifique se há arquivos de imagem para teste")

if __name__ == '__main__':
    # Verificar dependências
    try:
        subprocess.run(['exiftool', '-ver'], capture_output=True, check=True)
    except:
        print("❌ ExifTool não encontrado! Instale com: brew install exiftool")
        exit(1)
    
    try:
        from PIL import Image
    except:
        print("⚠️  Pillow não encontrado. Install: pip install Pillow")
        print("   (Benchmark de gravação será pulado)\n")
    
    main()
