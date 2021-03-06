#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include "common.h"
#include "debug.h"
#include "registers.h"
#include "vm.h"
#include "mmu.h"
#include "tlb.h"
#include "cache.h"
#include "io.h"
#include "interrupt.h"
#include "utils.h"

PageEntry page_table[PAGE_ENTRY_NUM] __attribute__ ((aligned(64)));

CacheLineL1 cache_l1i[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY];
CacheLineL1 cache_l1d[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY];
uint32_t cacheline_l1i[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY][CACHE_L1_LINE_SIZE] __attribute__ ((aligned(64)));
uint32_t cacheline_l1d[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY][CACHE_L1_LINE_SIZE] __attribute__ ((aligned(64)));
unsigned int cache_tick;
unsigned long long cache_l1i_total, cache_l1i_hit;
unsigned long long cache_l1d_total, cache_l1d_hit;

int memory_is_fault;
Memory memory_io_writeback;

TLB memory_tlb[TLB_ENTRY_MAX] __attribute__ ((aligned(64)));
unsigned long long tlb_access, tlb_hit;

void memory_init(void)
{
  unsigned int i, w;

  /* internal virtual memory table flush */
  for(i = 0; i < PAGE_ENTRY_NUM; i++) {
    page_table[i].valid = false;
  }

  cache_tick = 0;
  cache_l1i_total = 0;
  cache_l1i_hit = 0;
  cache_l1d_total = 0;
  cache_l1d_hit = 0;

  /* L1 cache flush */
  for(i = 0; i < CACHE_L1_LINE_PER_WAY; i++) {
    for(w = 0; w < CACHE_L1_WAY; w++) {
      cache_l1i[i][w].valid = false;
      cache_l1d[i][w].valid = false;
    }
  }

  tlb_access = 0;
  tlb_hit = 0;

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

#if CACHE_L1_PROFILE
  NOTICE("[Cache] L1 I hit %lld / %lld\n", cache_l1i_hit, cache_l1i_total);
  NOTICE("[Cache] L1 D hit %lld / %lld\n", cache_l1d_hit, cache_l1d_total);
#endif

#if TLB_PROFILE
  NOTICE("[TLB] hit %lld / %lld\n", tlb_hit, tlb_access);
#endif
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

Memory memory_page_walk_L2(Memory vaddr, bool is_write, bool is_exec)
{
  uint32_t *pdt, *pt, pte;
  unsigned int index_l1, index_l2, offset;

#if !NO_DEBUG
  if(vaddr == 0) {
    NOTICE("[NOTICE] NULL POINTER ACCESS %08x\n", PCR);
  }
#endif

  /* Level 1 */
  if((PSR & PSR_CMOD_MASK) == PSR_CMOD_USER) {
    pdt = memory_addr_phy2vm(PDTR, false);
  }
  else{
    pdt = memory_addr_phy2vm(KPDTR, false);
  }

  index_l1 = (vaddr & MMU_PAGE_INDEX_L1) >> 22;
  pte = pdt[index_l1];

  if(!(pte & MMU_PTE_VALID)) {
    /* Page Fault */
    if(DEBUG_MMU) abort_sim();
    DEBUGMMU("[MMU] PAGE FAULT L1 at 0x%08x (%08x)\n", vaddr, pte);

    return memory_page_fault(vaddr);
  }

  /* L1 privilege */
  if(!memory_check_privilege(pte, is_write, is_exec)) {
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
  if(!memory_check_privilege(pte, is_write, is_exec)) {
    if(DEBUG_MMU) abort_sim();
    DEBUGMMU("[MMU] PAGE ACCESS DENIED L2 at 0x%08x (%08x)\n", vaddr, pte);

    return memory_page_protection_fault(vaddr);
  }

  pte |= MMU_PTE_R | (is_write ? MMU_PTE_D : 0);
  pt[index_l2] = pte;

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
  /* FIXME: must be set fault factor */
  FI1R = 0;

  return MEMORY_MAX_ADDR;
}

Memory memory_page_protection_fault(Memory vaddr)
{
  memory_is_fault = IDT_INVALID_PRIV_NUM;
  FI0R = vaddr;

  return MEMORY_MAX_ADDR;
}

void memory_vm_alloc(Memory paddr, unsigned int page_num)
{
  if(page_table[page_num].valid) {
    errx(EXIT_FAILURE, "page_alloc invalid entry");
  }
#if MEMORY_CALLOC
  else if((page_table[page_num].addr = calloc(1, PAGE_SIZE)) == NULL) {
#else
  else if((page_table[page_num].addr = malloc(PAGE_SIZE)) == NULL) {
#endif
    err(EXIT_FAILURE, "page_alloc");
  }

  page_table[page_num].valid = true;

  DPUTS("[Memory] alloc: Virt %p, Real 0x%08x on 0x%08x\n",
	page_table[page_num].addr, paddr & PAGE_INDEX_MASK, paddr);
}

void *memory_vm_memcpy(void *dest, const void *src, size_t n)
{
  unsigned int i;

  memcpy(dest, src, n & (~3));

  for(i = n & (~3); i < n; i++) {
    ((char *)dest)[i ^ 2] = ((char *)src)[i ^ 2];
  }

  return dest;
}

int memory_vm_memcmp(const void *s1, const void *s2, size_t n)
{
  int cmp;
  unsigned int i;

  if((cmp = memcmp(s1, s2, n & (~3)))) {
    return cmp;
  }

  for(i = n & (~3); i < n; i++) {
    if(((char *)s1)[i ^ 2] != ((char *)s2)[i ^ 2]) {
      return 1;
    }
  }

  return 0;
}

/* convert endian. must pass vm address */
static inline void memory_vm_page_convert_endian(uint32_t *page)
{
  unsigned int i;
  uint32_t *value;

  value = page;

  for(i = 0; i < PAGE_SIZE_IN_WORD; i++) {
    *value = __builtin_bswap32(*value);
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
