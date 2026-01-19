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
  // yield();
  for (size_t i = 0; i < len; i++) {
    putch(((char *)buf)[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  // yield();
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
  // size_t read_len = strlen(tem) < len ? strlen(tem) : len; // exclude '\0'
  size_t read_len = strlen(tem) + 1 < len ? strlen(tem) + 1 : len; // include '\0'
  memcpy(buf, tem, read_len);
  return read_len - 1; // exclude '\0' in return value
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T display = io_read(AM_GPU_CONFIG);
  assert(display.present);
  char tem[64];
  int w = display.width;
  int h = display.height;
  int n = snprintf(tem, sizeof(tem), "WIDTH : %d\nHEIGHT : %d\n", w, h);
  if (len > n) len = n;
  memcpy(buf, tem + offset, len);
  return len;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  // yield();
  AM_GPU_CONFIG_T display = io_read(AM_GPU_CONFIG);
  assert(display.present);
  int screen_w = display.width;
  int x = (offset / 4) % screen_w;
  int y = (offset / 4) / screen_w;
  io_write(AM_GPU_FBDRAW, x, y, (void *)buf, len / 4, 1, true);
  return len;
}

size_t sb_write(const void *buf, size_t offset, size_t len) {
  Area sbuf;
  sbuf.start = (void *)buf;
  sbuf.end = (void *)buf + len;
  io_write(AM_AUDIO_PLAY, sbuf);
  return len;
}

size_t sbctl_read(void *buf, size_t offset, size_t len) {
  assert(len >= 4);
  int count = io_read(AM_AUDIO_STATUS).count;
  int size = io_read(AM_AUDIO_CONFIG).bufsize;
  ((int *)buf)[0] = size - count;
  return 4;
}

size_t sbctl_write(const void *buf, size_t offset, size_t len) {
  assert(len >= 12);
  io_write(AM_AUDIO_CTRL, ((const int *)buf)[0], ((const int *)buf)[1], ((const int *)buf)[2]);
  return 12;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
