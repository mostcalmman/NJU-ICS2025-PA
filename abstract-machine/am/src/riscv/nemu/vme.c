#include <am.h>
#include <nemu.h>
#include <klib.h>

#define PDX(va) (((uintptr_t)(va) >> 22) & 0x3ff) // VPN[1]
#define PTX(va) (((uintptr_t)(va) >> 12) & 0x3ff) // VPN[0]
#define PTE_ADDR(pte) (((uintptr_t)(pte) & ~0xfff)) // 低12位置0

static AddrSpace kas = {}; // Kernel address space, 负责管理虚拟内核空间
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    // printf("Mapping kernel address space [%p, %p)\n", segments[i].start, segments[i].end);
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 14); // 0b1110 R W X
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
  // void *old_pdir = c->pdir;
  // c->pdir = (vme_enable ? (void *)get_satp() : NULL);
  // printf("__am_get_cur_as: old_pdir=%p, new_pdir=%p, satp=%p\n", 
  //        old_pdir, c->pdir, (void*)get_satp());
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
  // printf("__am_switch: c->pdir=%p, current satp=%p\n", 
  //        c->pdir, (void*)get_satp());
  // if (vme_enable && c->pdir != NULL) {
  //   set_satp(c->pdir);
  //   printf("  -> switched to %p\n", c->pdir);
  // }
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  // printf("Mapping address %p to %p\n", va, pa);
  
  PTE* pdir = (PTE*)as->ptr; // 页目录基质
  PTE* pte1 = &pdir[PDX(va)]; // 从页目录中取出页表项

  // 分配新的页表
  if (!(*pte1 & PTE_V)) {
    void* new_page_table = pgalloc_usr(PGSIZE);
    // printf("Allocating new page table for va %p at %p\n", va, new_page_table);
    memset(new_page_table, 0, PGSIZE);
    *pte1 = ((uintptr_t)new_page_table >> 12 << 10) | PTE_V; // 右移12位获得PPN, 左移10位放到PTE的正确位置, 把V置为1
  }

  PTE* page_table = (PTE*)PTE_ADDR((*pte1 >> 10 << 12)); // 用1级页表项拼出页表基址
  PTE* pte0 = &page_table[PTX(va)]; // 取出0级页表项, 用于指向真实的page frame
  *pte0 = ((uintptr_t)pa >> 12 << 10) | PTE_V | prot; // 把pa写进0级页表项
}

// 查询某个va是否被映射过, 映射过就返回对应的pa, 否则返回NULL
void* query_pa(AddrSpace *as, void *va) {
  PTE *pdir = (PTE*)as->ptr;
  PTE pte1 = pdir[PDX(va)];
  if (!(pte1 & PTE_V)) return NULL;

  PTE* ptable = (PTE*)(pte1 >> 10 << 12);

  PTE pte0 = ptable[PTX(va)];
  if (!(pte0 & PTE_V)) return NULL;
  return (void*)((pte0 >> 10 << 12) | ((uintptr_t)va & 0xfff));
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *c = (Context *)kstack.end - 1;
  c->mepc = (uintptr_t)entry;
  c->mstatus = 0x1800;
  c->gpr[2] = (uintptr_t)c;
  c->pdir = as->ptr;
  return c;
}
