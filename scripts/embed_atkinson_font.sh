#!/usr/bin/env bash
set -euo pipefail

# Embed the first non-italic TTF found in assets_gui_initial/Atkinson_Hyperlegible_Mono
# Produces Source/GUIAssets/EmbeddedFont.h (C array) using xxd if available, otherwise base64.

ATKINSON_DIR="assets/gui/raw/Atkinson_Hyperlegible_Mono"
OUT_HEADER="Source/GUIAssets/EmbeddedFont.h"

if [ ! -d "$ATKINSON_DIR" ]; then
  echo "Directory $ATKINSON_DIR not found. Run from repo root." >&2
  exit 1
fi

FONT_FILE=""
for f in "$ATKINSON_DIR"/*.ttf; do
  fname=$(basename "$f" | tr '[:upper:]' '[:lower:]')
  if [[ "$fname" == *italic* ]]; then
    continue
  fi
  FONT_FILE="$f"
  break
done

if [ -z "$FONT_FILE" ]; then
  echo "No TTF found in $ATKINSON_DIR" >&2
  exit 1
fi

echo "Embedding $FONT_FILE -> $OUT_HEADER"

if command -v xxd >/dev/null 2>&1; then
  xxd -i "$FONT_FILE" > /tmp/font_xxd.h
  CNT=$(wc -c < "$FONT_FILE")
  # extract initializer list
  awk '/unsigned char/ {flag=1; next} /unsigned int/ {flag=0} flag {print}' /tmp/font_xxd.h > /tmp/font_array.txt
  cat > "$OUT_HEADER" <<EOF
// Auto-generated EmbeddedFont.h — binary array of selected Atkinson TTF
#pragma once
#include <cstddef>
static unsigned char embedded_font[] = {
$(cat /tmp/font_array.txt)
};
static const size_t embedded_font_size = ${CNT}u;
EOF
  rm -f /tmp/font_xxd.h /tmp/font_array.txt
else
  echo "xxd not found — creating base64 header"
  base64 -w0 "$FONT_FILE" > /tmp/font.b64
  B64=
  while IFS= read -r line; do B64+="$line"; done < /tmp/font.b64
  cat > "$OUT_HEADER" <<EOF
// Auto-generated EmbeddedFont.h — base64 of selected Atkinson TTF
#pragma once
#include <string>
static const char* embedded_font_base64 = "${B64}";
EOF
  rm -f /tmp/font.b64
fi

echo "Wrote $OUT_HEADER"
