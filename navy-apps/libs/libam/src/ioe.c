#include "sys/time.h"
#include <am.h>

bool ioe_init() {
  return true;
}

static void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) { // id = 6, 成员 uint64_t us
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uptime->us = tv.tv_sec * 1000000 + tv.tv_usec;
}

static void __am_timer_config(AM_TIMER_CONFIG_T *cfg) { cfg->present = true; cfg->has_rtc = true; }
static void __am_input_config(AM_INPUT_CONFIG_T *cfg) { cfg->present = true;  }

void ioe_read (int reg, void *buf) {
  switch (reg) {
    case AM_TIMER_UPTIME: __am_timer_uptime((AM_TIMER_UPTIME_T *)buf); return;
    case AM_TIMER_CONFIG: __am_timer_config((AM_TIMER_CONFIG_T *)buf); return;
    case AM_INPUT_CONFIG: __am_input_config((AM_INPUT_CONFIG_T *)buf); return;
    default:
      printf("am on os: ioe_read: unhandled reg = %d\n", reg); 
      break;
  }
}
void ioe_write(int reg, void *buf) { }
