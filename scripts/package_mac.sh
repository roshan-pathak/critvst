#!/usr/bin/env bash
set -euo pipefail

# package_mac.sh
# Creates a DMG and a PKG for the built plugin bundles.
# Usage: ./scripts/package_mac.sh <build_dir> <output_dir>
# Example: ./scripts/package_mac.sh build/ ReleaseDist

BUILD_DIR="${1:-build}"
OUTPUT_DIR="${2:-dist}"
mkdir -p "$OUTPUT_DIR"

# Paths to built artefacts (expected from CMake build)
AU_BUNDLE="$BUILD_DIR/crit_artefacts/Release/AU/crit.component"
VST3_BUNDLE="$BUILD_DIR/crit_artefacts/Release/VST3/crit.vst3"

if [[ ! -d "$AU_BUNDLE" && ! -d "$VST3_BUNDLE" ]]; then
  echo "No build artefacts found in $BUILD_DIR. Build first."
  exit 1
fi

# Copy bundles to temporary staging area
STAGING_DIR="$(mktemp -d -t crit_pkg_staging)")
mkdir -p "$STAGING_DIR/Plugins/Components" "$STAGING_DIR/Plugins/VST3"

if [[ -d "$AU_BUNDLE" ]]; then
  cp -R "$AU_BUNDLE" "$STAGING_DIR/Plugins/Components/"
fi
if [[ -d "$VST3_BUNDLE" ]]; then
  cp -R "$VST3_BUNDLE" "$STAGING_DIR/Plugins/VST3/"
fi

# Optionally include presets and docs
cp -R Source/Presets "$STAGING_DIR/" 2>/dev/null || true
cp -R README.md "$STAGING_DIR/" 2>/dev/null || true

# Create DMG
DMG_NAME="$OUTPUT_DIR/crit-$(date +%Y%m%d).dmg"
echo "Creating DMG: $DMG_NAME"
# Use hdiutil to create compressed read-only DMG
hdiutil create -volname crit -srcfolder "$STAGING_DIR" -ov -format UDZO "$DMG_NAME"

# Create PKG (simple component PKG that copies bundles) -- requires macOS pkgbuild/productbuild
PKG_ROOT="$(mktemp -d -t crit_pkg_root)"
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/Components"
mkdir -p "$PKG_ROOT/Library/Audio/Plug-Ins/VST3"

if [[ -d "$AU_BUNDLE" ]]; then
  cp -R "$AU_BUNDLE" "$PKG_ROOT/Library/Audio/Plug-Ins/Components/"
fi
if [[ -d "$VST3_BUNDLE" ]]; then
  cp -R "$VST3_BUNDLE" "$PKG_ROOT/Library/Audio/Plug-Ins/VST3/"
fi

PKG_ID="com.yourcompany.crit"
PKG_VERSION="1.0"
PKG_PATH="$OUTPUT_DIR/crit-$PKG_VERSION.pkg"

echo "Creating PKG: $PKG_PATH"
pkgbuild --root "$PKG_ROOT" --identifier "$PKG_ID" --version "$PKG_VERSION" --install-location / "$PKG_PATH"

# Clean up
rm -rf "$STAGING_DIR" "$PKG_ROOT"

echo "Packaging complete. DMG: $DMG_NAME  PKG: $PKG_PATH"