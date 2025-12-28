#!/usr/bin/env bash
set -euo pipefail
# Usage: package_mac_pkgs.sh <build_dir> <output_dir> <arch>
# arch: arm64 | x86_64

BUILD_DIR=${1:-build}
OUT_DIR=${2:-dist}
ARCH=${3:-arm64}

VERSION="1.0.0"
IDENTIFIER_BASE="com.yourcompany.crit"

STAGE_DIR=$(mktemp -d)
trap 'rm -rf "$STAGE_DIR"' EXIT

mkdir -p "$OUT_DIR"

echo "Staging bundles from: $BUILD_DIR/crit_artefacts/Release"
cp -R "$BUILD_DIR/crit_artefacts/Release"/* "$STAGE_DIR/"

# If a signing identity is provided, codesign the plugin bundles in the staging tree
if [ -n "${SIGN_IDENTITY:-}" ]; then
    echo "Signing bundles in staging tree with identity: $SIGN_IDENTITY"
    # Find common bundle types and sign them
    find "$STAGE_DIR" -type d \( -name "*.component" -o -name "*.vst3" -o -name "*.plugin" \) -print0 | while IFS= read -r -d '' bundle; do
        echo "Signing $bundle"
        /usr/bin/codesign --timestamp --sign "$SIGN_IDENTITY" --force --deep "$bundle" || true
    done
fi

# Prepare component package
COMPONENT_PKG="$OUT_DIR/crit-${VERSION}-${ARCH}.pkg"
echo "Creating component package: $COMPONENT_PKG"

pkgbuild --root "$STAGE_DIR" \
    --identifier "${IDENTIFIER_BASE}.component" \
    --version "$VERSION" \
    --install-location "/" \
    "$COMPONENT_PKG"

# Optionally wrap in a product archive (productbuild) to make it more flexible
PRODUCT_PKG="$OUT_DIR/crit-${VERSION}-${ARCH}-product.pkg"
echo "Building product archive: $PRODUCT_PKG"

productbuild --package "$COMPONENT_PKG" "$PRODUCT_PKG"

echo "PKG(s) created in: $OUT_DIR"
