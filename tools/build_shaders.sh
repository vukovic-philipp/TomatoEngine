#!/usr/bin/env bash
# Rebuilds all shaders in shaders/ to SPIR-V in shaders/compiled/
# Usage: ./build_shaders.sh [shader_dir] [out_dir]

set -u

SHADER_DIR="${1:-shaders}"
OUT_DIR="${2:-$SHADER_DIR/compiled}"

if [[ ! -d "$SHADER_DIR" ]]; then
    echo "error: shader directory '$SHADER_DIR' not found" >&2
    exit 1
fi

# Pick a compiler
if command -v glslc >/dev/null 2>&1; then
    COMPILER=glslc
elif command -v glslangValidator >/dev/null 2>&1; then
    COMPILER=glslangValidator
else
    echo "error: neither glslc nor glslangValidator found in PATH" >&2
    echo "       (NixOS: add shaderc or glslang to your flake dev shell)" >&2
    exit 1
fi

# Clean rebuild
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

ok=0
fail=0

# Shader stages by extension
while IFS= read -r -d '' file; do
    rel="${file#"$SHADER_DIR"/}"
    out="$OUT_DIR/${rel}.spv"
    mkdir -p "$(dirname "$out")"

    if [[ "$COMPILER" == glslc ]]; then
        glslc "$file" -o "$out"
    else
        glslangValidator -V "$file" -o "$out"
    fi

    if [[ $? -eq 0 ]]; then
        echo "[ok]   $rel"
        ((ok++))
    else
        echo "[FAIL] $rel" >&2
        ((fail++))
    fi
done < <(find "$SHADER_DIR" -path "$OUT_DIR" -prune -o -type f \
    \( -name '*.vert' -o -name '*.frag' -o -name '*.comp' \
       -o -name '*.geom' -o -name '*.tesc' -o -name '*.tese' \) \
    -print0 | sort -z)

echo "----"
echo "compiler: $COMPILER | built: $ok | failed: $fail"

[[ $fail -eq 0 ]]