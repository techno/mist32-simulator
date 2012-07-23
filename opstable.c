#include <stdlib.h>
#include "common.h"
#include "instructions.h"

OpcodeTable opcode_table_init(void)
{
  OpcodeTable opcode_t;
  opcode_t = (OpcodeTable)calloc(1024, sizeof(pOpcodeFunc));
  
  opcode_t[0] = i_add;
  opcode_t[1] = i_sub;
  opcode_t[2] = i_mull;
  opcode_t[3] = i_mulh;
  opcode_t[4] = i_udiv;
  opcode_t[5] = i_umod;
  opcode_t[6] = i_cmp;
  opcode_t[7] = i_div;
  opcode_t[8] = i_mod;
  opcode_t[9] = i_neg;

  opcode_t[14] = i_addc;
  opcode_t[16] = i_inc;
  opcode_t[17] = i_dec;
  opcode_t[28] = i_sext8;
  opcode_t[29] = i_sext16;
  
  opcode_t[64] = i_shl;
  opcode_t[65] = i_shr;  
  opcode_t[69] = i_sar;
  opcode_t[72] = i_rol;
  opcode_t[73] = i_ror;

  opcode_t[96] = i_and;
  opcode_t[97] = i_or;
  opcode_t[98] = i_not;
  opcode_t[99] = i_xor;
  opcode_t[100] = i_nand;
  opcode_t[101] = i_nor;
  opcode_t[102] = i_xnor;
  opcode_t[103] = i_test;

  opcode_t[106] = i_wl16;
  opcode_t[107] = i_wh16;
  opcode_t[108] = i_clrb;
  opcode_t[109] = i_setb;
  opcode_t[110] = i_clr;
  opcode_t[111] = i_set;
  opcode_t[112] = i_revb;
  opcode_t[113] = i_rev8;
  opcode_t[114] = i_getb;
  opcode_t[115] = i_get8;
  
  opcode_t[118] = i_lil;
  opcode_t[119] = i_lih;
  opcode_t[122] = i_ulil;

  opcode_t[128] = i_ld8;
  opcode_t[129] = i_ld16;
  opcode_t[130] = i_ld32;
  opcode_t[131] = i_st8;
  opcode_t[132] = i_st16;
  opcode_t[133] = i_st32;
  
  opcode_t[136] = i_push;
  opcode_t[137] = i_pushpc;
  opcode_t[144] = i_pop;
  
  opcode_t[160] = i_bur;
  opcode_t[161] = i_br;
  opcode_t[162] = i_b;
  opcode_t[163] = i_ib;
  
  opcode_t[176] = i_bur; /* burn */
  opcode_t[177] = i_br;  /* brn  */
  opcode_t[178] = i_b;   /* bn   */

  opcode_t[192] = i_srspr;
  opcode_t[203] = i_sriosr;
  opcode_t[224] = i_srspw;

  opcode_t[256] = i_nop;
  opcode_t[257] = i_halt;
  opcode_t[258] = i_move;
  opcode_t[259] = i_movepc;
  
  return opcode_t;
}
