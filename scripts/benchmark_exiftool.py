#!/usr/bin/env python3
"""
Benchmark ExifTool: Daemon vs Individual Process
Compara performance de stay-open mode vs fork/exec por chamada
"""

import subprocess
import time
import os
import sys
from pathlib import Path

def create_test_image(output_path: str):
    """Cria imagem de teste com metadados"""
    # Cria imagem simples 100x100 pixels
    from PIL import Image
    img = Image.new('RGB', (100, 100), color='red')
    img.save(output_path, 'JPEG', quality=95)
    
    # Adiciona metadados básicos
    subprocess.run([
        'exiftool', 
        '-overwrite_original',
        f'-Title=Test Image',
        f'-Description=Benchmark test image',
        f'-Rating=4',
        f'-Keywords=test,benchmark',
        output_path
    ], capture_output=True)
    
    return output_path

def benchmark_individual_process(image_path: str, iterations: int = 20):
    """Benchmark: fork/exec por chamada (modo antigo)"""
    print(f"\n🔹 Individual Process Mode (fork/exec por chamada)")
    print(f"   Testando {iterations} leituras...")
    
    times = []
    for i in range(iterations):
        start = time.perf_counter()
        result = subprocess.run(
            ['exiftool', '-json', '-a', '-s', image_path],
            capture_output=True,
            text=True
        )
        elapsed = (time.perf_counter() - start) * 1000  # ms
        times.append(elapsed)
        
        if i % 5 == 0:
            print(f"   [{i+1}/{iterations}] {elapsed:.1f}ms")
    
    avg = sum(times) / len(times)
    min_t = min(times)
    max_t = max(times)
    
    print(f"\n   📊 Resultados:")
    print(f"      Média:  {avg:.1f}ms")
    print(f"      Mínimo: {min_t:.1f}ms")
    print(f"      Máximo: {max_t:.1f}ms")
    
    return avg

def benchmark_stay_open(image_path: str, iterations: int = 20):
    """Benchmark: stay-open mode (ExifToolDaemon)"""
    print(f"\n🔸 Stay-Open Daemon Mode (ExifToolDaemon)")
    print(f"   Testando {iterations} leituras...")
    
    # Inicia processo stay-open
    process = subprocess.Popen(
        ['exiftool', '-stay_open', 'True', '-@', '-'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=0
    )
    
    times = []
    for i in range(iterations):
        start = time.perf_counter()
        
        # Envia comando
        process.stdin.write(f'-json\n-a\n-s\n{image_path}\n-execute\n')
        process.stdin.flush()
        
        # Lê resposta até {ready}
        response = []
        while True:
            line = process.stdout.readline()
            if '{ready}' in line:
                break
            response.append(line)
        
        elapsed = (time.perf_counter() - start) * 1000  # ms
        times.append(elapsed)
        
        if i % 5 == 0:
            print(f"   [{i+1}/{iterations}] {elapsed:.1f}ms")
    
    # Fecha daemon
    process.stdin.write('-stay_open\nFalse\n')
    process.stdin.close()
    process.wait()
    
    avg = sum(times) / len(times)
    min_t = min(times)
    max_t = max(times)
    
    print(f"\n   📊 Resultados:")
    print(f"      Média:  {avg:.1f}ms")
    print(f"      Mínimo: {min_t:.1f}ms")
    print(f"      Máximo: {max_t:.1f}ms")
    
    return avg

def main():
    print("=" * 70)
    print("📊 BENCHMARK: ExifTool Performance")
    print("=" * 70)
    
    # Verifica exiftool
    try:
        subprocess.run(['exiftool', '-ver'], capture_output=True, check=True)
    except:
        print("❌ ExifTool não encontrado. Instale com: brew install exiftool")
        sys.exit(1)
    
    # Cria imagem de teste
    test_dir = Path(__file__).parent.parent / 'build' / 'test_data'
    test_dir.mkdir(parents=True, exist_ok=True)
    test_image = str(test_dir / 'benchmark_test.jpg')
    
    print(f"\n📁 Criando imagem de teste: {test_image}")
    try:
        create_test_image(test_image)
        print(f"   ✅ Imagem criada com metadados")
    except ImportError:
        print("   ⚠️  PIL não instalado, usando imagem vazia")
        Path(test_image).touch()
    
    # Benchmarks
    iterations = 30
    
    old_avg = benchmark_individual_process(test_image, iterations)
    new_avg = benchmark_stay_open(test_image, iterations)
    
    # Comparação
    speedup = old_avg / new_avg
    improvement = ((old_avg - new_avg) / old_avg) * 100
    
    print("\n" + "=" * 70)
    print("🏁 COMPARAÇÃO FINAL")
    print("=" * 70)
    print(f"Individual Process:  {old_avg:.1f}ms")
    print(f"Stay-Open Daemon:    {new_avg:.1f}ms")
    print(f"\n🚀 Speedup:          {speedup:.1f}x mais rápido")
    print(f"📈 Melhoria:         {improvement:.0f}% mais eficiente")
    
    if speedup >= 5:
        print(f"\n✨ EXCELENTE! ExifToolDaemon entregou {speedup:.1f}x de speedup!")
    elif speedup >= 3:
        print(f"\n✅ BOM! ExifToolDaemon acelerou {speedup:.1f}x")
    else:
        print(f"\n⚠️  Speedup abaixo do esperado: {speedup:.1f}x (esperado 5-10x)")
    
    # Limpeza
    os.unlink(test_image)
    print(f"\n🧹 Limpeza concluída")

if __name__ == '__main__':
    main()
