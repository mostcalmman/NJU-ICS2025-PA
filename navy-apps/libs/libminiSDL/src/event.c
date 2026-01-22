#include <NDL.h>
#include <SDL.h>
#include <assert.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

static uint8_t keystate[sizeof(keyname) / sizeof(keyname[0])];

int SDL_PushEvent(SDL_Event *ev) {
  assert(0);
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  // MARK: CallbackHelper
  // bool CallbackHelper();
  // void SDL_RunAudio();
  // CallbackHelper();
  char buf[64];
  // SDL_RunAudio();
  if (NDL_PollEvent(buf, sizeof(buf))) {
    // printf("%s\n", buf);
    if (buf[0] == 'k' && (buf[1] == 'd' || buf[1] == 'u')) {
      // keyboard event
      ev->type = (buf[1] == 'd') ? SDL_KEYDOWN : SDL_KEYUP;
      // parse key name
      char mykeyname[32];
      sscanf(buf + 3, "%s", mykeyname);
      // map to SDLK_
      ev->key.keysym.sym = SDLK_NONE;
      for (int i = 1; i < sizeof(keyname)/sizeof(keyname[0]); i++) {
        if (strcmp(mykeyname, keyname[i]) == 0) {
          ev->key.keysym.sym = i;
          
          keystate[i] = (ev->type == SDL_KEYDOWN) ? 1 : 0;
          break;
        }
      }
      return 1;
    }
  }
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  while(SDL_PollEvent(event) == 0);
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  assert(0);
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  // MARK: CallbackHelper
  // bool CallbackHelper();
  // CallbackHelper();
  if (numkeys) *numkeys = sizeof(keyname) / sizeof(keyname[0]);
  return keystate;
}
