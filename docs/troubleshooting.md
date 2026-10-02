# BlueFare MultiMic Troubleshooting Guide

## 1. Android App Cannot Discover Server
- **Cause**: Wi-Fi network AP Isolation or Firewall blocking UDP broadcast.
- **Fix**: Ensure phone and PC are connected to the same Wi-Fi subnet. In the Android app Settings, manually enter the Linux server's IP address (e.g. `192.168.1.20`). Check firewall settings:
  ```bash
  sudo ufw allow 45454/tcp
  sudo ufw allow 45455/udp
  sudo ufw allow 45456/udp
  ```

## 2. Invalid PIN Pairing Error
- **Cause**: Entered PIN code does not match server PIN.
- **Fix**: Look at the terminal output or Web Dashboard (`http://127.0.0.1:8080`) of `multimic-server` for the current PIN code. Or launch server with a fixed PIN: `--code 483921`.

## 3. No Audio Output on Linux
- **Cause**: PipeWire or ALSA output device busy or misconfigured.
- **Fix**: Run `multimic-server --output alsa` or `--output pipewire`. Verify audio output devices using `pavucontrol` or `wpctl status`. Use `--record session.wav` to test WAV file generation directly.
