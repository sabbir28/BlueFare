# Android MultiMic Client User & Developer Guide

## Minimum & Target Requirements
- Minimum Android Version: Android 12 (API level 31)
- Target SDK: Android 14 (API level 34)
- Language: Java (with C++ NDK Opus JNI)

## Permissions & Privacy Model
The MultiMic Android application adheres strictly to user privacy and consent principles:
- **Explicit Runtime Permission**: Requests `RECORD_AUDIO` and `POST_NOTIFICATIONS` permissions.
- **Foreground Service Notice**: Whenever the microphone is streaming, Android displays a mandatory persistent notification stating:
  > **MultiMic microphone is active**
- **Visible Start/Stop Controls**: The main screen and notification panel both contain explicit Start/Stop buttons.
- **No Background Activation**: The application never accesses the microphone covertly or when stopped by the user.

## Building the APK
Run the build script from repository root:
```bash
./scripts/build-android.sh
```
Or via Gradle:
```bash
cd android
./gradlew assembleDebug
```
The compiled APK will be located at `android/app/build/outputs/apk/debug/app-debug.apk`.

## Installation & Setup
1. Install `app-debug.apk` onto your Android device:
   ```bash
   adb install android/app/build/outputs/apk/debug/app-debug.apk
   ```
2. Connect your Android phone to the same Wi-Fi network as your Linux computer.
3. Open **MultiMic Client**.
4. Tap **Scan / Connect Server** (or enter your Linux PC IP manually in Settings).
5. Enter the 6-digit **Pairing PIN** code shown on your Linux server screen (e.g. `483921`).
6. Tap **START MICROPHONE**.
