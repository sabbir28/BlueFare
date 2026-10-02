#!/usr/bin/env bash
set -e

echo "=== Building BlueFare MultiMic Android Client APK ==="
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$ROOT_DIR/android"
if [ -f "./gradlew" ]; then
    ./gradlew assembleDebug
else
    gradle assembleDebug
fi

echo "=== Android Build Completed Successfully! ==="
echo "APK location: $ROOT_DIR/android/app/build/outputs/apk/debug/app-debug.apk"
