#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include "common.h"
#include "debug.h"
#include "registers.h"
#include "dps.h"
#include "gci.h"

void io_init(void)
{
  NOTICE("[System] I/O Initialize... \n");
  IOSR = 0;
  dps_init();
  gci_init();

  gci_info();
}

void io_close(void)
{
  gci_close();
  dps_close();
}

void *io_addr_get(Memory addr)
{
  Memory offset, p;
  int i;

  if(addr < IOSR) {
    errx(EXIT_FAILURE, "io_load invalid IO address at %08x", addr);
  }
  else if(addr & 0x3) {
    errx(EXIT_FAILURE, "io_load invalid IO alignment at %08x", addr);
  }

  offset = addr - IOSR;
  
  if(offset < DPS_SIZE) {
    /* DPS */
    DPUTS("[I/O] DPS: Addr: 0x%08x\n", offset);
    return (char *)dps + offset;
  }
  else if(offset < DPS_SIZE + GCI_HUB_SIZE) {
    /* GCI Hub */
    p = offset - DPS_SIZE;
    DPUTS("[I/O] GCI Hub: Addr: 0x%08x\n", p);
    return (char *)gci_hub + p;
  }
  else {
    p = DPS_SIZE + GCI_HUB_SIZE;

    for(i = 0; i < GCI_NODE_MAX; i++) {
      if(offset < p + GCI_NODE_SIZE) {
	/* GCI Node Info */
	DPUTS("[I/O] GCI Node Info: %d, Addr: 0x%08x\n", i, offset - p);
	return (char *)gci_nodes[i].node_info + (offset - p);
      }
      else if(offset < p + gci_hub_nodes[i].size) {
	/* GCI Device Area */
	DPUTS("[I/O] GCI Device: %d, Addr: 0x%08x\n", i, offset - (p + GCI_NODE_SIZE));
	return (char *)gci_nodes[i].device_area + (offset - (p + GCI_NODE_SIZE));
      }
      else {
	/* next */
	p += gci_hub_nodes[i].size;
      }
    }
  }

  return NULL;
}

void io_load(Memory addr)
{
  Memory offset, p;
  int i;

  /* word align */
  if(addr & 0x3) {
    errx(EXIT_FAILURE, "io_load invalid IO alignment.");
  }

  offset = addr - IOSR;

  if(offset < DPS_UTIM64_SIZE) {
    /* UTIM64 */
    dps_utim64_read(addr, offset);
  }
  else if(offset == DPS_SCIRXD) {
    /* SCI RXD */
    dps_sci_rxd_read(addr, offset);
  }
  else if(offset == DPS_LSFLAGS) {
    /* LSFLAGS */
    dps_lsflags_read(addr, offset);
  }
  else if(offset > DPS_SIZE + GCI_HUB_SIZE) {
    /* GCI Area */
    p = DPS_SIZE + GCI_HUB_SIZE;

    for(i = 0; i < GCI_NODE_MAX; i++) {
      if(offset < p + GCI_NODE_SIZE) {
	/* GCI Node Info */
	if(offset == p + GCI_NODE_IRF_OFFSET) {
	  /* GCND_IRF (interrupt factor) */
	  gci_nodes[i].int_issued = false;
	  DEBUGIO("[I/O] GCI Interrupt ACK %d\n", i);
	}
	break;
      }
      else if(offset < p + gci_hub_nodes[i].size) {
	p += GCI_NODE_SIZE;

	switch(i) {
	case GCI_KMC_NUM:
	  gci_kmc_read(addr, offset - p, gci_nodes[i].device_area);
	  break;
	case GCI_MMCC_NUM:
	  gci_mmcc_read(addr, offset - p, gci_nodes[i].device_area);
	  break;
	default:
	  break;
	}

	break;
      }
      else {
	/* next */
	p += gci_hub_nodes[i].size;
      }
    }
  }
}

void io_store(Memory addr)
{
  Memory offset, p;
  int i;

  /* word align */
  if(addr & 0x3) {
    errx(EXIT_FAILURE, "io_store invalid IO alignment.");
  }

  offset = addr - IOSR;

  if(offset < DPS_UTIM64_SIZE) {
    /* UTIM64 */
    dps_utim64_write(addr, offset);
  }
  else if(offset == DPS_SCITXD) {
    /* SCI TXD */
    dps_sci_txd_write(addr, offset);
  }
  else if(offset == DPS_SCICFG) {
    /* SCI CFG */
    dps_sci_cfg_write(addr, offset);
  }
  else if(offset > DPS_SIZE + GCI_HUB_SIZE) {
    /* GCI Area */
    p = DPS_SIZE + GCI_HUB_SIZE; 

    for(i = 0; i < GCI_NODE_MAX; i++) {
      if(offset < p + GCI_NODE_SIZE) {
	/* Nothing to do if GCI Node Info */
	break;
      }
      else if(offset < p + gci_hub_nodes[i].size) {
	p += GCI_NODE_SIZE;

	switch(i) {
	case GCI_DISPLAY_NUM:
	  /* DISPLAY */
	  gci_display_write(addr, offset - p, gci_nodes[i].device_area);
	  break;
	case GCI_MMCC_NUM:
	  gci_mmcc_write(addr, offset - p, gci_nodes[i].device_area);
	  break;
	default:
	  break;
	}

	break;
      }
      else {
	/* next */
	p += gci_hub_nodes[i].size;
      }
    }
  }
}
