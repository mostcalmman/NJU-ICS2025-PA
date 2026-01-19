/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "common.h"
#include <isa.h>

#define IRQ_TIMER 0x80000007  // for riscv32

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.mepc = epc;
  cpu.mcause = NO;

  // 让处理器进入关中断
  bool MIE = (cpu.mstatus >> 3) & 0x1;
  cpu.mstatus = cpu.mstatus & (~(1 << 3)); // MIE = 0
  cpu.mstatus = cpu.mstatus & (~(1 << 7)); // MPIE = 0
  cpu.mstatus = cpu.mstatus | (MIE << 7); // MPIE = old MIE

#ifdef CONFIG_ETRACE
  Log("Exception NO: %d at pc = 0x%x, mstatus: %x, mtvec: %x", NO, epc, cpu.mstatus, cpu.mtvec);
  // _log_write("Exception NO: " FMT_WORD " at pc = 0x" FMT_PADDR ", mstatus: " FMT_WORD ", mtvec: " FMT_WORD, NO, epc, cpu.mstatus, cpu.mtvec);
#endif

  return cpu.mtvec;
}

word_t isa_query_intr() {
  // 只实现时钟中断, 所以中断引脚高电平就是时钟中断
  if (cpu.INTR) {
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
