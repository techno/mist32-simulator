#include <stdbool.h>

#define PAGE_SIZE (16384)        /* 2 ^ 14 */
#define PAGE_ENTRY_NUM (262144)  /* 2 ^ 18 */
#define PAGE_SIZE_IN_WORD (PAGE_SIZE >> 2)

#define PAGE_OFFSET_BIT_NUM (14) /* 16KB   */
#define PAGE_OFFSET_MASK 0x00003fff /* 14 bit */
#define PAGE_NUM_MASK 0x0003ffff /* 18 bit */
#define PAGE_INDEX_MASK (PAGE_NUM_MASK << PAGE_OFFSET_BIT_NUM)

#define MEMP(addr) ((unsigned int *)memory_addr_get(addr))
/* for little endian */
#define MEMP8(addr) ((unsigned char *)memory_addr_get(addr ^ 3))
#define MEMP16(addr) ((unsigned short *)memory_addr_get(addr ^ 2))

typedef unsigned int Memory;

typedef struct _pageentry {
  bool valid;
  void *addr;
} PageEntry;

extern PageEntry *page_table;

void memory_init(void);
void memory_free(void);
void *memory_addr_get(Memory addr);
void *memory_page_addr(Memory addr);
void memory_convert_endian(void);
