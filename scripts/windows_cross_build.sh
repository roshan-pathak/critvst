#!/usr/bin/env bash
set -euo pipefail
# Usage: windows_cross_build.sh [x86_64|i686]

ARCH=${1:-x86_64}

if ! command -v ${ARCH}-w64-mingw32-g++ >/dev/null 2>&1 ; then
  echo "Cross-compiler not found: ${ARCH}-w64-mingw32-g++."
  echo "Install mingw-w64 via Homebrew: brew install mingw-w64"
  exit 1
fi

BASEDIR=$(pwd)
BUILD_DIR="${BASEDIR}/build-windows-${ARCH}"
mkdir -p "$BUILD_DIR"

echo "Configuring CMake with mingw toolchain (ARCH=${ARCH})"
  cmake -S "$BASEDIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="${BASEDIR}/scripts/mingw_toolchain.cmake" \
    -DMINGW_ARCH="${ARCH}" \
    -DCMAKE_BUILD_TYPE=Release

echo "Building (Release)"
cmake --build "$BUILD_DIR" --config Release -- -j$(sysctl -n hw.ncpu)

echo "Build complete — artifacts are in: $BUILD_DIR"
echo "Note: Building JUCE VST/AU plugins for Windows via mingw on macOS can be fragile; for full plugin builds consider a native Windows build or CI." 
