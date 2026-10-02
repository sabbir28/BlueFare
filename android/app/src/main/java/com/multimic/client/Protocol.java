package com.multimic.client;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;

public class Protocol {
    public static final int UDP_AUDIO_MAGIC = 0x4D4D4155; // "MMAU"
    public static final long TCP_HEADER_MAGIC = 0x4D554C54494D4943L; // "MULTIMIC"
    public static final byte PROTOCOL_VERSION = 1;
    public static final int AUDIO_HEADER_SIZE = 50;

    public static byte[] serializeAudioPacket(
            String deviceId,
            long sessionId,
            long sequenceNum,
            long timestampUs,
            int sampleRate,
            int channels,
            int codec,
            byte[] payload,
            int payloadLen) {

        ByteBuffer buffer = ByteBuffer.allocate(AUDIO_HEADER_SIZE + payloadLen);
        buffer.order(ByteOrder.BIG_ENDIAN);

        buffer.putInt(UDP_AUDIO_MAGIC);
        buffer.put(PROTOCOL_VERSION);
        buffer.put((byte) 1); // AUDIO_DATA type

        byte[] devIdBytes = new byte[16];
        byte[] srcBytes = deviceId.getBytes(StandardCharsets.UTF_8);
        System.arraycopy(srcBytes, 0, devIdBytes, 0, Math.min(srcBytes.length, 15));
        buffer.put(devIdBytes);

        buffer.putInt((int) sessionId);
        buffer.putLong(sequenceNum);
        buffer.putLong(timestampUs);
        buffer.putInt(sampleRate);
        buffer.put((byte) channels);
        buffer.put((byte) codec);
        buffer.putShort((short) payloadLen);

        if (payload != null && payloadLen > 0) {
            buffer.put(payload, 0, payloadLen);
        }

        return buffer.array();
    }

    public static byte[] serializeTcpMessage(byte type, String jsonPayload) {
        byte[] jsonBytes = jsonPayload != null ? jsonPayload.getBytes(StandardCharsets.UTF_8) : new byte[0];
        ByteBuffer buffer = ByteBuffer.allocate(14 + jsonBytes.length);
        buffer.order(ByteOrder.BIG_ENDIAN);

        buffer.putLong(TCP_HEADER_MAGIC);
        buffer.put(PROTOCOL_VERSION);
        buffer.put(type);
        buffer.putInt(jsonBytes.length);
        if (jsonBytes.length > 0) {
            buffer.put(jsonBytes);
        }

        return buffer.array();
    }
}
