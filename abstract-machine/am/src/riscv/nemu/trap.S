#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS_LO16(f) \
      f( 1) f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)
#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

#define CONTEXT_SIZE  ((NR_REGS + 4) * XLEN)
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)
#define OFFSET_NP     ((NR_REGS + 3) * XLEN)

#define KERNAL 0
#define USER 1

.align 3
.globl __am_asm_trap
__am_asm_trap:
  # mscratch中保存的是ksp
  # gpr[sp]代表c->sp, 也就是usp
  # 在内核态下它始终为0, 真正的ksp在sp寄存器里; 在用户态下它保存着内核栈sp(即ksp)
  # 切换到内核栈
  csrrw sp, mscratch, sp # sp <-> mscratch
  bnez sp, from_user # ksp != 0: 用户态, 要切换到内核态; ksp == 0: 内核态

from_kernel:
  csrrw sp, mscratch, sp # 从mscratch恢复内核栈sp, 同时把kas的值(0)恢复到mscratch

  addi sp, sp, -CONTEXT_SIZE
  MAP(REGS, PUSH)

  # c->np = KERNEL
  li t0, KERNAL
  STORE t0, OFFSET_NP(sp)
  # c-sp = sp + CONTEXT_SIZE (原本的sp)
  addi t0, sp, CONTEXT_SIZE
  STORE t0, (2 * XLEN)(sp) # gpr[sp]代表c->sp

  j common_setup

from_user:
  # 此时 sp 已经是内核栈 (ksp), 而 mscratch 里存的是用户栈 (usp)
  addi sp, sp, -CONTEXT_SIZE
  MAP(REGS, PUSH)

  # c->np = USER
  li t0, USER
  STORE t0, OFFSET_NP(sp)
  # c-sp = mscratch (保存用户栈指针 usp)
  csrr t0, mscratch
  STORE t0, (2 * XLEN)(sp)
  # ksp = 0
  csrw mscratch, zero

common_setup:
  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  mv a0, sp
  
  call __am_irq_handle

  mv sp, a0

  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  LOAD t0, OFFSET_NP(sp)
  beqz t0, restore_regs
  # np == USER 的话要重置 ksp
  addi t1, sp, CONTEXT_SIZE # 把Context出栈后才是下一次的内核栈sp
  csrw mscratch, t1

restore_regs:
  MAP(REGS, POP)

  LOAD sp, (2 * XLEN)(sp)
  mret
