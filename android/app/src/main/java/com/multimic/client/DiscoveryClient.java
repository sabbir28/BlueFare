package com.multimic.client;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Looper;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class DiscoveryClient {
    public interface DiscoveryCallback {
        void onServerDiscovered(String serverName, String ipAddress, int controlPort);
    }

    private final Context context;
    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private boolean scanning = false;
    private DatagramSocket socket;

    public DiscoveryClient(Context context) {
        this.context = context.getApplicationContext();
    }

    public void startDiscovery(DiscoveryCallback callback) {
        if (scanning) return;
        scanning = true;

        executor.execute(() -> {
            WifiManager.MulticastLock lock = null;
            try {
                WifiManager wm = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
                if (wm != null) {
                    lock = wm.createMulticastLock("MultiMicDiscovery");
                    lock.acquire();
                }

                socket = new DatagramSocket(45456);
                socket.setBroadcast(true);
                socket.setSoTimeout(4000);

                byte[] buffer = new byte[1024];
                while (scanning && !socket.isClosed()) {
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                    socket.receive(packet);

                    String text = new String(packet.getData(), 0, packet.getLength(), StandardCharsets.UTF_8);
                    InetAddress address = packet.getAddress();
                    String ip = address != null ? address.getHostAddress() : "";

                    if (text.contains("MultiMic server available")) {
                        String name = "MultiMic-Linux";
                        int port = 45454;

                        int namePos = text.indexOf("\"name\":\"");
                        if (namePos != -1) {
                            int end = text.indexOf("\"", namePos + 8);
                            if (end != -1) name = text.substring(namePos + 8, end);
                        }

                        int portPos = text.indexOf("\"control_port\":");
                        if (portPos != -1) {
                            int end = text.indexOf("}", portPos + 15);
                            if (end == -1) end = text.indexOf(",", portPos + 15);
                            if (end != -1) {
                                try {
                                    port = Integer.parseInt(text.substring(portPos + 15, end).trim());
                                } catch (NumberFormatException ignored) {}
                            }
                        }

                        final String finalName = name;
                        final String finalIp = ip;
                        final int finalPort = port;
                        mainHandler.post(() -> callback.onServerDiscovered(finalName, finalIp, finalPort));
                    }
                }
            } catch (Exception e) {
                // Timeout or stopped
            } finally {
                if (lock != null && lock.isHeld()) {
                    lock.release();
                }
                stopDiscovery();
            }
        });
    }

    public void stopDiscovery() {
        scanning = false;
        if (socket != null && !socket.isClosed()) {
            socket.close();
            socket = null;
        }
    }
}
