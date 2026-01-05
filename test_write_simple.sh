#!/bin/bash
# Teste simples de escrita de metadados

set -e

TMPDIR=$(mktemp -d)
TESTIMG="$TMPDIR/test.jpg"

echo "=== Criando imagem de teste ==="
# Usar Python para criar JPEG simples
python3 -c "
from PIL import Image
img = Image.new('RGB', (100, 100), color='blue')
img.save('$TESTIMG', 'JPEG')
print('Imagem criada: $TESTIMG')
"

ls -lh "$TESTIMG"
echo

echo "=== TESTE 1: ExifTool direto ==="
exiftool -overwrite_original -XMP:Rating=5 "$TESTIMG" 2>&1
echo

echo "=== Verificar ==="
exiftool -XMP:Rating "$TESTIMG" 2>&1 || echo "Rating não encontrado"
echo

echo "=== TESTE 2: Stay-open mode ==="
(
    # Iniciar daemon
    exec 3> >(exiftool -stay_open True -@ -)
    
    # Enviar comando
    echo "-overwrite_original" >&3
    echo "-XMP:Title=Test Title" >&3
    echo "$TESTIMG" >&3
    echo "-execute" >&3
    
    # Aguardar
    sleep 1
    
    # Encerrar
    echo "-stay_open" >&3
    echo "False" >&3
    
    exec 3>&-
)

echo
echo "=== Verificação Final ==="
exiftool -XMP:Rating -XMP:Title "$TESTIMG" 2>&1

# Cleanup
rm -rf "$TMPDIR"
