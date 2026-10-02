#ifndef ALSA_ASOUNDLIB_H
#define ALSA_ASOUNDLIB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _snd_pcm snd_pcm_t;
typedef struct _snd_pcm_hw_params snd_pcm_hw_params_t;

typedef enum _snd_pcm_stream {
    SND_PCM_STREAM_PLAYBACK = 0,
    SND_PCM_STREAM_CAPTURE
} snd_pcm_stream_t;

typedef enum _snd_pcm_format {
    SND_PCM_FORMAT_S16_LE = 2,
    SND_PCM_FORMAT_FLOAT_LE = 14
} snd_pcm_format_t;

typedef enum _snd_pcm_access {
    SND_PCM_ACCESS_RW_INTERLEAVED = 3
} snd_pcm_access_t;

typedef unsigned long snd_pcm_uframes_t;
typedef long snd_pcm_sframes_t;

int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
int snd_pcm_close(snd_pcm_t *pcm);
int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t **ptr);
void snd_pcm_hw_params_free(snd_pcm_hw_params_t *obj);
int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t access);
int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
int snd_pcm_prepare(snd_pcm_t *pcm);
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent);
const char *snd_strerror(int errnum);

#ifdef __cplusplus
}
#endif

#endif // ALSA_ASOUNDLIB_H
