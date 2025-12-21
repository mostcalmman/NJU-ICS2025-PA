#include <NDL.h>
#include <assert.h>
#include <sdl-timer.h>
#include <stdio.h>
#include <stdbool.h>

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  assert(0);
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  assert(0);
  return 1;
}

uint32_t SDL_GetTicks() {
  uint32_t ms = NDL_GetTicks();
  return ms;
}

void SDL_Delay(uint32_t ms) {
  // MARK: CallbackHelper
  bool CallbackHelper();
  CallbackHelper();
  void SDL_RunAudio();
  uint32_t start = SDL_GetTicks();
  while (SDL_GetTicks() - start < ms) {
    SDL_RunAudio();
  }
}
