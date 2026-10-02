#ifndef OPUS_H
#define OPUS_H

#include "opus_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPUS_OK                      0
#define OPUS_BAD_ARG                -1
#define OPUS_BUFFER_TOO_SMALL       -2
#define OPUS_INTERNAL_ERROR         -3
#define OPUS_INVALID_PACKET         -4
#define OPUS_UNIMPLEMENTED          -5
#define OPUS_INVALID_STATE          -6
#define OPUS_ALLOC_FAIL             -7

#define OPUS_APPLICATION_VOIP              2048
#define OPUS_APPLICATION_AUDIO             2049
#define OPUS_APPLICATION_RESTRICTED_LOWDELAY 2051

#define OPUS_AUTO                           -1000
#define OPUS_BITRATE_MAX                    -1

#define OPUS_SET_BITRATE_REQUEST            4002
#define OPUS_GET_BITRATE_REQUEST            4003
#define OPUS_SET_COMPLEXITY_REQUEST         4010
#define OPUS_SET_INBAND_FEC_REQUEST         4012
#define OPUS_SET_PACKET_LOSS_PERC_REQUEST   4014
#define OPUS_SET_SIGNAL_REQUEST             4024

#define OPUS_SET_BITRATE(x)                 OPUS_SET_BITRATE_REQUEST, (opus_int32)(x)
#define OPUS_SET_COMPLEXITY(x)              OPUS_SET_COMPLEXITY_REQUEST, (opus_int32)(x)
#define OPUS_SET_INBAND_FEC(x)              OPUS_SET_INBAND_FEC_REQUEST, (opus_int32)(x)
#define OPUS_SET_PACKET_LOSS_PERC(x)        OPUS_SET_PACKET_LOSS_PERC_REQUEST, (opus_int32)(x)

typedef struct OpusEncoder OpusEncoder;
typedef struct OpusDecoder OpusDecoder;

OpusEncoder* opus_encoder_create(opus_int32 clock_rate, int channels, int application, int *error);
int opus_encoder_init(OpusEncoder *st, opus_int32 clock_rate, int channels, int application);
opus_int32 opus_encode(OpusEncoder *st, const opus_int16 *pcm, int frame_size, unsigned char *data, opus_int32 max_data_bytes);
opus_int32 opus_encode_float(OpusEncoder *st, const float *pcm, int frame_size, unsigned char *data, opus_int32 max_data_bytes);
void opus_encoder_destroy(OpusEncoder *st);
int opus_encoder_ctl(OpusEncoder *st, int request, ...);

OpusDecoder* opus_decoder_create(opus_int32 clock_rate, int channels, int *error);
int opus_decoder_init(OpusDecoder *st, opus_int32 clock_rate, int channels);
int opus_decode(OpusDecoder *st, const unsigned char *data, opus_int32 len, opus_int16 *pcm, int frame_size, int decode_fec);
int opus_decode_float(OpusDecoder *st, const unsigned char *data, opus_int32 len, float *pcm, int frame_size, int decode_fec);
void opus_decoder_destroy(OpusDecoder *st);
int opus_decoder_ctl(OpusDecoder *st, int request, ...);

const char* opus_strerror(int error);
const char* opus_get_version_string(void);

#ifdef __cplusplus
}
#endif

#endif // OPUS_H
