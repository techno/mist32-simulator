#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <endian.h>

#include "common.h"
#include "interrupt.h"

CacheLineL1 cache_l1i[CACHE_L1_WAY][CACHE_L1_LINE_PER_WAY];
CacheLineL1 cache_l1d[CACHE_L1_WAY][CACHE_L1_LINE_PER_WAY];
unsigned long long cache_l1i_total, cache_l1i_hit;
unsigned long long cache_l1d_total, cache_l1d_hit;

PageEntry *page_table;
TLB memory_tlb[TLB_ENTRY_MAX];

int memory_is_fault;
Memory memory_io_writeback;

/* unsigned int tlb_access = 0, tlb_hit = 0; */

void memory_init(void)
{
  unsigned int i, w;

  page_table = calloc(PAGE_ENTRY_NUM, sizeof(PageEntry));

  /* internal virtual memory table flush */
  for(i = 0; i < PAGE_ENTRY_NUM; i++) {
    page_table[i].valid = false;
  }

  cache_l1i_total = 0;
  cache_l1i_hit = 0;
  cache_l1d_total = 0;
  cache_l1d_hit = 0;

  /* L1 cache flush */
  for(w = 0; w < CACHE_L1_WAY; w++) {
    for(i = 0; i < CACHE_L1_LINE_PER_WAY; i++) {
      cache_l1i[w][i].valid = false;
      cache_l1d[w][i].valid = false;
    }
  }

  memory_tlb_flush();
}

void memory_free(void)
{
  unsigned int i;
  PageEntry *entry;

  for(i = 0; i < PAGE_ENTRY_NUM; i++) {
    entry = &page_table[i];
    if(entry->valid) {
      free(entry->addr);
    }
  }

/*
#if TLB_ENABLE
  printf("[TLB] Info hit %d / %d\n", tlb_hit, tlb_access);
#endif
*/

#if CACHE_L1_PROFILE
  printf("[Cache] L1 I hit %lld / %lld\n", cache_l1i_hit, cache_l1i_total);
  printf("[Cache] L1 D hit %lld / %lld\n", cache_l1d_hit, cache_l1d_total);
#endif

  free(page_table);
}

void *memory_addr_mmio(Memory paddr, bool is_write)
{
  if(paddr >= IOSR) {
    /* memory mapped IO area */
    if(paddr & 0x3) {
      abort_sim();
      errx(EXIT_FAILURE, "Invalid alignment in IO area. Must be word align.");
    }

    if(is_write) {
      /* set io address for writeback */
      memory_io_writeback = paddr;
    }
    else {
      io_load(paddr);
    }

    return io_addr_get(paddr);
  }
  else if(paddr >= MEMORY_MAX_ADDR) {
    abort_sim();
    errx(EXIT_FAILURE, "No memory at %08x", paddr);
  }

  return NULL;
}

Memory memory_page_walk_L2(Memory vaddr, bool is_write)
{
  unsigned int *pdt, *pt, pte;
  unsigned int index_l1, index_l2, offset;
  Memory paddr;

#if TLB_ENABLE
  /* check TLB */
  do {
    unsigned int i, xoraddr;

    i = TLB_INDEX(vaddr);
    /* tlb_access++; */

    pte = memory_tlb[i].page_entry;

    if(!(pte & MMU_PTE_VALID)) {
      /* miss */
      continue;
    }

    xoraddr = memory_tlb[i].page_num ^ vaddr;

    if(xoraddr & MMU_PAGE_INDEX_L1) {
      /* miss */
      continue;
    }

    if(pte & MMU_PTE_PE) {
      /* Page Size Extension */
      paddr = (pte & MMU_PAGE_INDEX_L1) | (vaddr & MMU_PAGE_OFFSET_PSE);
    }
    else if(!(xoraddr & MMU_PAGE_NUM)) {
      paddr = (pte & MMU_PAGE_NUM) | (vaddr & MMU_PAGE_OFFSET);
    }
    else {
      /* miss */
      continue;
    }

    if(memory_check_privilege(pte, is_write)) {
      /* tlb_hit++; */
      return paddr;
    }
    else {
      if(DEBUG_MMU) abort_sim();
      DEBUGMMU("[MMU] PAGE ACCESS DENIED TLB at 0x%08x (%08x)\n", vaddr, pte);

      return memory_page_protection_fault(vaddr);
    }
  } while(0);
#endif

  /* Level 1 */
  pdt = memory_addr_phy2vm(PDTR, false);
  index_l1 = (vaddr & MMU_PAGE_INDEX_L1) >> 22;
  pte = pdt[index_l1];

  if(!(pte & MMU_PTE_VALID)) {
    /* Page Fault */
    if(DEBUG_MMU) abort_sim();
    DEBUGMMU("[MMU] PAGE FAULT L1 at 0x%08x (%08x)\n", vaddr, pte);

    return memory_page_fault(vaddr);
  }

  /* L1 privilege */
  if(!memory_check_privilege(pte, is_write)) {
    if(DEBUG_MMU) abort_sim();
    DEBUGMMU("[MMU] PAGE ACCESS DENIED L1 at 0x%08x (%08x)\n", vaddr, pte);

    return memory_page_protection_fault(vaddr);
  }

  if(pte & MMU_PTE_PE) {
    /* Page Size Extension */
#if TLB_ENABLE
    memory_tlb[TLB_INDEX(vaddr)].page_num = vaddr;
    memory_tlb[TLB_INDEX(vaddr)].page_entry = pte;
#endif

    offset = vaddr & MMU_PAGE_OFFSET_PSE;
    return (pte & MMU_PAGE_INDEX_L1) | offset;
  }

  /* Level 2 */
  pt = memory_addr_phy2vm(pte & MMU_PAGE_NUM, false);
  index_l2 = (vaddr & MMU_PAGE_INDEX_L2) >> 12;
  pte = pt[index_l2];

  if(!(pte & MMU_PTE_VALID)) {
    /* Page Fault */
    if(DEBUG_MMU) abort_sim();
    DEBUGMMU("[MMU] PAGE FAULT L2 at 0x%08x (%08x)\n", vaddr, pte);

    return memory_page_fault(vaddr);
  }

  /* L2 privilege */
  if(!memory_check_privilege(pte, is_write)) {
    if(DEBUG_MMU) abort_sim();
    DEBUGMMU("[MMU] PAGE ACCESS DENIED L2 at 0x%08x (%08x)\n", vaddr, pte);

    return memory_page_protection_fault(vaddr);
  }

#if TLB_ENABLE
  /* add TLB */
  memory_tlb[TLB_INDEX(vaddr)].page_num = vaddr;
  memory_tlb[TLB_INDEX(vaddr)].page_entry = pte;
#endif

  offset = vaddr & MMU_PAGE_OFFSET;
  return (pte & MMU_PAGE_NUM) | offset;
}

Memory memory_page_fault(Memory vaddr)
{
  memory_is_fault = IDT_PAGEFAULT_NUM;
  FI0R = vaddr;

  return MEMORY_MAX_ADDR;
}

Memory memory_page_protection_fault(Memory vaddr)
{
  memory_is_fault = IDT_INVALID_PRIV_NUM;
  FI0R = vaddr;

  return MEMORY_MAX_ADDR;
}

void memory_vm_alloc(Memory paddr, PageEntry *entry)
{
  if(!entry || entry->valid) {
    errx(EXIT_FAILURE, "page_alloc invalid entry");
  }
  else if((entry->addr = malloc(PAGE_SIZE)) == NULL) {
    err(EXIT_FAILURE, "page_alloc");
  }

  entry->valid = true;

  DPUTS("[Memory] alloc: Virt %p, Real 0x%08x on 0x%08x\n",
	entry->addr, paddr & PAGE_INDEX_MASK, paddr);
}

/* convert endian. must pass vm address */
void memory_vm_page_convert_endian(unsigned int *page)
{
  unsigned int i;
  unsigned int *value;

  value = page;

  for(i = 0; i < PAGE_SIZE_IN_WORD; i++) {
    /*
     *value = (*value >> 24) | (*value << 24) | ((*value >> 8) & 0xff00) | ((*value << 8) & 0xff0000);
     value++;
    */

    *value = htobe32(*value);
    value++;
  }
}

void memory_vm_convert_endian(void)
{
  unsigned int i;
    
  for(i = 0; i < PAGE_ENTRY_NUM; i++) {
    if(page_table[i].valid) {
      memory_vm_page_convert_endian(page_table[i].addr);
    }
  }
}
