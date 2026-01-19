#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

#define IRQ_TIMER 0x80000007

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  // printf("\nContext\n");
  // uintptr_t *raw = (uintptr_t *)c;
  // for (int i = 0; i < 35; i++) printf("0x%x\n", raw[i]);
  // printf("Context End\n\n");
  __am_get_cur_as(c);
  assert(user_handler != NULL);
  Event ev = {0};
  switch (c->mcause) {
    case 11: 
      c->mepc += 4;
      if(c->GPR1 == -1) {
        ev.event = EVENT_YIELD;
      }else {
        ev.event = EVENT_SYSCALL;
      }
      break;
    case 12:
    case 13:
    case 15: 
      ev.event = EVENT_PAGEFAULT; break;
    case IRQ_TIMER:
      // c->mepc += 4;
      ev.event = EVENT_IRQ_TIMER; break;
    default: 
      ev.event = EVENT_ERROR; break;
  }

  c = user_handler(ev, c);
  assert(c != NULL);
  // printf("MARK");
  // if(c->pdir == NULL) printf("aaa\n");
  __am_switch(c); // 切换到新的地址空间
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  printf("MARK\n");
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *c = (Context *)kstack.end - 1;
  c->mepc = (uintptr_t)entry;
  c->mstatus = 0x1888; // PA 中用不到特权级, 但是设为 0x1800 可通过diffTest; 在此基础上, MIE 和 MPIE 设为 1
  c->GPR2 = (uintptr_t)arg;
  c->gpr[2] = (uintptr_t)c;
  c->pdir = NULL;
  return c;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
