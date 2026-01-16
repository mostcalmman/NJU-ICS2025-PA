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

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);

  if (fd == -1) {
      Log("Error: Can not open file '%s'", filename);
      // 可以在这里做一个 fallback，比如尝试去掉 /bin/ 前缀再次打开
      // fd = fs_open(filename + 5, 0, 0); 
      // if (fd == -1) ...
      assert(0); 
  }

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
  // ramdisk_read(phdr, ehdr.e_phoff, phdr_size);
  for (int i = 0; i < ehdr.e_phnum; i ++) {
    if (phdr[i].p_type == PT_LOAD) {
      // ramdisk_read((void *)phdr[i].p_vaddr, phdr[i].p_offset, phdr[i].p_filesz);
      fs_lseek(fd, phdr[i].p_offset, SEEK_SET);
      fs_read(fd, (void *)phdr[i].p_vaddr, phdr[i].p_filesz);
      memset((void *)(phdr[i].p_vaddr + phdr[i].p_filesz), 0, phdr[i].p_memsz - phdr[i].p_filesz);
    }
  }

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

static void* constructUserArgs(void *sp, const char *filename, char *const argv[], char *const envp[]) {
  // 传进来的第一个参数argv[0]是程序名

  uintptr_t user_argv[127];
  uintptr_t user_envp[127];
  int argc = 0;
  int envc = 0;

  assert(argv);

  // **argv(倒着放)
  for (char* const *p = argv; p && *p; ++p) {
    sp -= strlen(*p) + 1;
    strcpy(sp, *p);
    user_argv[argc] = (uintptr_t)sp;
    ++argc;
  }

  // **envp(倒着放)
  for (char* const *p = envp; p && *p; ++p) {
    sp -= strlen(*p) + 1;
    strcpy(sp, *p);
    user_envp[envc] = (uintptr_t)sp;
    ++envc;
  }

  // 放一个NULL
  sp -= sizeof(char*);
  *(void**)sp = NULL; // sp转为二阶指针, sp指向的才是一个指针, 才可以存NULL

  // envp指针数组
  for (int i = envc - 1; i >=0; --i) {
    sp -= sizeof(char*);
    *(uintptr_t*)sp = user_envp[i];
  }

  // 放一个NULL
  sp -= sizeof(char*);
  *(void**)sp = NULL;

  // argv指针数组
  for (int i = argc - 1; i >=0; --i) {
    sp -= sizeof(char*);
    *(uintptr_t*)sp = user_argv[i];
  }

  // argc
  sp -= sizeof(int);
  *(int*)sp = argc;

  return sp;
}

// 这部分代码应该是可移植的
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  void *usersp = (char*)new_page(8) + 8 * PGSIZE; // 创建用户栈
  usersp = constructUserArgs(usersp, filename, argv, envp);

  void *entry = (void*)loader(pcb, filename);
  pcb->cp = ucontext(&pcb->as, (Area){pcb->stack, pcb->stack + STACK_SIZE}, entry);
  
  pcb->cp->GPRx = (uintptr_t)usersp;
}