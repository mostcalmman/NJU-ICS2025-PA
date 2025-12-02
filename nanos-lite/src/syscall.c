#include <common.h>
#include <stdio.h>
#include "syscall.h"
#include "am.h"

void strace(Context *c){
  Log("Strace: syscall event at pc = 0x%x", c->mepc);
  const char *syscall_name = "UNSUPPORTED";
  switch (c->GPR1) {
    case SYS_yield: syscall_name = "SYS_yield"; break;
    case SYS_exit: syscall_name = "SYS_exit"; break;
  }

  Log("Strace: syscall type = %s (id = %d)", syscall_name, c->GPR1);
  Log("Strace: Context");
  uintptr_t *raw = (uintptr_t *)c;
  for (int i = 0; i < 35; i++) printf("0x%x\n", raw[i]);
  Log("Strace: Context End");
}


void do_syscall(Context *c) {
  strace(c);
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  switch (a[0]) {
    case SYS_yield:
      yield();
      c->mepc += 4;
      break;
    case SYS_exit:
      halt(a[1]);
      break;
    case SYS_write: {
      uintptr_t ret = 0;
      if(a[1]==1||a[1]==2){
        for(int i=0;i<a[3];i++){
          putch(((char*)a[2])[i]);
          ++ret;
        }
      }
      c->GPRx = ret;
      c->mepc += 4;
      break;
    }
    case SYS_read:
      assert(0);
      c->mepc += 4;
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
