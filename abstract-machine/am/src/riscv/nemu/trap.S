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
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)
#define OFFSET_NP     ((NR_REGS + 3) * XLEN)

.align 3
.globl __am_asm_trap
__am_asm_trap:
  # 1. 栈切换逻辑
  # 如果从用户态(User)进入: mscratch=ksp, sp=usp. 交换后 sp=ksp, mscratch=usp (sp!=0)
  # 如果从内核态(Kernel)进入: mscratch=0, sp=ksp. 交换后 sp=0, mscratch=ksp (sp==0)
  csrrw sp, mscratch, sp
  bnez sp, save_context   # sp!=0, 说明是从用户态来的，现在的sp已经是ksp了，直接去保存
  csrrw sp, mscratch, sp  # sp==0, 说明是从内核态来的，换回去，现在的sp是ksp

save_context:
  # 此时 sp 必定指向内核栈（无论是切换过来的还是原本就在的）
  addi sp, sp, -CONTEXT_SIZE

  MAP(REGS, PUSH)

  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  # 2. 确定 c->sp 和 c->np
  csrr t5, mscratch # 读取原来的 mscratch 值
  bnez t5, from_user

from_kernel:
  # mscratch 为 0，说明是从内核态进入
  addi t3, sp, CONTEXT_SIZE # 原始 sp 是分配 context 之前的 sp
  li t4, 0                  # np = 0 (KERNEL)
  j store_context

from_user:
  # mscratch 不为 0，说明是从用户态进入，t5 存储的就是 User SP
  mv t3, t5                 # 原始 sp 就是 mscratch 的值
  li t4, 1                  # np = 1 (USER)

store_context:
  # 此时: t0=cause, t1=status, t2=epc, t3=sp, t4=np
  # 重要: 进入内核处理前，必须把 mscratch 置 0，保证内核态的可重入性检测正确
  csrw mscratch, zero

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)
  STORE t3, OFFSET_SP(sp)   # 将计算好的正确 SP 保存到 Context
  STORE t4, OFFSET_NP(sp)

  # 设置 mstatus.MPRV (PA特定需求，通常用于 difftest)
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  mv a0, sp
  call __am_irq_handle
  
  # 3. 恢复上下文
  mv sp, a0 # 切换到新上下文的栈地址 (可能是调度后的新进程)

  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  LOAD t4, OFFSET_NP(sp)

  csrw mstatus, t1
  csrw mepc, t2

  # 4. 准备返回时的栈指针 (mscratch 设置)
  beqz t4, restore_regs    # 如果返回内核态 (np=0)，跳过 mscratch 设置(保持为0)

  # 如果返回用户态 (np=1):
  # 我们需要把内核栈的栈底(Top)放入 mscratch，供下次 trap 使用
  addi t5, sp, CONTEXT_SIZE
  csrw mscratch, t5

restore_regs:
  MAP(REGS, POP)
  
  # 5. 最后的栈切换
  # 此时 sp 依然指向 Context 结构体
  # REGS 宏中没有包含 x2(sp)，所以 POP 不会修改 sp 寄存器
  # 我们直接从 Context 中加载保存的 sp 值 (可能是 User SP，也可能是 Kernel SP)
  LOAD sp, OFFSET_SP(sp)
  
  mret