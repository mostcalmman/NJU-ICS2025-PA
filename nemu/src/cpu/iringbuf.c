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
  cpu.iringbuf_index++;
#endif
}

void printIringbuf(){
    for(int i = 0; i < 20; i++){
#ifdef CONFIG_IRINGBUF
      int index = (cpu.iringbuf_index + i) % 20;
      printf("0x%08x\n", cpu.iringbuf[index]);
#endif
    }
}