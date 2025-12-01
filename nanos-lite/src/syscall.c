#include <common.h>
#include <stdio.h>
#include "syscall.h"
void do_syscall(Context *c) {
  Log("Strace: syscall event at pc = 0x%x", c->mepc);
  uintptr_t a[4];
  const char *syscall_name = "UNSUPPORTED";
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_yield: syscall_name = "SYS_yield"; break;
    case SYS_exit: syscall_name = "SYS_exit"; break;
  }

  Log("Strace: syscall type = %s (id = %d)", syscall_name, a[0]);
  Log("Strace: Context");
  uintptr_t *raw = (uintptr_t *)c;
  for (int i = 0; i < 35; i++) printf("0x%x\n", raw[i]);
  Log("Context End");
  switch (a[0]) {
    case SYS_yield:
      yield();
      c->mepc += 4;
      break;
    case SYS_exit:
      halt(0);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
