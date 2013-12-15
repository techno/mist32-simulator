#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <endian.h>

#include "common.h"

PageEntry *page_table;
TLB memory_tlb[TLB_ENTRY_MAX];

/* unsigned int tlb_access = 0, tlb_hit = 0; */

void memory_init(void)
{
  unsigned int i;

  page_table = calloc(PAGE_ENTRY_NUM, sizeof(PageEntry));

  for(i = 0; i < PAGE_ENTRY_NUM; i++) {
    page_table[i].valid = false;
  }
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

  free(page_table);
}

void *memory_addr_get_nonmemory(Memory addr)
{
  if(addr >= IOSR) {
    /* memory mapped IO area */
    return io_addr_get(addr);
  }
  else if(addr >= MEMORY_MAX_ADDR) {
    errx(EXIT_FAILURE, "no memory at %08x", addr);
  }

  return NULL;
}

void *memory_addr_get_L2page(Memory addr, bool is_write)
{
  unsigned int *pdt, *pt;
  unsigned int index_l1, index_l2, offset, phyaddr;

  if(PSR_MMUPS != PSR_MMUPS_4KB) {
    errx(EXIT_FAILURE, "MMU page size (%d) not supported.", PSR_MMUPS);
  }

#if TLB_ENABLE
  /* check TLB */
  do {
    unsigned int i, xoraddr;

    i = TLB_INDEX(addr);
    /* tlb_access++; */

    if(!(memory_tlb[i].page_entry & MMU_PTE_VALID)) {
      /* miss */
      continue;
    }

    xoraddr = memory_tlb[i].page_num ^ addr;

    if(xoraddr & MMU_PAGE_INDEX_L1) {
      /* miss */
      continue;
    }

    if(memory_tlb[i].page_entry & MMU_PTE_PE) {
      /* Page Size Extension */
      phyaddr = (memory_tlb[i].page_entry & MMU_PAGE_INDEX_L1) | (addr & MMU_PAGE_OFFSET_PSE);
    }
    else if(!(xoraddr & MMU_PAGE_NUM)) {
      phyaddr = (memory_tlb[i].page_entry & MMU_PAGE_NUM) | (addr & MMU_PAGE_OFFSET);
    }
    else {
      /* miss */
      continue;
    }

    if(memory_check_privilege(memory_tlb[i].page_entry, is_write)) {
      /* tlb_hit++; */
      return memory_addr_get_from_physical(phyaddr);
    }
    else {
      errx(EXIT_FAILURE, "PAGE ACCESS DENIED TLB at 0x%08x (%08x)", addr, memory_tlb[i].page_entry);
    }
  } while(0);
#endif

  /* Level 1 */
  pdt = memory_addr_get_from_physical(PDTR);
  index_l1 = (addr & MMU_PAGE_INDEX_L1) >> 22;

  if(!(pdt[index_l1] & MMU_PTE_VALID)) {
    /* Page Fault */
    abort_sim();
    errx(EXIT_FAILURE, "PAGE FAULT L1 at 0x%08x (%08x)", addr, pdt[index_l1]);
  }

  /* L1 privilege */
  if(!memory_check_privilege(pdt[index_l1], is_write)) {
    abort_sim();
    errx(EXIT_FAILURE, "PAGE ACCESS DENIED L1 at 0x%08x (%08x)", addr, pdt[index_l1]);
  }

  if(pdt[index_l1] & MMU_PTE_PE) {
    /* Page Size Extension */
#if TLB_ENABLE
    memory_tlb[TLB_INDEX(addr)].page_num = addr;
    memory_tlb[TLB_INDEX(addr)].page_entry = pdt[index_l1];
#endif

    offset = addr & MMU_PAGE_OFFSET_PSE;
    phyaddr = (pdt[index_l1] & MMU_PAGE_INDEX_L1) | offset;
    return memory_addr_get_from_physical(phyaddr);
  }

  /* Level 2 */
  pt = memory_addr_get_from_physical(pdt[index_l1] & MMU_PAGE_NUM);
  index_l2 = (addr & MMU_PAGE_INDEX_L2) >> 12;

  if(!(pt[index_l2] & MMU_PTE_VALID)) {
    /* Page Fault */
    abort_sim();
    errx(EXIT_FAILURE, "PAGE FAULT L2 at 0x%08x (%08x)", addr, pt[index_l2]);
  }

  /* L2 privilege */
  if(!memory_check_privilege(pt[index_l2], is_write)) {
    abort_sim();
    errx(EXIT_FAILURE, "PAGE ACCESS DENIED L2 at 0x%08x (%08x)", addr, pt[index_l2]);
  }

#if TLB_ENABLE
  /* add TLB */
  memory_tlb[TLB_INDEX(addr)].page_num = addr;
  memory_tlb[TLB_INDEX(addr)].page_entry = pt[index_l2];
#endif

  offset = addr & MMU_PAGE_OFFSET;
  phyaddr = (pt[index_l2] & MMU_PAGE_NUM) | offset;
  return memory_addr_get_from_physical(phyaddr);
}

void memory_page_alloc(Memory addr, PageEntry *entry)
{
  if(!entry || entry->valid) {
    errx(EXIT_FAILURE, "page_alloc invalid entry");
  }
  else if((entry->addr = malloc(PAGE_SIZE)) == NULL) {
    err(EXIT_FAILURE, "page_alloc");
  }

  entry->valid = true;

  DPUTS("[Memory] alloc: Virt %p, Real 0x%08x on 0x%08x\n",
	entry->addr, addr & PAGE_INDEX_MASK, addr);
}

/* convert endian. must pass real address */
void memory_page_convert_endian(unsigned int *page)
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

void memory_convert_endian(void)
{
  unsigned int i;
    
  for(i = 0; i < PAGE_ENTRY_NUM; i++) {
    if(page_table[i].valid) {
      memory_page_convert_endian(page_table[i].addr);
    }
  }
}
