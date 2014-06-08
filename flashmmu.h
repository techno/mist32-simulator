#define FLASHMMU_ENABLE 1
#define FLASHMMU_SSDALLOC 0
#define FLASHMMU_START_ADDR 0x80000000
#define FLASHMMU_AREA_SIZE (					     \
  FLASHMMU_PAGEBUF_SIZE						     \
  + FLASHMMU_OBJCACHE_SIZE					     \
  + FLASHMMU_OBJ_MAX * sizeof(FLASHMMU_Object))

// 256MB Total
#define FLASHMMU_SIZE 0x10000000
#define FLASHMMU_PAGE_SIZE 0x1000
#define FLASHMMU_OBJ_MAX (FLASHMMU_SIZE / FLASHMMU_PAGE_SIZE)

// 4MB Page Buffer
#define FLASHMMU_PAGEBUF_SIZE 0x00400000
#define FLASHMMU_PAGEBUF_MAX (FLASHMMU_PAGEBUF_SIZE >> 12)
#define FLASHMMU_PAGEBUF_ADDR (FLASHMMU_START_ADDR)

#define FLASHMMU_PAGEBUF_WAY 8
#define FLASHMMU_PAGEBUF_PER_WAY (FLASHMMU_PAGEBUF_MAX / FLASHMMU_PAGEBUF_WAY)
#define FLASHMMU_PAGEBUF_HASH(objid) ((objid) & (FLASHMMU_PAGEBUF_PER_WAY - 1))
#define FLASHMMU_PAGEBUF_OBJID(entry) ((entry) >> 12)
#define FLASHMMU_PAGEBUF_FLAGS(entry) ((entry) & 0xfff)
#define FLASHMMU_PAGEBUF_TAG(objid, flags) (((objid) << 12) | (flags & 0xfff))

// 16MB RAM Object Cache
#define FLASHMMU_OBJCACHE_SIZE 0x01000000
#define FLASHMMU_OBJCACHE_ADDR (FLASHMMU_START_ADDR + FLASHMMU_PAGEBUF_SIZE)

// FLAGS
#define FLASHMMU_FLAGS_VALID 0x1
#define FLASHMMU_FLAGS_OBJCACHE 0x2
#define FLASHMMU_FLAGS_PAGEBUF 0x4
#define FLASHMMU_FLAGS_ACCESS 0x8
#define FLASHMMU_FLAGS_DIRTY 0x10
#define FLASHMMU_FLAGS_DIRTYBUF 0x20 /* FIXME: dirtybuf flag should have pagebuf tag. */

#define FLASHMMU_OBJID(paddr) ((paddr) >> 12)
#define FLASHMMU_OFFSET(paddr) ((paddr) & 0xfff)
#define FLASHMMU_ADDR(objid) ((objid) << 12)
#define FLASHMMU_SECTOR(objid) (((objid) << 12) >> 9)
#define FLASHMMU_BLOCKS(size) (((size) + 511) >> 9)

#define FLASHMMU_PAGEBUF_OBJ(pagebuf, hash, way) ((char *)(pagebuf) + ((FLASHMMU_PAGEBUF_PER_WAY * (way) + (hash)) << 12))
#define FLASHMMU_OBJCACHE_OBJ(objcache, offset) ((char *)(objcache) + ((offset) << 9))

/* Total: 64bit */
typedef struct _flashmmu_object {
  /* Little Endian */
  uint16_t flags;
  uint16_t size;         /* 12 */
  /* --- */
  uint32_t cache_offset; /* - 23 */
} FLASHMMU_Object;

typedef struct _flashmmu_pagebuf_tag {
  uint32_t tag[FLASHMMU_PAGEBUF_WAY];
  uint32_t last_access[FLASHMMU_PAGEBUF_WAY];
} FLASHMMU_PagebufTag;

extern char *flashmmu_mem;

void flashmmu_init(void);
Memory flashmmu_access(uint32_t pte, Memory vaddr, bool is_write);
