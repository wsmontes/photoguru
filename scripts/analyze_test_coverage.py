#!/usr/bin/env python3
"""
Analisa cobertura de testes do PhotoGuru
Compara arquivos de código vs arquivos de teste
"""

import os
from pathlib import Path
from typing import Dict, List, Set

def get_source_files(base_dir: Path) -> Dict[str, List[str]]:
    """Retorna arquivos .cpp por categoria"""
    sources = {
        'core': [],
        'ui': [],
        'ml': []
    }
    
    for category in sources.keys():
        src_dir = base_dir / 'src' / category
        if src_dir.exists():
            sources[category] = [
                f.stem for f in src_dir.glob('*.cpp')
            ]
    
    return sources

def get_test_files(base_dir: Path) -> Set[str]:
    """Retorna nomes dos componentes testados"""
    test_dir = base_dir / 'tests'
    tested = set()
    
    for test_file in test_dir.glob('test_*.cpp'):
        # test_metadata_reader.cpp -> metadata_reader
        component = test_file.stem.replace('test_', '')
        tested.add(component)
    
    return tested

def analyze_coverage(base_dir: Path):
    """Analisa cobertura de testes"""
    sources = get_source_files(base_dir)
    tested = get_test_files(base_dir)
    
    print("=" * 80)
    print("📊 ANÁLISE DE COBERTURA DE TESTES")
    print("=" * 80)
    
    total_files = 0
    total_tested = 0
    
    for category, files in sources.items():
        print(f"\n{'=' * 80}")
        print(f"📁 {category.upper()}")
        print('=' * 80)
        
        category_tested = 0
        for file in files:
            has_test = file in tested
            status = "✅" if has_test else "❌"
            
            total_files += 1
            if has_test:
                total_tested += 1
                category_tested += 1
            
            print(f"{status} {file:40s} {'TESTADO' if has_test else 'SEM TESTE'}")
        
        coverage = (category_tested / len(files) * 100) if files else 0
        print(f"\n📈 Cobertura {category}: {category_tested}/{len(files)} ({coverage:.0f}%)")
    
    # Resumo geral
    print("\n" + "=" * 80)
    print("🎯 RESUMO GERAL")
    print("=" * 80)
    print(f"Total de arquivos:     {total_files}")
    print(f"Arquivos testados:     {total_tested}")
    print(f"Arquivos SEM teste:    {total_files - total_tested}")
    
    overall_coverage = (total_tested / total_files * 100) if total_files else 0
    print(f"\n📊 Cobertura Total:     {overall_coverage:.0f}%")
    
    if overall_coverage == 100:
        print("\n🎉 PERFEITO! 100% de cobertura!")
    elif overall_coverage >= 80:
        print(f"\n✅ BOA cobertura! Faltam {total_files - total_tested} arquivos.")
    elif overall_coverage >= 60:
        print(f"\n⚠️  Cobertura moderada. Faltam {total_files - total_tested} arquivos.")
    else:
        print(f"\n❌ Cobertura BAIXA. Faltam {total_files - total_tested} arquivos.")
    
    # Recomendações
    print("\n" + "=" * 80)
    print("💡 ARQUIVOS SEM TESTE")
    print("=" * 80)
    
    missing_tests = []
    for category, files in sources.items():
        for file in files:
            if file not in tested:
                missing_tests.append((category, file))
    
    if missing_tests:
        print("\nPrecisa criar testes para:")
        for category, file in missing_tests:
            print(f"   - tests/test_{file}.cpp  [{category}]")
    else:
        print("\n✅ Todos os arquivos têm testes!")
    
    print("\n" + "=" * 80)

if __name__ == '__main__':
    base_dir = Path(__file__).parent.parent
    analyze_coverage(base_dir)
