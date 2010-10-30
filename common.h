struct instruction_base {
  unsigned int payload : 21;
  unsigned int opcode  : 10;
  unsigned int _extend : 1;
};

struct instruction_i5 {
  unsigned int immediate    : 5;
  unsigned int operand      : 5;
  unsigned int _reserve     : 6;
  unsigned int condition    : 4;
  unsigned int is_immediate : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_i11 {
  unsigned int immediate2   : 5;
  unsigned int operand      : 5;
  unsigned int immediate1   : 6;
  unsigned int condition    : 4;
  unsigned int is_immediate : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_i16 {
  unsigned int immediate2   : 5;
  unsigned int operand      : 5;
  unsigned int immediate1   : 11;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_o2 {
  unsigned int operand2     : 5;
  unsigned int operand1     : 5;
  unsigned int _reserve     : 6;
  unsigned int condition    : 4;
  unsigned int is_immediate : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_o1 {
  unsigned int ___reserve   : 5;
  unsigned int operand1     : 5;
  unsigned int _reserve     : 6;
  unsigned int condition    : 4;
  unsigned int __reserve    : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_c {
  unsigned int _reserve     : 16;
  unsigned int condition    : 4;
  unsigned int __reserve    : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_ji {
  unsigned int immediate2   : 5;
  unsigned int operand      : 5;
  unsigned int immediate1   : 6;
  unsigned int condition    : 4;
  unsigned int is_immediate : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct instruction_jo {
  unsigned int ___reserve   : 5;
  unsigned int operand1     : 5;
  unsigned int _reserve     : 6;
  unsigned int condition    : 4;
  unsigned int __reserve    : 1;
  unsigned int opcode       : 10;
  unsigned int _extend      : 1;
};

struct FLAGS {
  unsigned int          : 27;
  unsigned int sign     : 1;
  unsigned int overflow : 1;
  unsigned int carry    : 1;
  unsigned int parity   : 1;
  unsigned int zero     : 1;
};

union instruction {
  unsigned int value : 32;
  struct instruction_base base;
  struct instruction_i5   i5;
  struct instruction_i11  i11;
  struct instruction_i16  i16;
  struct instruction_o2   o2;
  struct instruction_o1   o1;
  struct instruction_c    c;
  struct instruction_ji   ji;
  struct instruction_jo   jo;
};

union memory_p {
  unsigned char *byte;
  unsigned int *word;
};

typedef union instruction Instruction;
typedef void (*pOpcodeFunc) (Instruction *);
typedef pOpcodeFunc* OpcodeTable;

extern int gr[32];
extern unsigned int ip;
extern struct FLAGS flags;

extern union memory_p mem;
extern union memory_p sp;

OpcodeTable opcode_table_init(void);

unsigned int immediate_i11(Instruction *inst);
unsigned int immediate_i16(Instruction *inst);

void ops_o2_i5(Instruction *inst, int **op1, int *op2);
void ops_o2_i11(Instruction *inst, int **op1, int *op2);

int src_o2_i11(Instruction *inst);
int src_o1_i11(Instruction *inst);

int check_condition(Instruction *inst);

void clr_flags(void);
void set_flags(int value);
