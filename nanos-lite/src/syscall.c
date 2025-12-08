#include <common.h>
#include "syscall.h"
#include "am.h"
#include <fs.h>

#define CONFIG_STRACE

void strace(Context *c){
  const char *syscall_name = "UNSUPPORTED";
  switch (c->GPR1) {
    case SYS_yield: syscall_name = "SYS_yield"; break;
    case SYS_exit: syscall_name = "SYS_exit"; break;
    case SYS_write: syscall_name = "SYS_write"; break;
    case SYS_read: syscall_name = "SYS_read"; break;
    case SYS_brk: syscall_name = "SYS_brk"; break;
    case SYS_open: syscall_name = "SYS_open"; break;
    case SYS_close: syscall_name = "SYS_close"; break;
    case SYS_lseek: syscall_name = "SYS_lseek"; break;
  }
  Log("Strace: syscall event at pc = 0x%x, type = %s (id = %d), arg list: %d, %d, %d", c->mepc, syscall_name, c->GPR1, c->GPR2, c->GPR3, c->GPR4);
  if (c->GPR1 == SYS_write || c->GPR1 == SYS_read || c->GPR1 == SYS_close || c->GPR1 == SYS_lseek) {
    Log("Strace: file operation on file %s (fd = %d)", fs_getname(c->GPR2), c->GPR2);
  }
  // Log("Strace: Context");
  // uintptr_t *raw = (uintptr_t *)c;
  // for (int i = 0; i < 35; i++) printf("0x%x\n", raw[i]);
  // Log("Strace: Context End");
}


void do_syscall(Context *c) {
#ifdef CONFIG_STRACE
  strace(c);
#endif
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
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
