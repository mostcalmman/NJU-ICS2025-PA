/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>

typedef uintptr_t PTE;

#define PDX(va) (((uintptr_t)(va) >> 22) & 0x3ff) // VPN[1]
#define PTX(va) (((uintptr_t)(va) >> 12) & 0x3ff) // VPN[0]
#define PTE_ADDR(pte) ((uintptr_t)((uintptr_t)(pte) & ~0xfff)) // 低12位置0
#define PTE_OFFSET(pte) ((uintptr_t)(pte) & 0xfff) // 低12位

static inline uintptr_t get_satp() {
  return cpu.satp;
}

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  uintptr_t satp = get_satp();
  bool mode = (satp >> 31) & 0x1;
  return mode ? MMU_TRANSLATE : MMU_DIRECT;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  static FILE *fp = NULL;
  if (fp == NULL) {
    fp = fopen("/home/liushengrui/ics2025/_Log", "a");
    if (fp == NULL) { assert(0); } // 确保打开成功
  }
  bool flag = false;
  if ((vaddr & ~0xfff) == 0x7ffff000 && vaddr > 0x7ffffc68) {
    fprintf(fp, "Translating special vaddr 0x%x", vaddr);
    flag = true;
  }
  uintptr_t satp = get_satp();
  paddr_t pdir_base = (satp & 0x3fffff) << 12;
  paddr_t pte1_addr = pdir_base + (PDX(vaddr) * 4);
  PTE pte1 = paddr_read(pte1_addr, 4);
  if (flag) {
    fprintf(fp, "pdir: 0x%x, PTE1 addr: 0x%x, value: 0x%x\n", pdir_base, pte1_addr, (uint32_t)pte1);
  }
  if(!(pte1 & 0x1)) {
    fprintf(fp, "Page table is invalid(addr = %x, value = 0x%x), operating on 0x%x", pte1_addr, (uint32_t)pte1, vaddr);
    fflush(fp);
    assert(0);
  }
  paddr_t pt_base = (pte1 >> 10) << 12;
  paddr_t pte0_addr = pt_base + (PTX(vaddr) * 4);
  PTE pte0 = paddr_read(pte0_addr, 4);
  if(!(pte0 & 0x1)) {
    fprintf(fp, "Page frame is invalid(addr = %x, value = 0x%x), operating on 0x%x", pte0_addr, (uint32_t)pte0, vaddr);
    fflush(fp);
    assert(0);
  }
  paddr_t paddr = PTE_ADDR(pte0 >> 10 << 12) | PTE_OFFSET(vaddr);
  return paddr;
}
