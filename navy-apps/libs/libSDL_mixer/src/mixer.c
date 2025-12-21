#include <SDL_mixer.h>
#include <SDL.h>
#include <stdlib.h>


// General

int Mix_OpenAudio(int frequency, uint16_t format, int channels, int chunksize) {
  SDL_AudioSpec spec;
  spec.freq = frequency;
  spec.format = format;
  spec.channels = channels;
  spec.samples = chunksize;
  spec.callback = NULL; // 如果不使用回调，设为 NULL
  spec.userdata = NULL;
  
  if (SDL_OpenAudio(&spec, NULL) < 0) {
    return -1;
  }
  return 0;
}

void Mix_CloseAudio() {
}

char *Mix_GetError() {
  return "";
}

int Mix_QuerySpec(int *frequency, uint16_t *format, int *channels) {
  return 0;
}

// Samples

Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc) {
  // 注意：src 在这里通常是文件名字符串，但在标准 SDL 中是 RWops
  // 在 Navy 的简化实现中，我们假设 Mix_LoadWAV 传入的是文件名
  // 但 Mix_LoadWAV 宏定义通常会展开为 Mix_LoadWAV_RW
  // 这里需要根据你的头文件定义来适配。
  // 如果 Mix_LoadWAV(file) 展开为 Mix_LoadWAV_RW(SDL_RWFromFile(file, "rb"), 1)
  // 那么你需要先实现 SDL_RWFromFile。
  
  // 为了简单，我们可以假设 Mix_LoadWAV 直接调用 SDL_LoadWAV
  // 或者你可以修改 Mix_LoadWAV 的宏定义直接调用一个自定义函数。
  
  // 这里假设 src 实际上无法直接使用，我们需要修改 Mix_LoadWAV 的实现
  // 或者在 mixer.c 中实现一个 Mix_LoadWAV 函数（非宏）
  return NULL; 
}

// 更直接的方式是实现 Mix_LoadWAV (覆盖宏定义或作为函数)
Mix_Chunk *Mix_LoadWAV(const char *file) {
  Mix_Chunk *chunk = malloc(sizeof(Mix_Chunk));
  SDL_AudioSpec spec;
  uint8_t *buf = NULL;
  uint32_t len = 0;

  if (SDL_LoadWAV(file, &spec, &buf, &len) == NULL) {
    free(chunk);
    return NULL;
  }

  chunk->allocated = 1;
  chunk->abuf = buf;
  chunk->alen = len;
  chunk->volume = MIX_MAX_VOLUME;
  return chunk;
}

void Mix_FreeChunk(Mix_Chunk *chunk) {
}


// Channels

int Mix_AllocateChannels(int numchans) {
  return 0;
}

int Mix_Volume(int channel, int volume) {
  return 0;
}

int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops) {
  // 这是一个非常简化的实现：直接播放，不支持混合
  // 真正的 Mixer 需要在后台混合多个通道
  if (chunk != NULL) {
    SDL_AudioSpec spec;
    spec.freq = 44100; // 假设默认频率
    spec.channels = 2;
    spec.samples = 4096;
    // 这里应该使用 SDL_MixAudio 混合到设备缓冲区
    // 但由于 libminiSDL 的限制，我们可能需要直接调用 NDL 或者 SDL_RunAudio
    
    // 关键：libminiSDL 的 SDL_MixAudio 只是内存操作，不负责播放。
    // 你需要将 chunk->abuf 的内容混合到 SDL 内部的音频缓冲区中。
    // 这通常需要修改 libminiSDL 暴露当前的音频缓冲区，或者通过回调。
    
    // 简单 Hack：直接调用 NDL_PlayAudio 播放（不支持并发）
    // NDL_PlayAudio(chunk->abuf, chunk->alen); 
    
    // 正确做法：
    // 1. 维护一个全局的活动通道列表。
    // 2. 在 SDL_OpenAudio 注册的回调函数中，遍历活动通道。
    // 3. 使用 SDL_MixAudio 将活动通道的数据混合到回调的 stream 中。
  }
  return 0;
}

void Mix_Pause(int channel) {
}

void Mix_ChannelFinished(void (*channel_finished)(int channel)) {
}

// Music

Mix_Music *Mix_LoadMUS(const char *file) {
  return NULL;
}

Mix_Music *Mix_LoadMUS_RW(SDL_RWops *src) {
  return NULL;
}

void Mix_FreeMusic(Mix_Music *music) {
}

int Mix_PlayMusic(Mix_Music *music, int loops) {
  return 0;
}

int Mix_SetMusicPosition(double position) {
  return 0;
}

int Mix_VolumeMusic(int volume) {
  return 0;
}

int Mix_SetMusicCMD(const char *command) {
  return 0;
}

int Mix_HaltMusic() {
  return 0;
}

void Mix_HookMusicFinished(void (*music_finished)()) {
}

int Mix_PlayingMusic() {
  return 0;
}
