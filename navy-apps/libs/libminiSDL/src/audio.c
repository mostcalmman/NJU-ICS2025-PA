#include <NDL.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 单位都是微秒
static long last_callback_time = 0;
static long interval = 0;
static SDL_AudioSpec audio_spec;
static uint8_t *audio_buf = NULL;
static uint32_t audio_len = 0;
static uint32_t audio_play_len = 0;
static int audio_paused = 1; // 默认为暂停状态
static int g_audio_cb_depth = 0;

static void update_audio_buf(){
  if (audio_spec.callback) {
      memset(audio_buf, 0, audio_len);
      // printf("audio_len: %d\n", audio_len);
      audio_play_len = NDL_QueryAudio() < audio_len ? NDL_QueryAudio() : audio_len;
      // printf("audio_play_len: %d\n", audio_play_len);
      // 进入回调, 设置标志, 防止重入
      g_audio_cb_depth ++;
      audio_spec.callback(audio_spec.userdata, audio_buf, audio_play_len);
      g_audio_cb_depth --;
    }
}

bool CallbackHelper() {
  if(g_audio_cb_depth > 0) return false;
  long now = NDL_GetTicks();
  if (now - last_callback_time >= interval) {
    last_callback_time = now;
    update_audio_buf();
    return 1;
  }
  return 0;
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  NDL_OpenAudio(desired->freq, desired->channels, desired->samples);
  interval = desired->samples * 1000000 / desired->freq;
  last_callback_time = NDL_GetTicks();

  audio_spec = *desired;
  int format_byte = desired->format == AUDIO_U8 ? 1 : 2;
  audio_len = desired->samples * format_byte * desired->channels;
  audio_buf = (uint8_t *)malloc(audio_len);
  audio_paused = 1;
  return 0;
}

void SDL_CloseAudio() {
  if (audio_buf) free(audio_buf);
  audio_len = 0;
  audio_paused = 1;
  NDL_CloseAudio();
}

void SDL_PauseAudio(int pause_on) {
  audio_paused = pause_on;
  if (!pause_on) {
    last_callback_time = NDL_GetTicks() - interval;
  }
}

// 非标准API，在SDL_Delay中被调用以驱动音频播放
void SDL_RunAudio() {
  if (audio_paused) return;
  if (CallbackHelper()) {
    NDL_PlayAudio(audio_buf, audio_play_len);
  }
}

void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {

  if (dst == NULL || src == NULL || len == 0) return;

  if (volume < 0) volume = 0;
  if (volume > SDL_MIX_MAXVOLUME) volume = SDL_MIX_MAXVOLUME;

  // miniSDL 只实现了 AUDIO_U8 / AUDIO_S16(AUDIO_S16SYS)
  if (audio_spec.format == AUDIO_U8) {
    for (uint32_t i = 0; i < len; i++) {
      int32_t dst_s = (int32_t)dst[i] - 128;
      int32_t src_s = (int32_t)src[i] - 128;
      src_s = (src_s * volume) / SDL_MIX_MAXVOLUME;
      int32_t out = dst_s + src_s;
      if (out > 127) out = 127;
      if (out < -128) out = -128;
      dst[i] = (uint8_t)(out + 128);
    }
    return;
  }

  // default: AUDIO_S16
  uint32_t nsamples = len / 2;
  int16_t *dst16 = (int16_t *)dst;
  int16_t *src16 = (int16_t *)src;
  for (uint32_t i = 0; i < nsamples; i++) {
    int32_t dst_s = (int32_t)dst16[i];
    int32_t src_s = (int32_t)src16[i];
    src_s = (src_s * volume) / SDL_MIX_MAXVOLUME;
    int32_t out = dst_s + src_s;
    if (out > 32767) out = 32767;
    if (out < -32768) out = -32768;
    dst16[i] = (int16_t)out;
  }
}

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  if (file == NULL || spec == NULL || audio_buf == NULL || audio_len == NULL) return NULL;

  *audio_buf = NULL;
  *audio_len = 0;

  FILE *fp = fopen(file, "rb");
  if (fp == NULL) return NULL;

  uint8_t riff[4];
  uint8_t wave[4];
  uint32_t riff_size = 0;

  if (fread(riff, 1, 4, fp) != 4) goto fail;
  if (fread(&riff_size, 4, 1, fp) != 1) goto fail;
  if (fread(wave, 1, 4, fp) != 4) goto fail;

  if (memcmp(riff, "RIFF", 4) != 0) goto fail;
  if (memcmp(wave, "WAVE", 4) != 0) goto fail;

  // fmt chunk
  int have_fmt = 0;
  uint16_t audio_format = 0;
  uint16_t num_channels = 0;
  uint32_t sample_rate = 0;
  uint16_t bits_per_sample = 0;

  // data chunk
  int have_data = 0;
  uint32_t data_size = 0;
  long data_offset = 0;

  // walk through chunks
  while (!have_data) {
    uint8_t chunk_id[4];
    uint32_t chunk_size = 0;
    if (fread(chunk_id, 1, 4, fp) != 4) break;
    if (fread(&chunk_size, 4, 1, fp) != 1) break;

    if (memcmp(chunk_id, "fmt ", 4) == 0) {
      // PCM fmt chunk is at least 16 bytes
      if (chunk_size < 16) goto fail;
      uint32_t byte_rate = 0;
      uint16_t block_align = 0;
      if (fread(&audio_format, 2, 1, fp) != 1) goto fail;
      if (fread(&num_channels, 2, 1, fp) != 1) goto fail;
      if (fread(&sample_rate, 4, 1, fp) != 1) goto fail;
      if (fread(&byte_rate, 4, 1, fp) != 1) goto fail;
      if (fread(&block_align, 2, 1, fp) != 1) goto fail;
      if (fread(&bits_per_sample, 2, 1, fp) != 1) goto fail;

      // skip remaining fmt bytes (e.g. cbSize)
      uint32_t remain = chunk_size - 16;
      if (remain > 0) {
        if (fseek(fp, remain, SEEK_CUR) != 0) goto fail;
      }

      have_fmt = 1;
    } else if (memcmp(chunk_id, "data", 4) == 0) {
      data_offset = ftell(fp);
      data_size = chunk_size;
      have_data = 1;
      // don't seek now; we'll read it after validation
      if (fseek(fp, chunk_size, SEEK_CUR) != 0) goto fail;
    } else {
      // skip unknown chunk
      if (fseek(fp, chunk_size, SEEK_CUR) != 0) goto fail;
    }

    // chunks are word-aligned
    if (chunk_size & 1) {
      if (fseek(fp, 1, SEEK_CUR) != 0) goto fail;
    }
  }

  if (!have_fmt || !have_data) goto fail;

  // Only support uncompressed PCM
  if (audio_format != 1) goto fail;
  if (!(bits_per_sample == 8 || bits_per_sample == 16)) goto fail;
  if (!(num_channels == 1 || num_channels == 2)) goto fail;
  if (sample_rate == 0) goto fail;

  uint8_t *buf = (uint8_t *)malloc(data_size);
  if (buf == NULL) goto fail;

  if (fseek(fp, data_offset, SEEK_SET) != 0) {
    free(buf);
    goto fail;
  }
  if (data_size > 0 && fread(buf, 1, data_size, fp) != data_size) {
    free(buf);
    goto fail;
  }

  // Fill SDL_AudioSpec
  memset(spec, 0, sizeof(*spec));
  spec->freq = (int)sample_rate;
  spec->channels = (uint8_t)num_channels;
  spec->format = (bits_per_sample == 8) ? AUDIO_U8 : AUDIO_S16SYS;
  // WAV 只提供数据；samples/callback/userdata 由调用者后续设置
  spec->samples = 4096;
  spec->size = data_size;
  spec->callback = NULL;
  spec->userdata = NULL;

  *audio_buf = buf;
  *audio_len = data_size;
  fclose(fp);
  (void)riff_size;
  return spec;

fail:
  fclose(fp);
  return NULL;
}

void SDL_FreeWAV(uint8_t *audio_buf) {

  if (audio_buf) free(audio_buf);
}

void SDL_LockAudio() {
}

void SDL_UnlockAudio() {
}
