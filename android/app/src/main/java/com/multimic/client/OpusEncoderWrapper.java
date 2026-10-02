package com.multimic.client;

public class OpusEncoderWrapper {
    static {
        try {
            System.loadLibrary("multimic_native");
        } catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
        }
    }

    private long nativeHandle = 0;

    public boolean init(int sampleRate, int channels, int bitrate) {
        try {
            nativeHandle = nativeInit(sampleRate, channels, bitrate);
            return nativeHandle != 0;
        } catch (UnsatisfiedLinkError e) {
            return false;
        }
    }

    public int encode(short[] pcmData, int frameSize, byte[] outPayload) {
        if (nativeHandle == 0) return 0;
        try {
            return nativeEncode(nativeHandle, pcmData, frameSize, outPayload);
        } catch (UnsatisfiedLinkError e) {
            return 0;
        }
    }

    public void destroy() {
        if (nativeHandle != 0) {
            try {
                nativeDestroy(nativeHandle);
            } catch (UnsatisfiedLinkError ignored) {}
            nativeHandle = 0;
        }
    }

    private native long nativeInit(int sampleRate, int channels, int bitrate);
    private native int nativeEncode(long handle, short[] pcmData, int frameSize, byte[] outPayload);
    private native void nativeDestroy(long handle);
}
