package com.multimic.client;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

public class SettingsActivity extends AppCompatActivity {
    public static final String PREFS_NAME = "MultiMicPrefs";
    public static final String KEY_SERVER_IP = "KEY_SERVER_IP";
    public static final String KEY_CONTROL_PORT = "KEY_CONTROL_PORT";
    public static final String KEY_AUDIO_PORT = "KEY_AUDIO_PORT";
    public static final String KEY_DEVICE_NAME = "KEY_DEVICE_NAME";
    public static final String KEY_BITRATE = "KEY_BITRATE";
    public static final String KEY_SAMPLE_RATE = "KEY_SAMPLE_RATE";

    private EditText editServerIp, editControlPort, editAudioPort, editDeviceName;
    private Spinner spinnerBitrate, spinnerSampleRate;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        if (getSupportActionBar() != null) {
            getSupportActionBar().setTitle("Settings");
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }

        editServerIp = findViewById(R.id.editServerIp);
        editControlPort = findViewById(R.id.editControlPort);
        editAudioPort = findViewById(R.id.editAudioPort);
        editDeviceName = findViewById(R.id.editDeviceName);
        spinnerBitrate = findViewById(R.id.spinnerBitrate);
        spinnerSampleRate = findViewById(R.id.spinnerSampleRate);
        Button btnSave = findViewById(R.id.btnSaveSettings);

        // Populate Bitrate Spinner
        String[] bitrates = {"16 kbps", "24 kbps", "32 kbps (Default)", "48 kbps", "64 kbps"};
        ArrayAdapter<String> adapterBitrate = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, bitrates);
        adapterBitrate.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerBitrate.setAdapter(adapterBitrate);

        // Populate Sample Rate Spinner
        String[] sampleRates = {"48000 Hz (Default)", "44100 Hz"};
        ArrayAdapter<String> adapterRates = new ArrayAdapter<>(this, android.R.layout.simple_spinner_item, sampleRates);
        adapterRates.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinnerSampleRate.setAdapter(adapterRates);

        loadPreferences();

        btnSave.setOnClickListener(v -> {
            savePreferences();
            Toast.makeText(this, "Settings Saved", Toast.LENGTH_SHORT).show();
            finish();
        });
    }

    private void loadPreferences() {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        editServerIp.setText(prefs.getString(KEY_SERVER_IP, "192.168.1.20"));
        editControlPort.setText(String.valueOf(prefs.getInt(KEY_CONTROL_PORT, 45454)));
        editAudioPort.setText(String.valueOf(prefs.getInt(KEY_AUDIO_PORT, 45455)));
        editDeviceName.setText(prefs.getString(KEY_DEVICE_NAME, android.os.Build.MODEL));

        int bitrate = prefs.getInt(KEY_BITRATE, 32000);
        if (bitrate == 16000) spinnerBitrate.setSelection(0);
        else if (bitrate == 24000) spinnerBitrate.setSelection(1);
        else if (bitrate == 32000) spinnerBitrate.setSelection(2);
        else if (bitrate == 48000) spinnerBitrate.setSelection(3);
        else if (bitrate == 64000) spinnerBitrate.setSelection(4);
    }

    private void savePreferences() {
        SharedPreferences.Editor editor = getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit();
        editor.putString(KEY_SERVER_IP, editServerIp.getText().toString().trim());
        editor.putInt(KEY_CONTROL_PORT, Integer.parseInt(editControlPort.getText().toString().trim()));
        editor.putInt(KEY_AUDIO_PORT, Integer.parseInt(editAudioPort.getText().toString().trim()));
        editor.putString(KEY_DEVICE_NAME, editDeviceName.getText().toString().trim());

        int selectedBitrate = 32000;
        switch (spinnerBitrate.getSelectedItemPosition()) {
            case 0: selectedBitrate = 16000; break;
            case 1: selectedBitrate = 24000; break;
            case 2: selectedBitrate = 32000; break;
            case 3: selectedBitrate = 48000; break;
            case 4: selectedBitrate = 64000; break;
        }
        editor.putInt(KEY_BITRATE, selectedBitrate);
        editor.putInt(KEY_SAMPLE_RATE, 48000);

        editor.apply();
    }

    @Override
    public boolean onSupportNavigateUp() {
        finish();
        return true;
    }
}
