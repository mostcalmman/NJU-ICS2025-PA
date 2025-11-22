#include <am.h>
#include <nemu.h>
#include <stdint.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

static uintptr_t sbuf_pos = 0;

void __am_audio_init() {
}

// bool present; int bufsize
void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

// int freq, channels, samples
void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1);
}

// int count
void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

// Area buf
void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  uint32_t len = ctl->buf.end - ctl->buf.start;
  uint8_t *data = (uint8_t *)ctl->buf.start;
  uint32_t sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
  while (len > 0) {
    uint32_t count = inl(AUDIO_COUNT_ADDR);
    uint32_t free_space = sbuf_size - count;
    if (free_space == 0) continue;
    uint32_t to_write = (len < free_space) ? len : free_space;
    uint32_t space_to_end = sbuf_size - sbuf_pos;
    if (to_write <= space_to_end) {
      for (uint32_t i = 0; i < to_write; i ++) {
        outl(AUDIO_SBUF_ADDR + sbuf_pos + i, data[i]);
      }
      sbuf_pos = sbuf_pos + to_write;
    } else {
      for (uint32_t i = 0; i < space_to_end; i ++) {
        outl(AUDIO_SBUF_ADDR + sbuf_pos + i, data[i]);
      }
      for (uint32_t i = 0; i < to_write - space_to_end; i ++) {
        outl(AUDIO_SBUF_ADDR + i, data[space_to_end + i]);
      }
      sbuf_pos = (sbuf_pos + to_write) % sbuf_size;
    }
    data += to_write;
    len -= to_write;
  }
}
