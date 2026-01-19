#include <common.h>
void do_syscall(Context *c);
Context* schedule(Context *prev);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: {
      // uintptr_t *old_satp = c->pdir;
      c = schedule(c);
      // Log("Yield, pdir change from %p to %p", (void*)old_satp, c->pdir);
      break;
    }
    case EVENT_SYSCALL: 
      do_syscall(c); 
      break;
    case EVENT_IRQ_TIMER:
      break; // 暂时什么都不做
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
