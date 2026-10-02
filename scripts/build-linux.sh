#!/usr/bin/env bash
set -e

echo "=== Building BlueFare MultiMic Linux Server & CLI ==="
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

BUILD_DIR="$ROOT_DIR/build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$ROOT_DIR/server"
make -j$(nproc)
ctest --output-on-failure

echo "=== Linux Build Completed Successfully! ==="
echo "Executables generated:"
echo "  - $BUILD_DIR/multimic-server"
echo "  - $BUILD_DIR/multimic-cli"
