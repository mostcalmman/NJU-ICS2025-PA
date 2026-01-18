#include <memory.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
  void *page_start = pf;
  pf += nr_page * PGSIZE;
  memset(page_start, 0, nr_page * PGSIZE); 
  return page_start;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  assert(n % PGSIZE == 0);
  size_t nr = n / PGSIZE;
  void* page_start =  new_page(nr);
  memset(page_start, 0, n);
  return page_start;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  extern char _stack_top;
  extern char _stack_pointer;
  Log("Physical stack ranges: [%p, %p)", &_stack_top, &_stack_pointer);
  Log("Physical heap ranges: [%p, %p)", heap.start, heap.end);
  Log("Free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
