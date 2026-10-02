# BlueFare Architecture Documentation

## System Architecture

```mermaid
graph TD
    subgraph Android Phone 1
        A1[AudioRecord API] --> E1[Opus Encoder JNI]
        E1 --> T1[WifiTransport UDP]
    end

    subgraph Android Phone 2
        A2[AudioRecord API] --> E2[Opus Encoder JNI]
        E2 --> T2[WifiTransport UDP]
    end

    subgraph Linux MultiMic Host
        T1 --> R[UDP Audio Receiver]
        T2 --> R
        
        R --> JB1[Jitter Buffer Phone 1]
        R --> JB2[Jitter Buffer Phone 2]
        
        JB1 --> D1[Opus Decoder 1]
        JB2 --> D2[Opus Decoder 2]
        
        D1 --> M[Realtime Float Mixer]
        D2 --> M
        
        M --> O1[PipeWire / ALSA Soundcard]
        M --> O2[WAV File Master Recorder]
        
        W[Web Dashboard Server :8080] <--> DM[Device Manager]
        CLI[multimic-cli] <--> TCP[TCP Control Server :45454]
        TCP <--> DM
    end
```

## Component Breakdown

1. **Android MultiMic Client**:
   - `AudioRecord`: Captures 48000 Hz, mono 16-bit PCM audio.
   - `Foreground Service`: Runs active recording session with persistent notification.
   - `Opus Encoder`: Converts 20ms PCM frames to Opus compressed frames (32 kbps default).
   - `NetworkClient`: UDP packetizer and TCP PIN pairing client.

2. **Linux Server (`multimic-server`)**:
   - `TcpServer`: Handles device discovery, authentication, session tokens, and CLI control commands.
   - `DiscoveryBeacon`: Broadcasts server availability on LAN via UDP port 45456.
   - `AudioReceiver`: High-performance UDP socket loop validating sessions and routing incoming frames to per-device buffers.
   - `AudioJitterBuffer`: 40-80ms reordering buffer with missing packet detection and Opus PLC (Packet Loss Concealment).
   - `AudioMixer`: Multi-channel float mixing engine with per-device gain, mute, volume ramping, dynamic headroom limiter, clipping prevention, and peak/RMS level meters.
   - `AudioOutput`: Direct output to PipeWire / ALSA audio subsystems and master WAV file recorder (`--record session.wav`).
   - `WebServer`: Built-in HTTP dashboard at `http://127.0.0.1:8080` displaying connected devices, real-time volume controls, mute buttons, and level meters.
