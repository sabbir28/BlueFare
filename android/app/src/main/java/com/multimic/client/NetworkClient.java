package com.multimic.client;

import android.os.Handler;
import android.os.Looper;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NetworkClient {
    public interface ConnectionCallback {
        void onPaired(long sessionId, int audioPort);
        void onError(String message);
    }

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private DatagramSocket udpSocket;

    public void pairWithServer(String host, int controlPort, String deviceId, String deviceName, String pin, ConnectionCallback callback) {
        executor.execute(() -> {
            try (Socket socket = new Socket(host, controlPort)) {
                socket.setSoTimeout(5000);
                OutputStream out = socket.getOutputStream();
                InputStream in = socket.getInputStream();

                String json = "{\"device_id\":\"" + deviceId + "\",\"name\":\"" + deviceName + "\",\"pin\":\"" + pin + "\"}";
                byte[] packet = Protocol.serializeTcpMessage((byte) 0x03, json); // PAIR_REQUEST
                out.write(packet);
                out.flush();

                byte[] headerBuf = new byte[14];
                int read = in.read(headerBuf);
                if (read >= 14) {
                    ByteBuffer bb = ByteBuffer.wrap(headerBuf).order(ByteOrder.BIG_ENDIAN);
                    long magic = bb.getLong();
                    byte ver = bb.get();
                    byte type = bb.get();
                    int payloadLen = bb.getInt();

                    if (magic == Protocol.TCP_HEADER_MAGIC && payloadLen > 0) {
                        byte[] payloadBuf = new byte[payloadLen];
                        in.read(payloadBuf);
                        String respJson = new String(payloadBuf);

                        if (respJson.contains("\"status\":\"OK\"")) {
                            long sessionId = 0;
                            int audioPort = 45455;

                            int sessPos = respJson.indexOf("\"session_id\":");
                            if (sessPos != -1) {
                                int end = respJson.indexOf(",", sessPos + 13);
                                if (end == -1) end = respJson.indexOf("}", sessPos + 13);
                                if (end != -1) sessionId = Long.parseLong(respJson.substring(sessPos + 13, end).trim());
                            }

                            int portPos = respJson.indexOf("\"audio_port\":");
                            if (portPos != -1) {
                                int end = respJson.indexOf("}", portPos + 13);
                                if (end != -1) audioPort = Integer.parseInt(respJson.substring(portPos + 13, end).trim());
                            }

                            final long finalSess = sessionId;
                            final int finalPort = audioPort;
                            mainHandler.post(() -> callback.onPaired(finalSess, finalPort));
                            return;
                        } else {
                            mainHandler.post(() -> callback.onError("Pairing Rejected: Invalid PIN code"));
                            return;
                        }
                    }
                }
                mainHandler.post(() -> callback.onError("Invalid server response"));
            } catch (Exception e) {
                mainHandler.post(() -> callback.onError("Connection failed: " + e.getMessage()));
            }
        });
    }

    public boolean initUdpSocket() {
        try {
            if (udpSocket == null || udpSocket.isClosed()) {
                udpSocket = new DatagramSocket();
            }
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public void sendAudioPacket(String host, int audioPort, byte[] packetData) {
        if (udpSocket == null || udpSocket.isClosed()) return;
        try {
            InetAddress addr = InetAddress.getByName(host);
            DatagramPacket packet = new DatagramPacket(packetData, packetData.length, addr, audioPort);
            udpSocket.send(packet);
        } catch (Exception ignored) {}
    }

    public void close() {
        if (udpSocket != null && !udpSocket.isClosed()) {
            udpSocket.close();
            udpSocket = null;
        }
    }
}
