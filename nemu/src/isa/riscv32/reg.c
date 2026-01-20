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

#include <isa.h>
#include <string.h>
// #include "local-include/reg.h"

const char *regs[] = {
  // 这里改过, 第一个原本是$0
  "0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  printf("------Register State Table------\nreg             dec\t     hex\n");
  printf("%-5s %13d %12x\n", "pc", cpu.pc, cpu.pc);
  printf("%-5s %13d %12x\n", "$0", cpu.gpr[0], cpu.gpr[0]);
  for (int i = 1; i < 32; i++) {
    printf("%-5s %13d %12x\n", regs[i], cpu.gpr[i], cpu.gpr[i]);
  }
  printf("----Register State Table End----\n");
}

word_t isa_reg_str2val(const char *s, bool *success) {
  for(int i = 0; i < 32; i++) {
    if(strcmp(s+1, regs[i]) == 0) {
      return cpu.gpr[i];
    }
  }
  if(strcmp(s+1, "pc") == 0) {
    return cpu.pc;
  }
  if(strcmp(s+1, "mepc") == 0) {
    return cpu.mepc;
  }
  if(strcmp(s+1, "mstatus") == 0) {
    return cpu.mstatus;
  }
  if(strcmp(s+1, "mcause") == 0) {
    return cpu.mcause;
  }
  *success = false;
  return 0;
}

// 对照表在指令集规范第二册
word_t* isa_csr_str2ptr(word_t csr) {
  switch(csr) {
    case 0x341: return &cpu.mepc;
    case 0x300: return &cpu.mstatus;
    case 0x342: return &cpu.mcause;
    case 0x305: return &cpu.mtvec;
    case 0x180: return &cpu.satp;
    case 0x340: return &cpu.mscratch;
    default: panic("Unsupported CSR address: 0x%x", csr);
  }
}