#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <string.h>
#include "../monitor/sdb/sdb.h"

extern CPU_state cpu;

void writeIringbuf(Decode *s){
#ifdef CONFIG_IRINGBUF
  cpu.iringbuf_index = cpu.iringbuf_index % 20;
  cpu.iringbuf[cpu.iringbuf_index] = s->isa.inst;
  cpu.iringbuf_pc[cpu.iringbuf_index] = s->pc;
  cpu.iringbuf_index++;
#endif
}

void printIringbuf(){
#ifdef CONFIG_IRINGBUF
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  printf("======= IRINGBUF =======\n");
  for(int i = 0; i < 20; i++){
    if(i == cpu.iringbuf_index % 20 - 1) {
      printf("-->");
    } else {
      printf("   ");
    }
    printf("%08x: %08x\n", cpu.iringbuf_pc[i], cpu.iringbuf[i]);
  }
  printf("====== TEST ======\n");
  char disasm_buf[128];
  disassemble(disasm_buf, 4, cpu.iringbuf_pc[cpu.iringbuf_index % 20 - 1], (uint8_t *)&cpu.iringbuf[cpu.iringbuf_index % 20 - 1], 4);
  printf("%s\n", disasm_buf);
#endif
}