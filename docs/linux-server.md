# Linux MultiMic Server User & CLI Guide

## Building & Installation
Run the automated build and installation scripts:
```bash
./scripts/build-linux.sh
sudo ./scripts/install.sh
```

## Running the Server
Launch `multimic-server` from terminal:
```bash
multimic-server
```

### Options:
- `--port 45454`: Control TCP port (default: 45454)
- `--audio-port 45455`: UDP Audio port (default: 45455)
- `--web-port 8080`: Web Dashboard port (default: 8080)
- `--config config.json`: Load JSON configuration file
- `--output pipewire`: Audio output driver (`pipewire`, `alsa`, `dummy`)
- `--record session.wav`: Save mixed audio stream to WAV recording file
- `--code 483921`: Set custom 6-digit PIN pairing code
- `--verbose`: Enable verbose debug log output

## Using `multimic-cli`
Manage connected Android microphone devices from the CLI:

```bash
# List all connected devices
multimic-cli devices

# Show server status
multimic-cli status

# Adjust device volume (0.0 to 2.0)
multimic-cli volume phone01 0.75

# Mute / Unmute device
multimic-cli mute phone01
multimic-cli unmute phone01

# Start / Stop stream
multimic-cli start phone01
multimic-cli stop phone01
```

## Web Dashboard
Open `http://127.0.0.1:8080` in your web browser while the server is running to view connected devices, live RMS/peak level meters, volume sliders, and mute controls.
