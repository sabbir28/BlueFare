package com.multimic.client;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.InputType;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends AppCompatActivity {
    private static final int PERMISSION_REQUEST_CODE = 200;

    private TextView textConnectionStatus, textDeviceName, textServerAddress, textMicStatus, textActiveIndicator;
    private ProgressBar progressAudioLevel;
    private Button btnScanConnect, btnStartMic, btnStopMic, btnSettings;

    private NetworkClient networkClient;
    private DiscoveryClient discoveryClient;

    private String serverIp = "192.168.1.20";
    private int controlPort = 45454;
    private int audioPort = 45455;
    private String deviceId = "phone01";
    private String deviceName = "My Phone";
    private long currentSessionId = 0;
    private boolean isPaired = false;

    private final BroadcastReceiver audioLevelReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (AudioCaptureService.ACTION_AUDIO_LEVEL.equals(intent.getAction())) {
                float rms = intent.getFloatExtra(AudioCaptureService.EXTRA_RMS_LEVEL, 0.0f);
                int progress = Math.min(100, (int) (rms * 300));
                progressAudioLevel.setProgress(progress);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textConnectionStatus = findViewById(R.id.textConnectionStatus);
        textDeviceName = findViewById(R.id.textDeviceName);
        textServerAddress = findViewById(R.id.textServerAddress);
        textMicStatus = findViewById(R.id.textMicStatus);
        textActiveIndicator = findViewById(R.id.textActiveIndicator);
        progressAudioLevel = findViewById(R.id.progressAudioLevel);

        btnScanConnect = findViewById(R.id.btnScanConnect);
        btnStartMic = findViewById(R.id.btnStartMic);
        btnStopMic = findViewById(R.id.btnStopMic);
        btnSettings = findViewById(R.id.btnSettings);

        networkClient = new NetworkClient();
        discoveryClient = new DiscoveryClient(this);

        loadSettings();

        btnScanConnect.setOnClickListener(v -> scanOrConnectServer());
        btnStartMic.setOnClickListener(v -> checkPermissionsAndStartMic());
        btnStopMic.setOnClickListener(v -> stopMicrophoneStream());
        btnSettings.setOnClickListener(v -> startActivity(new Intent(this, SettingsActivity.class)));

        updateUiState(false, false);
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadSettings();
        registerReceiver(audioLevelReceiver, new IntentFilter(AudioCaptureService.ACTION_AUDIO_LEVEL), Context.RECEIVER_NOT_EXPORTED);
    }

    @Override
    protected void onPause() {
        super.onPause();
        try {
            unregisterReceiver(audioLevelReceiver);
        } catch (Exception ignored) {}
    }

    private void loadSettings() {
        SharedPreferences prefs = getSharedPreferences(SettingsActivity.PREFS_NAME, MODE_PRIVATE);
        serverIp = prefs.getString(SettingsActivity.KEY_SERVER_IP, "192.168.1.20");
        controlPort = prefs.getInt(SettingsActivity.KEY_CONTROL_PORT, 45454);
        audioPort = prefs.getInt(SettingsActivity.KEY_AUDIO_PORT, 45455);
        deviceName = prefs.getString(SettingsActivity.KEY_DEVICE_NAME, Build.MODEL);
        deviceId = "phone_" + Math.abs(deviceName.hashCode());

        textDeviceName.setText(deviceName);
        textServerAddress.setText(serverIp + ":" + controlPort);
    }

    private void scanOrConnectServer() {
        Toast.makeText(this, "Scanning LAN for MultiMic Server...", Toast.LENGTH_SHORT).show();
        textConnectionStatus.setText("Scanning LAN...");

        discoveryClient.startDiscovery((serverName, ipAddress, port) -> {
            serverIp = ipAddress;
            controlPort = port;
            textServerAddress.setText(serverIp + ":" + controlPort);
            discoveryClient.stopDiscovery();
            promptPairingPin();
        });

        // Fallback after 2.5s if discovery has no immediate response: prompt directly for IP/PIN
        btnScanConnect.postDelayed(() -> {
            discoveryClient.stopDiscovery();
            if (!isPaired) promptPairingPin();
        }, 2500);
    }

    private void promptPairingPin() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("MultiMic Server Pairing");
        builder.setMessage("Enter the 6-digit Pairing Code displayed on the Linux host:");

        final EditText input = new EditText(this);
        input.setInputType(InputType.TYPE_CLASS_NUMBER);
        input.setHint("e.g. 483921");
        builder.setView(input);

        builder.setPositiveButton("Connect & Pair", (dialog, which) -> {
            String pin = input.getText().toString().trim();
            if (pin.length() == 6) {
                attemptPairing(pin);
            } else {
                Toast.makeText(this, "Please enter a valid 6-digit PIN code", Toast.LENGTH_SHORT).show();
            }
        });
        builder.setNegativeButton("Cancel", (dialog, which) -> dialog.cancel());
        builder.show();
    }

    private void attemptPairing(String pin) {
        textConnectionStatus.setText("Connecting & Pairing...");
        networkClient.pairWithServer(serverIp, controlPort, deviceId, deviceName, pin, new NetworkClient.ConnectionCallback() {
            @Override
            public void onPaired(long sessionId, int serverAudioPort) {
                currentSessionId = sessionId;
                audioPort = serverAudioPort;
                isPaired = true;
                textConnectionStatus.setText("CONNECTED (Session #" + sessionId + ")");
                updateUiState(true, false);
                Toast.makeText(MainActivity.this, "Pairing Successful!", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onError(String message) {
                isPaired = false;
                textConnectionStatus.setText("Disconnected");
                updateUiState(false, false);
                Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();
            }
        });
    }

    private void checkPermissionsAndStartMic() {
        if (!isPaired) {
            Toast.makeText(this, "Please connect and pair with Linux server first", Toast.LENGTH_SHORT).show();
            return;
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    this,
                    new String[]{Manifest.permission.RECORD_AUDIO, Manifest.permission.POST_NOTIFICATIONS},
                    PERMISSION_REQUEST_CODE);
            return;
        }

        startMicrophoneStream();
    }

    private void startMicrophoneStream() {
        SharedPreferences prefs = getSharedPreferences(SettingsActivity.PREFS_NAME, MODE_PRIVATE);
        int bitrate = prefs.getInt(SettingsActivity.KEY_BITRATE, 32000);
        int sampleRate = prefs.getInt(SettingsActivity.KEY_SAMPLE_RATE, 48000);

        Intent intent = new Intent(this, AudioCaptureService.class);
        intent.setAction(AudioCaptureService.ACTION_START);
        intent.putExtra(AudioCaptureService.EXTRA_HOST, serverIp);
        intent.putExtra(AudioCaptureService.EXTRA_PORT, audioPort);
        intent.putExtra(AudioCaptureService.EXTRA_DEVICE_ID, deviceId);
        intent.putExtra(AudioCaptureService.EXTRA_SESSION_ID, currentSessionId);
        intent.putExtra(AudioCaptureService.EXTRA_BITRATE, bitrate);
        intent.putExtra(AudioCaptureService.EXTRA_SAMPLE_RATE, sampleRate);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }

        updateUiState(true, true);
        Toast.makeText(this, "Microphone Active & Streaming", Toast.LENGTH_SHORT).show();
    }

    private void stopMicrophoneStream() {
        Intent intent = new Intent(this, AudioCaptureService.class);
        intent.setAction(AudioCaptureService.ACTION_STOP);
        startService(intent);

        updateUiState(isPaired, false);
        progressAudioLevel.setProgress(0);
        Toast.makeText(this, "Microphone Stopped", Toast.LENGTH_SHORT).show();
    }

    private void updateUiState(boolean connected, boolean streaming) {
        if (streaming) {
            textMicStatus.setText("ACTIVE");
            textMicStatus.setTextColor(0xFFFF0000); // Red
            textActiveIndicator.setVisibility(View.VISIBLE);
            btnStartMic.setEnabled(false);
            btnStopMic.setEnabled(true);
        } else {
            textMicStatus.setText("STOPPED");
            textMicStatus.setTextColor(0xFF888888); // Gray
            textActiveIndicator.setVisibility(View.GONE);
            btnStartMic.setEnabled(connected);
            btnStopMic.setEnabled(false);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                startMicrophoneStream();
            } else {
                Toast.makeText(this, "Microphone permission is required to stream audio", Toast.LENGTH_LONG).show();
            }
        }
    }
}
