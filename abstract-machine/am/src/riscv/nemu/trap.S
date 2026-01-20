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
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
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
#define OFFSET_SP     (2 * XLEN)
#define OFFSET_SP_N  (-((NR_REGS + 2) * XLEN)) # 从栈顶访问
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)
#define OFFSET_NP     ((NR_REGS + 3) * XLEN)

.align 3
.globl __am_asm_trap
__am_asm_trap:
  # mscratch中保存的是ksp
  # gpr[sp]代表c->sp, 也就是usp
  # 在内核态下它始终为0, 真正的ksp在sp寄存器里; 在用户态下它保存着内核栈sp(即ksp)
  # 切换到内核栈
  csrrw sp, mscratch, sp # sp <-> mscratch
  bnez sp, save_context # ksp != 0: 用户态, 切换到内核态, 直接开开始保存上下文; ksp == 0: 内核态, 执行下一条
  csrrw sp, mscratch, sp # 从mscratch恢复内核栈sp, 同时把kas的值恢复到mscratch


save_context:
  # 此时mscratch中保存着原本的sp
  # 从用户态来, mscratch = 原sp(用户栈), sp是新的内核栈sp; 从内核态来, mscratch = 0, sp上就是原sp(内核栈)
  addi sp, sp, -CONTEXT_SIZE

  MAP(REGS, PUSH)

  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc
  csrr t5, mscratch # 仅用作比较

  bnez t5, store_mscratch_to_csp
  # mscratch==0执行的
  mv t3, sp
  mv t4, zero # c-np = 0, 内核态
store_mscratch_to_csp:
  beqz t5, store_sp_to_csp
  # mscratch!=0执行的
  csrr t3, mscratch
  li t4, 0x1 # c-np = 1, 用户态
store_sp_to_csp:

  csrw mscratch, zero # ksp = 0, 标记为内核态, 允许重入

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)
  STORE t3, OFFSET_SP(sp)
  STORE t4, OFFSET_NP(sp)

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  mv a0, sp

  call __am_irq_handle

  mv sp, a0 # 恢复sp, 也就是Context *的地址

  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  LOAD t4, OFFSET_NP(sp)
  csrw mstatus, t1
  csrw mepc, t2

  beqz t4, not_store_ksp
  csrw mscratch, sp # 从用户态来的, 恢复ksp到mscratch
not_store_ksp:

  MAP(REGS, POP)

  addi sp, sp, CONTEXT_SIZE
  # 切换回原来的栈
#   LOAD sp, OFFSET_SP_N(sp)
  LOAD sp, -((NR_REGS + 2) * XLEN)(sp) # 恢复原sp
  mret
