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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_R, TYPE_J,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 30, 21) << 1) | (BITS(i, 20, 20) << 11) | (BITS(i, 19, 12) << 12); } while(0) // LSR

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_J:                   immJ(); break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  // str: 指令格式, ?是立即数等; name: 指令名, 仅为阅读方便, 无用; type: 指令类型; ...: 指令具体的执行语句
  /*
  这里列一些模板方便复制
  R---------funct7--rs2---rs1-funct3-rd---opcode
  INSTPAT("xxxxxxx ????? ????? xxx ????? xxxxxxx", , R, );

  S----------imm----rs2--rs1-funct3-imm---opcode
  INSTPAT("??????? ????? ????? xxx ????? xxxxxxx", , S, );

  I-----------imm--------rs1-funct3-rd----opcode
  INSTPAT("????????????  ????? xxx ????? xxxxxxx", , I, );

  U---------------imm---------------rd----opcode
  INSTPAT("????????????????????    ????? xxxxxxx", , U, );

  J--------------imm----------------rd----opcode
  INSTPAT("????????????????????    ????? xxxxxxx", , J, );

  */
  
  // 整数运算
  INSTPAT("0000000 ????? ????? 000 ????? 0110011", add    , R, R(rd) = src1 + src2);
  INSTPAT("????????????  ????? 000 ????? 0010011", addi   , I, R(rd) = src1 + imm);
  INSTPAT("????????????????????    ????? 0010111", auipc  , U, R(rd) = s->pc + imm);

  // load and store
  INSTPAT("????????????  ????? 100 ????? 0000011", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 0100011", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 0100011", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 0100011", sw     , S, Mw(src1 + imm, 4, src2));

  // 跳转
  INSTPAT("????????????????????    ????? 1101111", jal    , J, R(rd) = s->pc + 4; s->dnpc = s->pc + imm);
  INSTPAT("????????????  ????? 000 ????? 1100111", jalr   , I, R(rd) = s->pc + 4; s->dnpc = src1 + imm);

  // 其他

  // 结束程序, 这个必须倒数第二
  INSTPAT("0000000 00001 00000 000 00000 1110011", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  // 无效指令, 这个必须在最后
  INSTPAT("??????? ????? ????? ??? ????? ???????", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

// inst的类型是uint32_t
int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
