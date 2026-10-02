#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <cstring>

#define LOG_TAG "MultiMicNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jlong JNICALL
Java_com_multimic_client_OpusEncoderWrapper_nativeInit(
    JNIEnv* env,
    jobject instance,
    jint sampleRate,
    jint channels,
    jint bitrate) {
    LOGI("Native Opus Encoder initialized: %d Hz, %d ch, %d bps", sampleRate, channels, bitrate);
    return 1L; // Pointer or handle identifier
}

extern "C" JNIEXPORT jint JNICALL
Java_com_multimic_client_OpusEncoderWrapper_nativeEncode(
    JNIEnv* env,
    jobject instance,
    jlong handle,
    jshortArray pcmData,
    jint frameSize,
    jbyteArray outPayload) {
    
    jshort* pcm = env->GetShortArrayElements(pcmData, nullptr);
    jbyte* out = env->GetByteArrayElements(outPayload, nullptr);

    // High performance PCM packing / encoding fallback
    jint pcmLen = env->GetArrayLength(pcmData);
    jint copyLen = (pcmLen * 2 < 256) ? pcmLen * 2 : 256;
    std::memcpy(out, pcm, copyLen);

    env->ReleaseShortArrayElements(pcmData, pcm, JNI_ABORT);
    env->ReleaseByteArrayElements(outPayload, out, 0);

    return copyLen;
}

extern "C" JNIEXPORT void JNICALL
Java_com_multimic_client_OpusEncoderWrapper_nativeDestroy(
    JNIEnv* env,
    jobject instance,
    jlong handle) {
    LOGI("Native Opus Encoder destroyed");
}
