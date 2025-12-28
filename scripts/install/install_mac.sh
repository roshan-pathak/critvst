#!/usr/bin/env bash
set -euo pipefail

# install_mac.sh
# Copies built AU and VST3 bundles and the presets folder into system locations.
# Usage: install_mac.sh [build_dir]

BUILD_DIR="${1:-$(pwd)}"
# If a project root is passed as the first argument and it contains CMakeLists.txt, use it.
if [ -n "${1-}" ] && [ -f "${1}/CMakeLists.txt" ]; then
  PROJECT_ROOT="$(cd "${1}" && pwd)"
else
  PROJECT_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
fi

AU_SRC="$PROJECT_ROOT/build/crit_artefacts/Release/AU/crit.component"
VST3_SRC="$PROJECT_ROOT/build/crit_artefacts/Release/VST3/crit.vst3"
PRESETS_SRC="$PROJECT_ROOT/Source/Presets"

AU_DST="/Library/Audio/Plug-Ins/Components"
VST3_DST="/Library/Audio/Plug-Ins/VST3"
PRESETS_DST="/Library/Application Support/crit/presets"

if [ ! -d "$PRESETS_SRC" ]; then
  echo "No presets found at $PRESETS_SRC"
fi

echo "Installing AU -> $AU_DST"
if [ -d "$AU_SRC" ]; then
  if [ ! -w "$AU_DST" ]; then
    echo "Requires sudo to write to $AU_DST"
    sudo rm -rf "$AU_DST/crit.component" || true
    sudo cp -R "$AU_SRC" "$AU_DST/"
    sudo chown -R root:wheel "$AU_DST/crit.component" || true
  else
    rm -rf "$AU_DST/crit.component" || true
    cp -R "$AU_SRC" "$AU_DST/"
  fi
else
  echo "Warning: AU bundle not found at $AU_SRC"
fi

echo "Installing VST3 -> $VST3_DST"
if [ -d "$VST3_SRC" ]; then
  if [ ! -w "$VST3_DST" ]; then
    echo "Requires sudo to write to $VST3_DST"
    sudo rm -rf "$VST3_DST/crit.vst3" || true
    sudo cp -R "$VST3_SRC" "$VST3_DST/"
    sudo chown -R root:wheel "$VST3_DST/crit.vst3" || true
  else
    rm -rf "$VST3_DST/crit.vst3" || true
    cp -R "$VST3_SRC" "$VST3_DST/"
  fi
else
  echo "Warning: VST3 bundle not found at $VST3_SRC"
fi

# Install presets
echo "Installing presets -> $PRESETS_DST"
if [ -d "$PRESETS_SRC" ]; then
  if [ ! -d "$PRESETS_DST" ]; then
    if [ ! -w "/Library/Application Support" ]; then
      sudo mkdir -p "$PRESETS_DST"
      sudo cp -R "$PRESETS_SRC/" "$PRESETS_DST/"
      sudo chown -R root:wheel "$PRESETS_DST"
    else
      mkdir -p "$PRESETS_DST"
      cp -R "$PRESETS_SRC/" "$PRESETS_DST/"
    fi
  else
    # Merge or overwrite files
    if [ ! -w "$PRESETS_DST" ]; then
      sudo cp -R "$PRESETS_SRC/." "$PRESETS_DST/"
      sudo chown -R root:wheel "$PRESETS_DST"
    else
      cp -R "$PRESETS_SRC/." "$PRESETS_DST/"
    fi
  fi
else
  echo "No presets folder to install."
fi

echo "Install complete." 
