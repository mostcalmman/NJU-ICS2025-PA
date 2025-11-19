#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

#define ALIGN_MASK 7
static void *addr = NULL;

void *malloc(size_t size) {
//   // On native, malloc() will be called during initializaion of C runtime.
//   // Therefore do not call panic() here, else it will yield a dead recursion:
//   //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
// #if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
//   panic("Not implemented");
// #endif


  if (size == 0) { return NULL; }

  if (addr == NULL) { addr = heap.start; }

  uintptr_t current_addr_int = (uintptr_t)addr;
  uintptr_t aligned_addr_int = (current_addr_int + ALIGN_MASK) & ~ALIGN_MASK; // 8字节对齐
  void *current_addr = (void *)aligned_addr_int;

  if(size > PTRDIFF_MAX){
      printf("Error: malloc size too large! (Request size: %u)\n", size);
      return NULL;
  }

  void *next_addr = (void *)((char *)current_addr + size);

  if (next_addr > heap.end) {
      printf("Error: malloc out of memory! (Request size: %u)\n", size);
      return NULL;
  }

  addr = next_addr;
  return current_addr;
}

void free(void *ptr) {
  // 简化情况, 直接留空
}

#endif
