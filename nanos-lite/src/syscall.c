#include <common.h>
#include "syscall.h"
#include "am.h"
#include <fs.h>

// #define CONFIG_STRACE

#ifdef CONFIG_STRACE
static const char *syscall_names[] = {
  "SYS_exit",
  "SYS_yield",
  "SYS_open",
  "SYS_read",
  "SYS_write",
  "SYS_kill",
  "SYS_getpid",
  "SYS_close",
  "SYS_lseek",
  "SYS_brk",
  "SYS_fstat",
  "SYS_time",
  "SYS_signal",
  "SYS_execve",
  "SYS_fork",
  "SYS_link",
  "SYS_unlink",
  "SYS_wait",
  "SYS_times",
  "SYS_gettimeofday"
};

static void strace(Context *c){
  Log("Strace: syscall event at pc = 0x%x, type = %s (id = %d), arg list: %d, %d, %d", c->mepc, syscall_names[c->GPR1], c->GPR1, c->GPR2, c->GPR3, c->GPR4);
  if (c->GPR1 == SYS_write || c->GPR1 == SYS_read || c->GPR1 == SYS_close || c->GPR1 == SYS_lseek) {
    Log("Strace: file operation on file %s (fd = %d)", fs_getname(c->GPR2), c->GPR2);
  }
  // Log("Strace: Context");
  // uintptr_t *raw = (uintptr_t *)c;
  // for (int i = 0; i < 35; i++) printf("0x%x\n", raw[i]);
  // Log("Strace: Context End");
}
#endif

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
#ifdef CONFIG_STRACE
  strace(c);
#endif
  switch (a[0]) {
    case SYS_yield:
      yield();
      break;
    case SYS_exit:
      // MARK: halt
      if( a[1]!=0 ){
        Log("Program exited with code %d", a[1]);
      }
      halt(a[1]);
      // halt(0);
      break;
    case SYS_brk: {
      c->GPRx = 0;
      break;
    }
    case SYS_open: {
      c->GPRx = fs_open((const char *)a[1], (int)a[2], (int)a[3]);
#ifdef CONFIG_STRACE
      Log("Strace: open file %s (fd = %d)", (const char *)a[1], c->GPRx);
#endif
      break;
    }
    case SYS_read: {
      c->GPRx = fs_read((int)a[1], (void *)a[2], (size_t)a[3]);
      break;
    }
    case SYS_write: {
      // uintptr_t ret = 0;
      // if(a[1]==1||a[1]==2){
      //   for(int i=0;i<a[3];i++){
      //     putch(((char*)a[2])[i]);
      //     ++ret;
      //   }
      // }
      // c->GPRx = ret;
      // break;
      c->GPRx = fs_write((int)a[1], (const void *)a[2], (size_t)a[3]);
      break;
    }
    case SYS_lseek: {
      c->GPRx = fs_lseek((int)a[1], (size_t)a[2], (int)a[3]);
      break;
    }
    case SYS_close: {
      c->GPRx = fs_close((int)a[1]);
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
