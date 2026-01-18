#include <proc.h>
#include <elf.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#ifdef __ISA_RISCV32__
#define EXPECT_ISA EM_RISCV
#elif defined(__ISA_NATIVE__)
#define EXPECT_ISA EM_X86_64
#endif

#define USER_SPACE RANGE(0x40000000, 0x80000000)

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);

  Elf_Ehdr ehdr;
  // ramdisk_read(&ehdr, 0, sizeof(ehdr));
  fs_read(fd, &ehdr, sizeof(ehdr));
  // 注意是小端存储, 最低有效位在最后
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f); // 0x7f 'E' 'L' 'F'
  assert(ehdr.e_machine == EXPECT_ISA);
  int phdr_size = ehdr.e_phnum * ehdr.e_phentsize;
  Elf_Phdr *phdr = malloc(phdr_size);
  fs_lseek(fd, ehdr.e_phoff, SEEK_SET);
  fs_read(fd, phdr, phdr_size);
  uintptr_t max_vaddr = 0;
  for (int i = 0; i < ehdr.e_phnum; i ++) {
    if (phdr[i].p_type == PT_LOAD) {
      Log("MARK1");
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      int nr_pg = phdr[i].p_memsz % PGSIZE == 0 ? phdr[i].p_memsz / PGSIZE : phdr[i].p_memsz / PGSIZE + 1;
      for (int j = 0; j < nr_pg; j ++) {
        void *usrpg = new_page(1);
        map(&pcb->as, (void*)(phdr[i].p_vaddr + j * PGSIZE), usrpg, 14); // R W X
        Log("Mapped user page %d for segment %d at vaddr %p to phys addr %p", j, i, (void*)(phdr[i].p_vaddr + j * PGSIZE), usrpg);
      }
      Log("MARK2");
      fs_read(fd, (void *)phdr[i].p_vaddr, phdr[i].p_filesz);
      Log("MARK3");
      memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
      uintptr_t end_vaddr = phdr[i].p_vaddr + phdr[i].p_memsz;
      if (end_vaddr > max_vaddr) {
        max_vaddr = end_vaddr;
      }
    }
  }

  pcb->max_brk = max_vaddr;

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

#define PUSH_STR(src) { \
    size_t len = strlen(src) + 1; \
    psp -= len; vsp -= len; \
    strcpy((char *)psp, src); \
  }
#define PUSH_VAL(type, val) { \
    psp -= sizeof(type); vsp -= sizeof(type); \
    *(type *)psp = val; \
  }
static void* constructUserArgs(void *vsp, void *psp, const char *filename, char *const argv[], char *const envp[]) {
  /*
  传进来的第一个参数argv[0]是程序名
  此时仍然处于内核空间, 无法直接向用户空间的虚拟地址上写内容
  但内核空间采用身份映射(Identity Mapping(, 物理地址pa在数值上等于内核可以直接访问的指针
  所以直接把对vsp的操作改为对psp的即可
  */ 

  uintptr_t user_argv[127];
  uintptr_t user_envp[127];
  int argc = 0;
  int envc = 0;

  if (!argv) {
    // 如果argv为空, 就放程序名
    Log("argv is NULL");
    PUSH_STR(filename)
    user_argv[0] = (uintptr_t)vsp;
    argc = 1;
  } else{
    // **argv(倒着放)
    for (char* const *p = argv; p && *p; ++p) {
      PUSH_STR(*p)
      user_argv[argc] = (uintptr_t)vsp;
      ++argc;
    }
  }

  // **envp(倒着放)
  for (char* const *p = envp; p && *p; ++p) {
    PUSH_STR(*p)
    user_envp[envc] = (uintptr_t)vsp;
    ++envc;
  }

  // 放一个NULL
  // sp -= sizeof(char*);
  // *(void**)sp = NULL; // sp转为二阶指针, sp指向的才是一个指针, 才可以存NULL
  PUSH_VAL(void*, NULL)

  // envp指针数组
  for (int i = envc - 1; i >=0; --i) {
    PUSH_VAL(uintptr_t, user_envp[i]);
  }

  // 放一个NULL
  PUSH_VAL(void*, NULL)

  // argv指针数组
  for (int i = argc - 1; i >=0; --i) {
    PUSH_VAL(uintptr_t, user_argv[i]);
  }

  // argc
  PUSH_VAL(int, argc)

  return vsp;
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  protect(&pcb->as); // 将内核地址映射保护到pcb的地址空间中

  // 创建用户栈, 处于[as.area.end - 32KB, as.area.end)
  void *usr_stack_top = pcb->as.area.end - STACK_SIZE;
  void *psp_page_start = NULL;
  for (int i = 0; i < 8; i ++) {
    void *stackpg = new_page(1);
    if (i == 7) {
      psp_page_start = stackpg;
    }
    map(&pcb->as, (void*)(usr_stack_top + i * PGSIZE), stackpg, 14); // R W X
  }
  void *usrsp_v = pcb->as.area.end;
  void *usrsp_p = psp_page_start + PGSIZE;
  Log("User stack range [%p, %p)", usr_stack_top, usrsp_v);

  usrsp_v = constructUserArgs(usrsp_v, usrsp_p, filename, argv, envp);

  void *entry = (void*)loader(pcb, filename);
  pcb->cp = ucontext(&pcb->as, (Area){pcb->stack, pcb->stack + STACK_SIZE}, entry);
  Log("User heap starts at %p", (void*)pcb->max_brk);
  
  pcb->cp->GPRx = (uintptr_t)usrsp_v;
}