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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <cpu/iringbuf.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include "../monitor/sdb/sdb.h"
#include "common.h"
#include "macro.h"
#include <ftrace.h>
// void paddr_write(paddr_t addr, int len, word_t data); // 测试用

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10

CPU_state cpu = {
#ifdef CONFIG_IRINGBUF
  .iringbuf = {0},
  .iringbuf_index = 0,
#endif
};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

#ifdef CONFIG_FTRACE
  // _this->logbuf[24]开始是反汇编助记符
  const char* get_function_name(vaddr_t addr);
  char ftracebuf[128];
  char *ptr = ftracebuf;
  FuncStack fstack = {.func_number=0};
  if(memcmp(_this->logbuf + 24, "jal", 3) == 0){
    memcpy(ptr, _this->logbuf, 12); // 复制 "0x8000000c: "
    ptr+=12;
    for(int i = 0; i < fstack.func_number; i++){
      *ptr++ = '\t';
    }
    sprintf(ptr, "call [%s@" FMT_WORD "]\n", get_function_name(dnpc), dnpc);
    fstackPush(dnpc, &fstack);
  }else if(memcmp(_this->logbuf + 24, "ret", 3) == 0){
    vaddr_t addr = fstackPop(&fstack);
    memcpy(ptr, _this->logbuf, 12); // 复制 "0x8000000c: "
    ptr+=12;
    for(int i = 0; i < fstack.func_number; i++){
      *ptr++ = '\t';
    }
    sprintf(ptr, "ret  [%s]\n", get_function_name(addr));
  }
  puts(ftracebuf);
#endif

#ifdef CONFIG_WATCHPOINT
  WP *p = watchpoint_head;
  bool success = true;
  while(p != NULL){
    word_t new_val = expr(p->expr, &success);
    if(!success){
      printf("Invalid watchpoint expression: %s\n; Something must be wrong!\n", p->expr);
      success = true; // reset success for next watchpoint
      p = p->next;
      continue;
    }
    if(new_val != p->last_value){
      printf("Watchpoint %d: %s\n", p->NO, p->expr);
      printf("Old value = 0x%x\n", p->last_value);
      printf("New value = 0x%x\n", new_val);
      nemu_state.state = NEMU_STOP;
      p->last_value = new_val;
      // 这里不break，允许多个watchpoint同时触发
    }
    p = p->next;
  }
#endif
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
  // 由于ITRACE在实际执行后, 所以Log中记录的内存读写对应的是下一条指令
// ITRACE
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  // 目标, 大小, 内容 ...
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
#ifdef CONFIG_ISA_x86
  for (i = 0; i < ilen; i ++) {
#else
  for (i = ilen - 1; i >= 0; i --) {
#endif
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);
#endif

  // IRINGBUF
  IFDEF(CONFIG_IRINGBUF, writeIringbuf(s));

}

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    // paddr_write(0x80000000, 4, cpu.pc + 1); // 测试用
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  // if n < 10, this will print the assembly code of each instruction.
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT: case NEMU_QUIT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
