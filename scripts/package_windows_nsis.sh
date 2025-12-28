#!/usr/bin/env bash
set -euo pipefail
# Usage: package_windows_nsis.sh <build_dir> <output_dir>

BUILD_DIR=${1:-build}
OUT_DIR=${2:-dist}

NSIS_SCRIPT="$(dirname "$0")/installer.nsi"

mkdir -p "$OUT_DIR"

STAGE_DIR=$(mktemp -d)
trap 'rm -rf "$STAGE_DIR"' EXIT

echo "Staging Windows artifacts from: $BUILD_DIR"
cp -R "$BUILD_DIR"/* "$STAGE_DIR/"

echo "Building NSIS installer (requires makensis on PATH)"
makensis -DOUT_DIR="$OUT_DIR" -DSTAGE_DIR="$STAGE_DIR" "$NSIS_SCRIPT"

echo "Installer generated in: $OUT_DIR"
