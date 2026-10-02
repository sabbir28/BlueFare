# BlueFare MultiMic Network Protocol Specification

Version: 1.0  
Magic Header: `MULTIMIC` (TCP), `MMAU` (UDP Audio)

## 1. Overview

The MultiMic network protocol specifies the binary communication format between Android microphone client devices and the Linux MultiMic host server over local Wi-Fi networks.

Communication is split into two channels:
1. **TCP Control Channel (Default Port 45454)**: Session pairing, security authentication, stream start/stop commands, metadata sync, CLI controls.
2. **UDP Audio Channel (Default Port 45455)**: Low-latency real-time Opus-encoded audio packets.
3. **UDP Discovery Channel (Default Port 45456)**: LAN server broadcast beaconing ("MultiMic server available").

---

## 2. TCP Control Channel Format

Every TCP message consists of a fixed 14-byte binary header followed by a variable-length JSON string payload. All multi-byte integers are encoded in **Network Byte Order (Big Endian)**.

### Header (14 bytes)

| Field | Size (Bytes) | Type | Description |
|---|---|---|---|
| Magic | 8 | uint64_t | Fixed magic constant `0x4D554C54494D4943ULL` ("MULTIMIC") |
| Version | 1 | uint8_t | Protocol version (`0x01`) |
| Message Type | 1 | uint8_t | Message type identifier enum |
| Payload Length | 4 | uint32_t | Length of following JSON payload in bytes |

### Message Types (uint8_t)
- `0x01`: `DISCOVERY_REQUEST`
- `0x02`: `DISCOVERY_RESPONSE`
- `0x03`: `PAIR_REQUEST`
- `0x04`: `PAIR_RESPONSE`
- `0x05`: `START_STREAM_REQUEST`
- `0x06`: `START_STREAM_RESPONSE`
- `0x07`: `STOP_STREAM_REQUEST`
- `0x08`: `STOP_STREAM_RESPONSE`
- `0x09`: `HEARTBEAT_PING`
- `0x0A`: `HEARTBEAT_PONG`
- `0x0B`: `CLI_COMMAND`
- `0x0C`: `CLI_RESPONSE`

---

## 3. UDP Audio Packet Format

Every UDP packet contains a fixed 50-byte binary header followed by the Opus encoded payload.

### Fixed Audio Header (50 bytes)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       MAGIC (0x4D4D4155)                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    VERSION    |      TYPE     |                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
|                                                               |
+                       DEVICE_ID (16 bytes)                    +
|                                                               |
+                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               |          SESSION_ID           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                     SEQUENCE_NUM (64-bit)                     +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                     TIMESTAMP_US (64-bit)                     +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          SAMPLE_RATE                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    CHANNELS   |     CODEC     |         PAYLOAD_LEN           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        OPUS PAYLOAD ...                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field | Size (Bytes) | Type | Value / Description |
|---|---|---|---|
| MAGIC | 4 | uint32_t | `0x4D4D4155` ("MMAU") |
| VERSION | 1 | uint8_t | `1` |
| TYPE | 1 | uint8_t | `1` (AUDIO_DATA) |
| DEVICE_ID | 16 | char[16] | Null-padded device string (e.g. `phone01`) |
| SESSION_ID | 4 | uint32_t | Session ID issued during PIN pairing |
| SEQUENCE_NUM | 8 | uint64_t | Monotonically increasing sequence number |
| TIMESTAMP_US | 8 | uint64_t | Microseconds timestamp |
| SAMPLE_RATE | 4 | uint32_t | e.g. `48000` |
| CHANNELS | 1 | uint8_t | `1` (Mono) or `2` (Stereo) |
| CODEC | 1 | uint8_t | `1` (Opus) or `0` (Raw PCM) |
| PAYLOAD_LEN | 2 | uint16_t | Opus payload size in bytes |
| PAYLOAD | Variable | byte[] | Opus encoded audio frame |
