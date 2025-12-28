#!/usr/bin/env bash
set -euo pipefail

# Downloads Noto Sans Gujarati TTF and embeds it into Source/GUIAssets/EmbeddedFont.h
# Run from repo root: ./scripts/download_and_embed_font.sh

FONT_DIR="assets/fonts"
mkdir -p "$FONT_DIR"

FONT_URL="https://github.com/googlefonts/noto-fonts/raw/main/unhinted/ttf/NotoSansGujarati/NotoSansGujarati-Regular.ttf"
FONT_FILE="$FONT_DIR/NotoSansGujarati-Regular.ttf"

echo "Downloading Noto Sans Gujarati..."
curl -L --fail -o "$FONT_FILE" "$FONT_URL"

echo "Embedding font into C++ header..."
HEADER_FILE="Source/GUIAssets/EmbeddedFont.h"

# Use xxd to convert to C array if available, otherwise use base64 fallback
if command -v xxd >/dev/null 2>&1; then
  xxd -i "$FONT_FILE" > /tmp/font_xxd.h
  CNT=$(wc -c < "$FONT_FILE")
  cat > "$HEADER_FILE" <<EOF
// Auto-generated embedded font header
#pragma once
#include <cstddef>
static unsigned char embedded_font[] = {
$(sed -n '1,200000p' /tmp/font_xxd.h | sed -n '1,$p' | sed -n '1,200000p' | awk '/unsigned char/{flag=1;next} /};/{flag=0} flag{print}' )
};
static const size_t embedded_font_size = ${CNT}u;
EOF
  rm -f /tmp/font_xxd.h
else
  echo "xxd not found; creating base64 header instead"
  base64 -w0 "$FONT_FILE" > /tmp/font.b64
  B64=$(cat /tmp/font.b64)
  cat > "$HEADER_FILE" <<EOF
// Auto-generated embedded font header (base64)
#pragma once
#include <string>
static const char* embedded_font_base64 = "${B64}";
EOF
  rm -f /tmp/font.b64
fi

echo "Embedded header written to $HEADER_FILE"
