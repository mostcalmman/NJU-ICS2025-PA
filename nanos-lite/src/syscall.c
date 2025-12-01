#include <common.h>
#include <stdio.h>
#include "syscall.h"
void do_syscall(Context *c) {
  Log("\nStrace: syscall event at pc = 0x%x", c->mepc);
  Log("Strace: Context");
  uintptr_t *raw = (uintptr_t *)c;
  for (int i = 0; i < 35; i++) printf("0x%x\n", raw[i]);
  Log("Context End\n");
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_yield:
      yield();
      c->mepc+=4;
      break;
    case SYS_exit:
      halt(0);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
