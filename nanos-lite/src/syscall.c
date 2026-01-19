#include <common.h>
#include "syscall.h"
#include "am.h"
#include "proc.h"
#include <fs.h>

// struct timeval {
// 	long		tv_sec;		/* seconds */
// 	long		tv_usec;	/* and microseconds */
// };

extern PCB *current;
void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb();
int mm_brk(uintptr_t brk);

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
  if (c->GPR1 == SYS_write || c->GPR1 == SYS_read || c->GPR1 == SYS_close || c->GPR1 == SYS_lseek) {
    if ( c->GPR2 == 3 || c->GPR2 == 4) return; // 忽略对 /dev/events 和 /dev/fb 的读写
  }
  if (c->GPR1 == SYS_gettimeofday) return; // 忽略 gettimeofday 调用
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
      panic("Yield syscall should be handled in irq.c!");
      yield();
      break;
    case SYS_exit:
      if( a[1]!=0 ){
        Log("Error: Program exited with code %d", a[1]);
      }
      Log("Halting...");
      halt(a[1]);
      panic("Shoud not reach here!");
      context_uload(current, "/bin/nterm", NULL, NULL);
      switch_boot_pcb();
      yield();
      break;
    case SYS_brk: {
      c->GPRx = mm_brk((uintptr_t)a[1]);
      Log("Max brk: %p\n", (void*)current->max_brk);
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
      c->GPRx = fs_lseek((int)a[1], (off_t)a[2], (int)a[3]);
      break;
    }
    case SYS_close: {
      c->GPRx = fs_close((int)a[1]);
      break;
    }
    case SYS_gettimeofday: {
      if(a[2] != 0){
        panic("timezone is not supported");
      }
      struct timeval *tv = (struct timeval *)a[1];
      if (tv != NULL) {
        uint64_t uptime = io_read(AM_TIMER_UPTIME).us;
        tv->tv_sec = uptime / 1000000;
        tv->tv_usec = uptime % 1000000;
      }
      c->GPRx = 0;
      break;
    }
    // 让新进程接管旧进程, PCB覆盖, 但需要新的用户栈(因为执行系统调用是在旧进程的用户栈上的)
    case SYS_execve: {
      int ret = fs_open((const char*)a[1], 0, 0);
      if (ret < 0) {
        c->GPRx = ret;
        break;
      }
      Log("Execve: loading new program %s", (const char *)a[1]);
      context_uload(current, (const char *)a[1], (char* const*)a[2], (char* const*)a[3]);
      switch_boot_pcb(); // 切换current到boot pcb, 下一次yield就会切回0号进程
      yield();
      break;
    }
    case SYS_getpid: {
      c->GPRx = 1; // 返回个假的pid
      break;
    }
    case SYS_kill: {
      c->GPRx = 0; // 假装成功了
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
