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
#include "utils.h"
#include <isa.h>

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.mepc = epc;
  cpu.mcause = NO;

#ifdef CONFIG_ETRACE
  Log("Exception NO: %d at pc = 0x%x, mstatus: %x, mtvec: %x", NO, epc, cpu.mstatus, cpu.mtvec);
  log_write("\n\n\nLSR\n\n");
  log_write("Exception NO: " FMT_WORD " at pc = 0x" FMT_PADDR ", mstatus: " FMT_WORD ", mtvec: " FMT_WORD, NO, epc, cpu.mstatus, cpu.mtvec);
  extern bool log_enable();
  if (log_enable()) Log("LSRLSR");
#endif

  return cpu.mtvec;
}
word_t isa_query_intr() {
  return INTR_EMPTY;
}
