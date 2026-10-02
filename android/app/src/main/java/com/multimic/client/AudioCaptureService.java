package com.multimic.client;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import androidx.core.app.NotificationCompat;

import java.util.concurrent.atomic.AtomicBoolean;

public class AudioCaptureService extends Service {
    public static final String ACTION_START = "com.multimic.client.START_CAPTURE";
    public static final String ACTION_STOP = "com.multimic.client.STOP_CAPTURE";
    public static final String EXTRA_HOST = "EXTRA_HOST";
    public static final String EXTRA_PORT = "EXTRA_PORT";
    public static final String EXTRA_DEVICE_ID = "EXTRA_DEVICE_ID";
    public static final String EXTRA_SESSION_ID = "EXTRA_SESSION_ID";
    public static final String EXTRA_BITRATE = "EXTRA_BITRATE";
    public static final String EXTRA_SAMPLE_RATE = "EXTRA_SAMPLE_RATE";

    public static final String ACTION_AUDIO_LEVEL = "com.multimic.client.AUDIO_LEVEL";
    public static final String EXTRA_RMS_LEVEL = "EXTRA_RMS_LEVEL";

    private static final String CHANNEL_ID = "MultiMicServiceChannel";
    private static final int NOTIFICATION_ID = 1001;

    private final IBinder binder = new LocalBinder();
    private final AtomicBoolean isRecording = new AtomicBoolean(false);
    private Thread captureThread;
    private NetworkClient networkClient;
    private OpusEncoderWrapper opusEncoder;

    public class LocalBinder extends Binder {
        public AudioCaptureService getService() {
            return AudioCaptureService.this;
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();
        networkClient = new NetworkClient();
        opusEncoder = new OpusEncoderWrapper();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if (ACTION_START.equals(action)) {
                String host = intent.getStringExtra(EXTRA_HOST);
                int port = intent.getIntExtra(EXTRA_PORT, 45455);
                String deviceId = intent.getStringExtra(EXTRA_DEVICE_ID);
                long sessionId = intent.getLongExtra(EXTRA_SESSION_ID, 0);
                int bitrate = intent.getIntExtra(EXTRA_BITRATE, 32000);
                int sampleRate = intent.getIntExtra(EXTRA_SAMPLE_RATE, 48000);

                startForegroundServiceWithNotification();
                startAudioCapture(host, port, deviceId, sessionId, bitrate, sampleRate);
            } else if (ACTION_STOP.equals(action)) {
                stopAudioCapture();
                stopForeground(true);
                stopSelf();
            }
        }
        return START_NOT_STICKY;
    }

    private void startForegroundServiceWithNotification() {
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                this, 0, notificationIntent,
                PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);

        Intent stopIntent = new Intent(this, AudioCaptureService.class);
        stopIntent.setAction(ACTION_STOP);
        PendingIntent stopPendingIntent = PendingIntent.getService(
                this, 0, stopIntent,
                PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);

        Notification notification = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("MultiMic Active")
                .setContentText("MultiMic microphone is active")
                .setSmallIcon(android.R.drawable.ic_btn_speak_now)
                .setContentIntent(pendingIntent)
                .setOngoing(true)
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .addAction(android.R.drawable.ic_delete, "Stop Microphone", stopPendingIntent)
                .build();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }
    }

    @SuppressLint("MissingPermission")
    private void startAudioCapture(String host, int port, String deviceId, long sessionId, int bitrate, int sampleRate) {
        if (isRecording.get()) return;

        isRecording.set(true);
        networkClient.initUdpSocket();
        opusEncoder.init(sampleRate, 1, bitrate);

        captureThread = new Thread(() -> {
            int frameSize = sampleRate / 50; // 20ms frames = 960 samples
            int minBufSize = AudioRecord.getMinBufferSize(
                    sampleRate,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT);
            int bufferSize = Math.max(minBufSize, frameSize * 2 * 4);

            AudioRecord recorder = new AudioRecord(
                    MediaRecorder.AudioSource.MIC,
                    sampleRate,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT,
                    bufferSize);

            if (recorder.getState() != AudioRecord.STATE_INITIALIZED) {
                isRecording.set(false);
                return;
            }

            recorder.startRecording();

            short[] pcmBuffer = new short[frameSize];
            byte[] encodedPayload = new byte[512];
            long seqNum = 1;

            while (isRecording.get()) {
                int read = recorder.read(pcmBuffer, 0, frameSize);
                if (read > 0) {
                    // Calculate RMS Audio Level
                    double sumSq = 0;
                    for (int i = 0; i < read; i++) {
                        sumSq += pcmBuffer[i] * pcmBuffer[i];
                    }
                    float rms = (float) Math.sqrt(sumSq / read) / 32768.0f;

                    // Broadcast audio level to UI
                    Intent levelIntent = new Intent(ACTION_AUDIO_LEVEL);
                    levelIntent.putExtra(EXTRA_RMS_LEVEL, rms);
                    sendBroadcast(levelIntent);

                    // Encode Opus / Pack PCM
                    int encodedLen = opusEncoder.encode(pcmBuffer, read, encodedPayload);
                    if (encodedLen <= 0) {
                        // Raw PCM fallback if encoder unavailable
                        encodedLen = read * 2;
                        ByteBuffer bb = ByteBuffer.allocate(encodedLen).order(ByteOrder.LITTLE_ENDIAN);
                        for (int i = 0; i < read; i++) bb.putShort(pcmBuffer[i]);
                        System.arraycopy(bb.array(), 0, encodedPayload, 0, encodedLen);
                    }

                    long timestampUs = System.currentTimeMillis() * 1000;
                    byte[] packet = Protocol.serializeAudioPacket(
                            deviceId, sessionId, seqNum++, timestampUs, sampleRate, 1, 1, encodedPayload, encodedLen);

                    networkClient.sendAudioPacket(host, port, packet);
                }
            }

            recorder.stop();
            recorder.release();
            opusEncoder.destroy();
            networkClient.close();
        }, "AudioCaptureThread");

        captureThread.start();
    }

    private void stopAudioCapture() {
        isRecording.set(false);
        if (captureThread != null) {
            try {
                captureThread.join(1000);
            } catch (InterruptedException ignored) {}
            captureThread = null;
        }
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "MultiMic Capture Channel",
                    NotificationManager.IMPORTANCE_HIGH);
            channel.setDescription("Shows persistent status notification when microphone is actively streaming");
            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onDestroy() {
        stopAudioCapture();
        super.onDestroy();
    }

    public boolean isMicrophoneActive() {
        return isRecording.get();
    }
}
