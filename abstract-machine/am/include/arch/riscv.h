#ifndef ARCH_H__
#define ARCH_H__

#include <stdint.h>
#include <x86_64-linux-gnu/sys/types.h>
#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

struct Context {
  // TODO: fix the order of these members to match trap.S
  union {
    void *pdir; // 地址空间占用reg[0]
    uintptr_t gpr0;
  };
  uintptr_t gpr[NR_REGS - 1];
  uintptr_t mcause;
  uintptr_t mstatus;
  uintptr_t mepc;
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
