#include <stdbool.h>

#define DEBUG_REG 0
#define DEBUG_STACK 0
#define DEBUG_MON 1
#define DEBUG_INT 1
#define DEBUG_IO 1
#define DEBUG_DPS 0
#define DEBUG_MMU 1

#define DPUTS if(DEBUG || step_by_step) printf
#define DEBUGLD if(DEBUG_LD || step_by_step) printf
#define DEBUGST if(DEBUG_ST || step_by_step) printf
#define DEBUGLDHW if(DEBUG_HW) printf
#define DEBUGSTHW if(DEBUG_HW) printf
#define DEBUGJMP if(DEBUG_JMP || step_by_step) printf
#define DEBUGMON if(DEBUG_MON || step_by_step) printf
#define DEBUGINT if(DEBUG_INT || step_by_step) printf
#define DEBUGIO if(DEBUG_IO || step_by_step) printf
#define DEBUGMMU if(DEBUG_MMU || step_by_step) printf

#define MONITOR_RECV_INTERVAL_MASK (0x1000 - 1)

/* Memory Size */
#define MEMORY_MAX_ADDR 0x04000000

#define EM_MIST32 0x1032
#define STACK_DEFAULT MEMORY_MAX_ADDR
#define OPCODE_MAX 1024

#define msb(word) (!!((word) & 0x80000000))
#define NOT(reg) (reg = ~reg)

/* reg */
#define PSR_MMUMOD_MASK 0x3
#define PSR_MMUMOD (PSR & PSR_MMUMOD_MASK)
#define PSR_MMUMOD_DIRECT 0x0
#define PSR_MMUMOD_L1 0x1
#define PSR_MMUMOD_L2 0x2
#define PSR_IM_ENABLE 0x4
#define PSR_CMOD_KERNEL 0x00
#define PSR_CMOD_USER 0x60
#define PSR_MMUPS_MASK 0x380
#define PSR_MMUPS ((PSR & PSR_MMUPS_MASK) >> 7)
#define PSR_MMUPS_4KB 0x1

struct FLAGS {
  unsigned int          : 27;
  unsigned int sign     : 1;
  unsigned int overflow : 1;
  unsigned int carry    : 1;
  unsigned int parity   : 1;
  unsigned int zero     : 1;
};

/* Instruction format */
#include "instruction_format.h"

typedef unsigned int Memory;

/* Function pointer void *pOpcodeFunc(Instruction *) */
typedef void (*pOpcodeFunc) (Instruction *);
typedef pOpcodeFunc* OpcodeTable;

/* Debug flags */
extern bool DEBUG, DEBUG_LD, DEBUG_ST, DEBUG_JMP, DEBUG_HW;
extern bool MONITOR;
extern bool step_by_step, exec_finish;

/* Break points */
extern unsigned int breakp[100];
extern unsigned int breakp_next;

/* Vritual Memory */
extern Memory vmem;

/* General Register */
extern int GR[32];

/* System Register */
extern struct FLAGS FLAGR;
extern Memory PCR, next_PCR;
extern Memory SPR;
extern unsigned int PSR;
extern Memory IOSR;
extern Memory PDTR;
extern Memory IDTR;
extern unsigned long long FRCR;

/* opcode */
OpcodeTable opcode_table_init(void);

/* utils */
void print_instruction(Instruction *inst);
void print_registers(void);
void print_stack(Memory sp);

/* simulator */
int exec(Memory entry);

/* FIXME: include */
#include "memory.h"
#include "io.h"
