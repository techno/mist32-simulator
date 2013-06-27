#define DPS_SIZE 0x200
#define GCI_HUB_SIZE 0x400
#define GCI_HUB_HEADER_SIZE 0x100
#define GCI_NODE_SIZE 0x400

#define DPS_UTIM64A 0x000
#define DPS_UTIM64B 0x040
#define DPS_SCI 0x100
#define DPS_MIMSR 0x120
#define DPS_LSFLAGS 0x1fc

#define UTIM64MCFG_ENA 0x1

typedef volatile struct _gci_hub_info {
  unsigned int total;
  unsigned int space_size;
} gci_hub_info;

typedef volatile struct _gci_hub_node {
  unsigned int size;
  unsigned int priority;
  unsigned int _reserved1;
  unsigned int _reserved2;
  unsigned int _reserved3;
} gci_hub_node;

typedef volatile struct _dps_utim64 {
  volatile unsigned int mcfg;
  volatile unsigned int mc[2];
  volatile unsigned int cc0[2];
  volatile unsigned int cc1[2];
  volatile unsigned int cc2[2];
  volatile unsigned int cc3[2];
  volatile unsigned int cc0cfg;
  volatile unsigned int cc1cfg;
  volatile unsigned int cc2cfg;
  volatile unsigned int cc3cfg;
} dps_utim64;

typedef volatile struct _dps_sci {
  volatile unsigned int txd;
  volatile unsigned int rxd;
  volatile unsigned int cfg;
} dps_sci;

void io_init(void);
void dps_init(void);
void gci_init(void);
void *io_load(Memory addr);
void io_info(void);
