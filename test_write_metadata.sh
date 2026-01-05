#!/bin/bash
# Teste direto para validar escrita de metadados com ExifTool

set -e

# Criar diretório temporário
TMPDIR=$(mktemp -d)
TESTIMG="$TMPDIR/test.jpg"

echo "=== Criando imagem de teste ==="
# Criar imagem simples (requer ImageMagick)
if command -v convert &> /dev/null; then
    convert -size 100x100 xc:blue "$TESTIMG"
elif command -v sips &> /dev/null; then
    # macOS nativo
    sips -s format jpeg -z 100 100 /System/Library/Desktop\ Pictures/Solid\ Colors/Solid\ Aqua\ Blue.png --out "$TESTIMG" 2>/dev/null || {
        # Fallback: copiar qualquer JPEG existente
        cp /System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/JPEG.icns "$TESTIMG" 2>/dev/null || {
            echo "ERRO: Não foi possível criar imagem de teste"
            exit 1
        }
    }
fi

echo "Imagem criada: $TESTIMG"
echo

echo "=== TESTE 1: ExifTool direto (modo normal) ==="
exiftool -overwrite_original -XMP:Rating=5 "$TESTIMG"
echo

echo "=== TESTE 2: Verificar se foi escrito ==="
exiftool -XMP:Rating "$TESTIMG"
echo

echo "=== TESTE 3: ExifTool stay-open mode ==="
# Criar arquivo de comandos
CMDFILE="$TMPDIR/commands.txt"
cat > "$CMDFILE" <<EOF
-overwrite_original
-XMP:Title=Test Title from Stay-Open
$TESTIMG
-execute
-stay_open
False
EOF

echo "Comandos enviados:"
cat "$CMDFILE"
echo

# Executar via stdin
exiftool -stay_open True -@ - < "$CMDFILE"

echo
echo "=== VERIFICAÇÃO FINAL ==="
exiftool -XMP:Rating -XMP:Title "$TESTIMG"

# Cleanup
rm -rf "$TMPDIR"
