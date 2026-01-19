#include "proc.h"
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
  uintptr_t old_brk = current->max_brk;
  uintptr_t page_aligned_brk = ROUNDUP(old_brk, PGSIZE);
  Log("mm_brk called: old_brk=%p, brk=%p, page_aligned_brk=%p", (void*)old_brk, (void*)brk, (void*)page_aligned_brk);
  if(brk <= page_aligned_brk) {
    current->max_brk = brk;
    Log("skipped");
    return 0;
  }
  int nr_page = (brk - page_aligned_brk) % PGSIZE == 0 ? 
                (brk - page_aligned_brk) / PGSIZE : 
                (brk - page_aligned_brk) / PGSIZE + 1;
  void *new_pg = new_page(nr_page);
  for(int i = 0; i < nr_page; i ++) {
    // void *new_pg = new_page(1);
    map(&current->as, (void*)(page_aligned_brk + i * PGSIZE), new_pg + i * PGSIZE, 14); // R W X
  }
  current->max_brk = brk;
  Log("brk from %p to %p", (void*)old_brk, (void*)brk);
  Log("free P_page from %p", new_pg + nr_page * PGSIZE);
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
