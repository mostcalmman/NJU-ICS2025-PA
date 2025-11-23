/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <SDL2/SDL_audio.h>
#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <sys/types.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;
static uint32_t play_pos = 0;
static uint32_t count = 0;

static void audio_callback(void *userdata, Uint8 * stream, int len){
  uint32_t to_play = len;
  if(len > audio_base[reg_count]){
    to_play = audio_base[reg_count];
    memset(stream + to_play, 0, len - to_play);
  }
  for(uint32_t i = 0; i < to_play; i ++){
    stream[i] = sbuf[(play_pos + i) % audio_base[reg_sbuf_size]];
  }
  play_pos = (play_pos + to_play) % audio_base[reg_sbuf_size];
  audio_base[reg_count] -= to_play;
  count = audio_base[reg_count];
}

static void init_SDL_audio_subsystem(){
  SDL_AudioSpec s = {};
  s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
  s.userdata = NULL;        // 不使用
  s.freq = audio_base[reg_freq];
  s.channels = audio_base[reg_channels];
  s.samples = audio_base[reg_samples];
  s.callback = audio_callback;

  SDL_InitSubSystem(SDL_INIT_AUDIO);
  SDL_OpenAudio(&s, NULL);
  SDL_PauseAudio(0);
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if(!is_write) return; // 读只会读状态, 没有需要处理的
  int reg_id = offset / 4;
  assert(offset % 4 == 0 && reg_id >= 0 && reg_id < nr_reg);
  if(reg_id == reg_init && audio_base[reg_init] == 1){
    init_SDL_audio_subsystem();
  }
  if(reg_id == reg_count){
    SDL_LockAudio();
    count += audio_base[reg_count];
    audio_base[reg_count] = count;
    SDL_UnlockAudio();
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
  audio_base[reg_init] = 0;
  audio_base[reg_count] = 0;
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
  count = 0;
}
