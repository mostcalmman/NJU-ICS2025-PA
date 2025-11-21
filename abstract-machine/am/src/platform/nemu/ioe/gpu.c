#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  int i;
  int w = 400;  // TODO: get the correct width
  int h = 300;  // TODO: get the correct height
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);
}

// bool present, has_accel; int width, height, vmemsz
void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->has_accel = false;
  uint32_t width = inl(VGACTL_ADDR) >> 16;
  uint32_t height = inl(VGACTL_ADDR) & 0xffff;
  cfg->width = width;
  cfg->height = height;
  cfg->vmemsz = width * height * sizeof(uint32_t);
}

// int x, y; void *pixels; int w, h; bool sync
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t width = inl(VGACTL_ADDR) >> 16;
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
  uint32_t *start = (uint32_t *)FB_ADDR + ctl->y * width + ctl->x;
  uint32_t *pixels = (uint32_t *)ctl->pixels;
  for (int i = 0; i < ctl->h; i ++) {
    for (int j = 0; j < ctl->w; j ++) {
      start[i * width + j] = pixels[i * ctl->w + j];
    }
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
