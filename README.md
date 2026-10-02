# BlueFare MultiMic

**BlueFare (MultiMic)** is an open-source, high-performance system that allows multiple Android phones to voluntarily connect to a Linux computer over Wi-Fi and function as low-latency wireless audio sources (e.g. for multi-microphone audio production, podcasts, live streams, or recordings).

---

## 🔒 Privacy & Consent Principles

- **Explicit Permission**: Every Android device must explicitly grant `RECORD_AUDIO` permission.
- **Mandatory Notification**: A persistent foreground notification is displayed on Android whenever the microphone is active ("MultiMic microphone is active").
- **Visible Controls**: Clear Start/Stop microphone buttons. No background or hidden microphone activation.
- **PIN Authentication**: Secure 6-digit PIN pairing before any phone can stream audio to the Linux host.

---

## 🏗️ Architecture

```mermaid
flowchart TD
    subgraph Android Phone 1
        A1[AudioRecord API 48kHz Mono] --> E1[Opus Encoder JNI]
        E1 --> UDP1[UDP Audio Packetizer]
    end

    subgraph Android Phone 2
        A2[AudioRecord API 48kHz Mono] --> E2[Opus Encoder JNI]
        E2 --> UDP2[UDP Audio Packetizer]
    end

    subgraph Linux Host (C++20)
        UDP1 & UDP2 --> Receiver[UDP Audio Receiver :45455]
        Receiver --> JB[Jitter Buffers 40-80ms]
        JB --> Decoders[Opus Decoders + PLC]
        Decoders --> Mixer[Realtime Float Mixer]
        
        Mixer --> PW[PipeWire / ALSA Speakers]
        Mixer --> WAV[WAV File Recorder]
        
        Web[Web Dashboard :8080] <---> DM[Device Manager]
        CLI[multimic-cli] <---> TCP[TCP Control Server :45454]
    end
```

---

## 🚀 Quick Start

### 1. Build & Run Linux Server
```bash
# Clone repository and run Linux build script
./scripts/build-linux.sh

# Run server
./build/multimic-server --verbose --record session.wav
```

### 2. Build & Install Android App
```bash
# Build Android APK
./scripts/build-android.sh

# Install via ADB
adb install android/app/build/outputs/apk/debug/app-debug.apk
```

### 3. Connect & Pair
1. Launch **multimic-server** on Linux. Note the **Pairing PIN** (e.g. `483921`).
2. Open **MultiMic Client** on your Android phone.
3. Tap **Scan / Connect Server**.
4. Enter the 6-digit PIN code when prompted.
5. Tap **START MICROPHONE**.

### 4. Web Dashboard
Open `http://127.0.0.1:8080` in your web browser to manage connected devices, adjust per-device volume sliders, toggle mutes, and view live RMS/peak audio meters.

### 5. CLI Management
```bash
# List connected devices
./build/multimic-cli devices

# Adjust volume
./build/multimic-cli volume phone01 0.75

# Mute device
./build/multimic-cli mute phone01
```

---

## 📚 Documentation

- [Architecture Guide](file:///home/sabbir/Documents/BlueFare/docs/architecture.md)
- [Protocol Specification](file:///home/sabbir/Documents/BlueFare/docs/protocol.md)
- [Linux Server Guide](file:///home/sabbir/Documents/BlueFare/docs/linux-server.md)
- [Android Client Guide](file:///home/sabbir/Documents/BlueFare/docs/android-client.md)
- [Security Model](file:///home/sabbir/Documents/BlueFare/docs/security.md)
- [Troubleshooting](file:///home/sabbir/Documents/BlueFare/docs/troubleshooting.md)

---

## 📜 License

[MIT License](file:///home/sabbir/Documents/BlueFare/LICENSE)
