#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

#include "common.h"
#include "interrupt.h"
#include "monitor.h"

Memory mem;

/* General Register */
int GR[32];

/* System Register */
FLAGS FLAGR;
Memory PCR, next_PCR;
Memory SPR, KSPR, USPR;
unsigned int PSR;
Memory IOSR;
Memory PDTR;
Memory IDTR;
unsigned int TIDR;
unsigned long long FRCR;
unsigned int FI0R, FI1R;

Memory traceback[TRACEBACK_MAX];
unsigned int traceback_next;

bool step_by_step = false;
bool exec_finish = false;

void signal_on_sigint(int signo)
{
  /* EXIT */
  exec_finish = true;

  if(MONITOR) {
    monitor_disconnect();
  }

  fprintf(stderr, "[System] Keyboard Interrupt.\n");
}

int exec(Memory entry_p)
{
  OpcodeTable opcode_t;

  Instruction insn;
  Memory phypc;
  unsigned long clk = 0;

  unsigned int cmod;
  int memfd;

  unsigned int i;
  char c;

  if(signal(SIGINT, signal_on_sigint) == SIG_ERR) {
    err(EXIT_FAILURE, "signal SIGINT");
  }

  /*
  for(i = 0; i < breakp_next; i++) {
    printf("Break point[%d]: 0x%08x\n", i, breakp[i]);
  }
  */

  /* initialize internal variable */
  traceback_next = 0;
  memory_is_fault = 0;
  memory_io_writeback = 0;

  /* opcode table init */
  opcode_t = opcode_table_init();

  /* setup system registers */
  PSR = 0;
  PCR = entry_p;
  KSPR = (Memory)STACK_DEFAULT;

  printf("Execution Start: entry = 0x%08x\n", PCR);

  do {
    next_PCR = ~0;

    /* choose stack */
    cmod = (PSR & PSR_CMOD_MASK);
    SPR = cmod ? USPR : KSPR;

    /* break point check */
    for(i = 0; i < breakp_next; i++) {
      if(PCR == breakp[i]) {
	step_by_step = true;
	break;
      }
    }

    /* instruction fetch */
    phypc = memory_addr_virt2phy(PCR, false);

    if(memory_is_fault) {
      /* fault fetch */
      DEBUGINT("[FAULT] Instruction fetch: %08x\n", PCR);
      goto fault;
    }

#if CACHE_L1_I_ENABLE
    insn.value = memory_cache_l1i_read(phypc);
#else
    insn.value = *(unsigned int *)memory_addr_phy2vm(phypc, false);
#endif

    /* decode */
    if(opcode_t[insn.base.opcode] == NULL) {
      print_instruction(insn);
      abort_sim();
      errx(EXIT_FAILURE, "invalid opcode. (pc:%08x op:%x)", PCR, insn.base.opcode);
    }

    if(DEBUG || step_by_step) {
      puts("---");
      print_instruction(insn);
    }

    /* execution */
    (*(opcode_t[insn.base.opcode]))(insn);

  fault:
    if(memory_is_fault) {
      /* faulting memory access */
      interrupt_dispatch_nonmask(memory_is_fault);
      next_PCR = PCR;

      memory_io_writeback = 0;
      memory_is_fault = 0;
    }
    else if(memory_io_writeback) {
      /* sync io */
      io_store(memory_io_writeback);
      memory_io_writeback = 0;
    }

    /* writeback SP */
    if(cmod) {
      USPR = SPR;
    }
    else {
      KSPR = SPR;
    }

    if(step_by_step) {
      print_registers();
      print_traceback();
      // print_stack(SPR);
      // dps_info();

      printf("> ");

      while((c = getchar()) == -1);

      if(c == 'c') {
	step_by_step = false;
      }
      else if(c == 'q') {
	exec_finish = true;
      }
      else if(c == 'm') {
	memfd = open("memory.dump", O_WRONLY | O_CREAT, S_IRWXU);
	write(memfd, memory_addr_phy2vm(memory_addr_virt2phy(0, false), 0x1000), false);
	close(memfd);
      }
    }
    else {
      if(DEBUG_REG) { print_registers(); }
      if(DEBUG_TRACE) { print_traceback(); }
      if(DEBUG_STACK) { print_stack(SPR); }
      if(DEBUG_DPS) { dps_info(); }
    }

    if(!(clk & MONITOR_RECV_INTERVAL_MASK)) {
      if((PSR & PSR_IM_ENABLE) && IDT_ISENABLE(IDT_DPS_LS_NUM)) {
	dps_sci_recv();
      }

      if(MONITOR) {
	monitor_method_recv();
      }
    }

    /* next */
    if(next_PCR != ~0) {
      /* alignment check */
      if(next_PCR & 0x3) {
	abort_sim();
	errx(EXIT_FAILURE, "invalid branch addres. %08x", next_PCR);
      }

      PCR = next_PCR;
    }
    else {
      PCR += 4;
    }

    interrupt_dispatcher();
    clk++;

  } while(!(PCR == 0 && GR[31] == 0 && DEBUG_EXIT_B0) && !exec_finish);
  /* DEBUG_EXIT_B0: exit if b rret && rret == 0 */

  puts("---- Program Terminated ----");
  print_instruction(insn);
  print_registers();

  free(opcode_t);

  return 0;
}
