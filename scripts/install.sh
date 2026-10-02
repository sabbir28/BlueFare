#!/usr/bin/env bash
set -e

echo "=== Installing BlueFare MultiMic Linux Executables ==="
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT_DIR/build"

if [ ! -f "$BUILD_DIR/multimic-server" ]; then
    echo "Executables not found. Running build..."
    "$SCRIPT_DIR/build-linux.sh"
fi

INSTALL_PREFIX="${PREFIX:-/usr/local}"

echo "Installing binaries to $INSTALL_PREFIX/bin..."
install -d "$INSTALL_PREFIX/bin"
install -m 755 "$BUILD_DIR/multimic-server" "$INSTALL_PREFIX/bin/multimic-server"
install -m 755 "$BUILD_DIR/multimic-cli" "$INSTALL_PREFIX/bin/multimic-cli"

echo "=== Installation Complete! ==="
echo "You can now run 'multimic-server' or 'multimic-cli' from any terminal."
