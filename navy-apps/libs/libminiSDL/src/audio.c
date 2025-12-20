#include <NDL.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

// 单位都是微秒
static long last_callback_time = 0;
static long interval = 0;
static SDL_AudioSpec audio_spec;
static uint8_t *audio_buf = NULL;
static uint32_t audio_len = 0;
static uint32_t audio_play_len = 0;
static int audio_paused = 1; // 默认为暂停状态

static void update_audio_buf(){
  if (audio_spec.callback) {
      memset(audio_buf, 0, audio_len);
      audio_play_len = NDL_QueryAudio() < audio_len ? NDL_QueryAudio() : audio_len;
      audio_spec.callback(audio_spec.userdata, audio_buf, audio_play_len);
    }
}

void CallbackHelper() {
  long now = NDL_GetTicks();
  if (now - last_callback_time >= interval) {
    last_callback_time = now;
    update_audio_buf();
  }
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
    last_callback_time = NDL_GetTicks();
    update_audio_buf();
  }
}

// 非标准API，在SDL_Delay中被调用以驱动音频播放
void SDL_RunAudio() {
  if (audio_paused) return;
  CallbackHelper();
  NDL_PlayAudio(audio_buf, audio_play_len);
}

void SDL_MixAudio(uint8_t *dst, uint8_t *src, uint32_t len, int volume) {
}

SDL_AudioSpec *SDL_LoadWAV(const char *file, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len) {
  return NULL;
}

void SDL_FreeWAV(uint8_t *audio_buf) {
}

void SDL_LockAudio() {
}

void SDL_UnlockAudio() {
}
