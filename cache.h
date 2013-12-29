/* L1 Cache */
#define CACHE_L1_I_ENABLE 1
#define CACHE_L1_D_ENABLE 1
#define CACHE_L1_PROFILE 1

#define CACHE_L1_WAY 4
#define CACHE_L1_LINE_PER_WAY 16
#define CACHE_L1_LINE_SIZE 16 /* number of word */
#define CACHE_L1_LINE_MASK 0xffffffc0
#define CACHE_L1_TAG(addr) (addr & 0xfffffc00)
#define CACHE_L1_INDEX(addr) ((addr >> 6) & 0xf)
#define CACHE_L1_WORD(addr) ((addr >> 2) & 0xf)

typedef struct _cachelinel1 {
  bool valid;
  uint32_t tag;
  unsigned int miss;
} CacheLineL1;

extern CacheLineL1 cache_l1i[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY];
extern CacheLineL1 cache_l1d[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY];
extern uint32_t cacheline_l1i[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY][CACHE_L1_LINE_SIZE];
extern uint32_t cacheline_l1d[CACHE_L1_LINE_PER_WAY][CACHE_L1_WAY][CACHE_L1_LINE_SIZE];
extern unsigned long long cache_l1i_total, cache_l1i_hit;
extern unsigned long long cache_l1d_total, cache_l1d_hit;

#if CACHE_L1_I_ENABLE || CACHE_L1_D_ENABLE

static inline uint32_t memory_cache_l1_read(Memory paddr, int is_icache)
{
  CacheLineL1 (*cache)[CACHE_L1_WAY];
  uint32_t (*cacheline)[CACHE_L1_WAY][CACHE_L1_LINE_SIZE];
  unsigned int w, i;
  unsigned int tag, index, word;
  unsigned int miss, maxmiss, target;

  if(paddr >= MEMORY_MAX_ADDR) {
    /* non-cache area */
    return *(uint32_t *)memory_addr_phy2vm(paddr, false);
  }

  if(is_icache) {
    cache = cache_l1i;
    cacheline = cacheline_l1i;
#if CACHE_L1_PROFILE
    cache_l1i_total++;
#endif
  }
  else {
    cache = cache_l1d;
    cacheline = cacheline_l1d;
#if CACHE_L1_PROFILE
    cache_l1d_total++;
#endif
  }

  tag = CACHE_L1_TAG(paddr);
  index = CACHE_L1_INDEX(paddr);
  word = CACHE_L1_WORD(paddr);

  for(w = 0; w < CACHE_L1_WAY; w++) {
    if(cache[index][w].tag == tag && cache[index][w].valid) {
      /* hit */
      for(i = 0; i < CACHE_L1_WAY; i++) {
	/* LRU */
	cache[index][i].miss++;
      }
      cache[index][w].miss = 0;

#if CACHE_L1_PROFILE
      if(is_icache) {
	cache_l1i_hit++;
      }
      else {
	cache_l1d_hit++;
      }
#endif

      return cacheline[index][w][word];
    }
  }

  /* miss */
  maxmiss = 0;
  target = 0;

  /* find victim by LRU */
  for(w = 0; w < CACHE_L1_WAY; w++) {
    if(!cache[index][w].valid) {
      target = w;
      break;
    }

    miss = cache[index][w].miss;
    if(maxmiss < miss) {
      maxmiss = miss;
      target = w;
    }
  }

  cache[index][target].valid = true;
  cache[index][target].miss = 0;
  cache[index][target].tag = tag;
  memcpy(cacheline[index][target],
	 memory_addr_phy2vm(paddr & CACHE_L1_LINE_MASK, false),
	 CACHE_L1_LINE_SIZE * sizeof(uint32_t));

  return cacheline[index][target][word];
}

static inline void memory_cache_l1_write(Memory paddr, uint32_t data)
{
  int w;
  unsigned int tag, index;

  tag = CACHE_L1_TAG(paddr);
  index = CACHE_L1_INDEX(paddr);

  if(paddr >= MEMORY_MAX_ADDR) {
    /* non-cache area */
#if CACHE_L1_D_ENABLE
    *(uint32_t *)memory_addr_phy2vm(paddr, true) = data;
#endif
    return;
  }

#if CACHE_L1_I_ENABLE
  for(w = 0; w < CACHE_L1_WAY; w++) {
    if(cache_l1i[index][w].tag == tag) {
      /* hit */
      cache_l1i[index][w].valid = false;
      break;
    }
  }
#endif

#if CACHE_L1_D_ENABLE
  unsigned int word;

  /* writethrough */
  *(uint32_t *)memory_addr_phy2vm(paddr, true) = data;

  for(w = 0; w < CACHE_L1_WAY; w++) {
    if(cache_l1d[index][w].tag == tag && cache_l1d[index][w].valid) {
      /* hit */
      word = CACHE_L1_WORD(paddr);
      cacheline_l1d[index][w][word] = data;

#if CACHE_L1_PROFILE
      cache_l1d_total++;
      cache_l1d_hit++;
#endif
      return;
    }
  }

  /* miss */
  memory_cache_l1_read(paddr, 0);

#if CACHE_L1_PROFILE
  cache_l1d_total++;
#endif
#endif
}

#endif
