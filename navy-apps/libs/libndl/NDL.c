#include "sys/time.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;

static struct timeval t_ori;
static long t_us;

static struct Canvas {
  int w, h, loc_x, loc_y;
} canvas;

uint32_t NDL_GetTicks() {
  struct timeval now;
  gettimeofday(&now, NULL);
  long now_us = now.tv_sec * 1000000 + now.tv_usec;
  return now_us - t_us;
}

int NDL_PollEvent(char *buf, int len) {
  return read(evtdev, buf, len);
}

void NDL_OpenCanvas(int *w, int *h) {
  int fd = open("/proc/dispinfo", 0, 0);
  char buf[64];
  int nread = read(fd, buf, sizeof(buf) - 1);
  buf[nread] = '\0';
  sscanf(buf, "WIDTH : %d\nHEIGHT : %d\n", &screen_w, &screen_h);
  close(fd);
  if(*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }
  if (*w > screen_w) *w = screen_w;
  if (*h > screen_h) *h = screen_h;
  canvas.w = *w;
  canvas.h = *h;
  canvas.loc_x = (screen_w - canvas.w) / 2;
  canvas.loc_y = (screen_h - canvas.h) / 2;

  // if (getenv("NWM_APP")) {
  //   int fbctl = 4;
  //   fbdev = 5;
  //   screen_w = *w; screen_h = *h;
  //   char buf[64];
  //   int len = sprintf(buf, "%d %d", screen_w, screen_h);
  //   // let NWM resize the window and create the frame buffer
  //   write(fbctl, buf, len);
  //   while (1) {
  //     // 3 = evtdev
  //     int nread = read(3, buf, sizeof(buf) - 1);
  //     if (nread <= 0) continue;
  //     buf[nread] = '\0';
  //     if (strcmp(buf, "mmap ok") == 0) break;
  //   }
  //   close(fbctl);
  // }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  int screen_loc_x = canvas.loc_x + x;
  int screen_loc_y = canvas.loc_y + y;
  size_t offset = screen_w * 4 * screen_loc_y + screen_loc_x * 4;
  
  if (w == screen_w) {
    lseek(fbdev, offset, SEEK_SET);
    write(fbdev, pixels, w * h * 4);
  } else {
    for (int i = 0; i < h; i ++) {
      lseek(fbdev, offset + i * screen_w * 4, SEEK_SET);
      write(fbdev, pixels + i * w, w * 4);
    }
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  } else {
    evtdev = open("/dev/events", 0, 0);
  }
  fbdev = open("/dev/fb", 0, 0);

  gettimeofday(&t_ori, NULL);
  t_us = t_ori.tv_sec * 1000000 + t_ori.tv_usec;
  return 0;
}

void NDL_Quit() {
  close(evtdev);
  close(fbdev);
}
