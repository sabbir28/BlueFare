# BlueFare MultiMic Security & Privacy Model

## 1. Explicit Consent & Transparency
- Microphones are strictly controlled by the user.
- Hidden or covert recording is prohibited by architectural design.
- The Android client requires explicit user permission for `RECORD_AUDIO`.
- When active, Android displays an un-dismissable foreground service notification:
  `MultiMic microphone is active`

## 2. Session Authentication & Device Pairing
- Random 6-digit PIN code displayed on the Linux host terminal and web UI.
- Devices must submit the valid PIN over TCP control channel before an authenticated `session_id` token is issued.
- Unauthenticated UDP audio packets or invalid session IDs are immediately rejected by the `AudioReceiver` module.

## 3. Network Isolation
- Operates entirely on the Local Area Network (LAN).
- Zero external cloud services, third-party tracking, or external telemetry required.
