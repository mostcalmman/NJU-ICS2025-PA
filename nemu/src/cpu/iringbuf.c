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
  char disasm_buf[64];
  printf("======= IRINGBUF =======\n");
  for(int i = 0; i < 20; i++){
    disassemble(disasm_buf, 64, cpu.iringbuf_pc[cpu.iringbuf_index % 20 - 1], (uint8_t *)&cpu.iringbuf[cpu.iringbuf_index % 20 - 1], 4);
    if(i == cpu.iringbuf_index % 20 - 1) {
      printf("-->");
    } else {
      printf("   ");
    }
    printf("%08x: %08x  %s\n", cpu.iringbuf_pc[i], cpu.iringbuf[i], disasm_buf);
  }
#endif
}