#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  while(1) {
    char buf[64];
    if (NDL_PollEvent(buf, sizeof(buf))) {
      printf("%s\n", buf);
      if (buf[0] == 'k' && (buf[1] == 'd' || buf[1] == 'u')) {
        // keyboard event
        event->type = (buf[1] == 'd') ? SDL_KEYDOWN : SDL_KEYUP;
        // parse key name
        char mykeyname[32];
        sscanf(buf + 3, "%s", mykeyname);
        // map to SDLK_
        event->key.keysym.sym = SDLK_NONE;
        for (int i = 1; i < sizeof(keyname)/sizeof(keyname[0]); i++) {
          if (strcmp(mykeyname, keyname[i]) == 0) {
            event->key.keysym.sym = i;
            break;
          }
        }
        return 1;
      }
    }
  }
  return 1;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
