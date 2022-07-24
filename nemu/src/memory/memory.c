#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"
#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) 
		return mmio_read(addr, len, map_NO);
  else 
		return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
	int map_NO = is_mmio(addr);
	if (map_NO != -1) 
		mmio_write(addr, len, data, map_NO);
    else 
		memcpy(guest_to_host(addr), &data, len);
}

paddr_t page_translate(vaddr_t addr, bool is_write) {
  PDE pde, *page_direct;
  PTE pte, *page_tabes;
  paddr_t paddr = addr;

  if (cpu.cr0.protect_enable && cpu.cr0.paging) {
    page_direct = (PDE *)(intptr_t)(cpu.cr3.page_directory_base << 12);
    pde.val = paddr_read((intptr_t)&page_direct[(addr >> 22) & 0x3ff], 4);
    assert(pde.present);
    pde.accessed = 1;
    page_tabes = (PTE *)(intptr_t)(pde.page_frame << 12);
    pte.val = paddr_read((intptr_t)&page_tabes[(addr >> 12) & 0x3ff], 4);
    assert(pte.present);
    pte.accessed = 1;
    pte.dirty = is_write ? 1 : pte.dirty;

    paddr = (pte.page_frame << 12) | (addr & PAGE_MASK);
  }
  return paddr;
}
#define CROSSES(addr, len) \
  ((((addr) + (len) - 1) & ~PAGE_MASK) != ((addr) & ~PAGE_MASK))
uint32_t vaddr_read(vaddr_t addr, int len) {
  paddr_t paddr;

  if (!CROSSES(addr, len)) {
    paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  } 
  else {
    union {
      uint8_t bytes[4];
      uint32_t dword;
    } data = {0};
    for (int i = 0; i < len; i++) {
      paddr = page_translate(addr + i, false);
      data.bytes[i] = (uint8_t)paddr_read(paddr, 1);
    }
    return data.dword; 
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  paddr_t paddr;

  if (!CROSSES(addr, len)) {
    paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  } 
  else {
    assert(0);
    for (int i = 0; i < len; i++) {
      paddr = page_translate(addr, true);
      paddr_write(paddr, 1, data);
      data >>= 8;
      addr++;
    } 
  }
}
