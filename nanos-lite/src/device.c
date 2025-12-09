#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  while(*((char *)buf) != '\0' && len -- > 0) {
    putch(*(char *)buf);
    buf ++;
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T kbd = io_read(AM_INPUT_KEYBRD);
  int keycode = kbd.keycode;
  if(keycode == AM_KEY_NONE) return 0;
  char tem[32];
  bool down  = kbd.keydown;
  const char *key = keyname[keycode];
  if (down) {
    memcpy(tem, "kd ", 3);
  } else {
    memcpy(tem, "ku ", 3);
  }
  strcpy(tem + 3, key);
  memcpy(tem + 3 + strlen(key), "\n\0", 2);
  size_t read_len = strlen(tem) < len ? strlen(tem) : len; // exclude '\0'
  // size_t read_len = strlen(tem) + 1 < len ? strlen(tem) + 1 : len; // include '\0'
  memcpy(buf, tem, read_len);
  return read_len;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T display = io_read(AM_GPU_CONFIG);
  assert(display.present);
  char tem[64];
  int w = display.width;
  int h = display.height;
  int n = sprintf(tem, "WIDTH : %d\nHEIGHT : %d\n", w, h);
  size_t read_len = n < len ? n : len;
  memcpy(buf, tem + offset, read_len);
  return read_len;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
