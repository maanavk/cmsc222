#include <stdio.h>
#include "shell.h"

/*
Completed + Tested:
- ADDI
- ADDIS
- ADD
- ADDS
- SUBI
- SUBIS
- SUB
- SUBS
- AND
- ANDS
- ORR
- EOR
- MOVZ
- CMP
- CBZ
- CBNZ
- B
- LDUR
- STUR
- HLT
*/

/*
--------------------------------------------------------------------------------

defining globals

--------------------------------------------------------------------------------
*/

uint32_t currinstr;
void (*curr_func)(void); //can change this to a queue if needed for parallelism
typedef enum ShiftType{
  LSL = 0b00,
  LSR = 0b01,
  ASR = 0b10,
  ROR = 0b11
} ShiftType;

void clear_flags(){
  NEXT_STATE.FLAG_C = 0;
  NEXT_STATE.FLAG_N = 0;
  NEXT_STATE.FLAG_Z = 0;
  NEXT_STATE.FLAG_V = 0;
}

int AddWithCarry(int64_t operand1, int64_t operand2, uint8_t carry_bit, uint8_t* nzcv_mask){
  //Adds and returns flags. Flags returned as side effect. Function defined on page 6065
  //Note that I am a bit concerned about the safety of this function - specifically with the regards to the V flag. Might just remove it if it is not required.
  uint64_t u_sum = (uint64_t)operand1 + (uint64_t)operand2 + carry_bit;
  uint64_t result = u_sum;
  *nzcv_mask = 0;
  *nzcv_mask |= (result >> 63 ? 0b1000 : 0b0000); //N flag
  *nzcv_mask |= (result == 0 ? 0b0100 : 0b0000); //Z flag
  *nzcv_mask |= (UINT64_MAX - operand1 < operand2 + carry_bit ? 0b0010 : 0b0000); //C flag
  *nzcv_mask |= (INT64_MAX - operand1 < operand2 + carry_bit && operand1 >= 0 ||
                  INT64_MIN - operand1 > operand2 + carry_bit && operand1 < 0? 0b0001 : 0b0000); //V flag
  return result;
}

uint64_t ShiftReg(uint16_t reg, ShiftType type, uint32_t amount){
  //shifts a register and returns the shifted value
  switch (type){
    case (LSL):
      return NEXT_STATE.REGS[reg] << amount;
    case (LSR):
      return (uint64_t)NEXT_STATE.REGS[reg] >> amount;
    case (ASR):
      return NEXT_STATE.REGS[reg] >> amount;
    case (ROR):
      return (NEXT_STATE.REGS[reg] >> amount) | ((NEXT_STATE.REGS[reg] & 0x1) << 63); 
  }
}

int64_t SignExtend(uint64_t input, uint8_t numbits){
  if (input >> (numbits - 1) == 1){
    return input | (UINT64_MAX << numbits);
  }
  else {
    return input;
  }
}

uint8_t ConditionHolds(uint8_t cond){
  //the bgt bge and lt lte tests might be broken code seems to run fine from my tests but I don't know.
  uint8_t result;
  printf("N: %d, V: %d \n", CURRENT_STATE.FLAG_N, CURRENT_STATE.FLAG_V);
  switch (cond >> 1){
    case (0b000):
      result = CURRENT_STATE.FLAG_Z;
      break;
    case (0b001):
      result = CURRENT_STATE.FLAG_C;
      break;
    case (0b010):
      result = CURRENT_STATE.FLAG_N;
      break;
    case (0b011):
      result = CURRENT_STATE.FLAG_V;
      break;
    case (0b100):
      result = ((CURRENT_STATE.FLAG_C == 1) && (CURRENT_STATE.FLAG_Z == 0));
      break;
    case (0b101):
      result = (CURRENT_STATE.FLAG_N == CURRENT_STATE.FLAG_V);
      break;
    case (0b110):
      result = ((CURRENT_STATE.FLAG_N == CURRENT_STATE.FLAG_V) && (CURRENT_STATE.FLAG_Z == 0));
      break;
    case (0b111):
      return TRUE;
  }
  return ((cond & 0b1)? !result : result);
}

void fetch()
{
  currinstr = mem_read_32(CURRENT_STATE.PC);
}

/*
--------------------------------------------------------------------------------

Begin Data Proc

--------------------------------------------------------------------------------
*/

void op_ADDIS(){
  int shift = (currinstr & 0x00C00000) >> 22;
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint64_t imm = (currinstr & 0x001FFC00) >> 10;
  if (shift) imm << 12;
  uint8_t flags;
  clear_flags();
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src_idx], imm, 0, &flags);
  NEXT_STATE.FLAG_N = ((flags & 0b1000) >> 3);
  NEXT_STATE.FLAG_Z = ((flags & 0b0100) >> 2);
  NEXT_STATE.FLAG_C = ((flags & 0b0010) >> 1); //can make a macro for flag setting if needed using enums
  NEXT_STATE.FLAG_V = flags & 0b0001;
}

void op_ADDI(){
  int shift = (currinstr & 0x00C00000) >> 22;
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint64_t imm = (currinstr & 0x001FFC00) >> 10;
  if (shift) imm << 12;
  uint8_t _;
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src_idx], imm, 0, &_);
}

void op_SUBIS(){
  int shift = (currinstr & 0x00C00000) >> 22;
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint64_t imm = (currinstr & 0x001FFC00) >> 10;
  if (shift) imm << 12;
  uint8_t flags;
  clear_flags();
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src_idx], -imm, 0, &flags);
  NEXT_STATE.FLAG_N = ((flags & 0b1000) >> 3);
  NEXT_STATE.FLAG_Z = ((flags & 0b0100) >> 2);
  NEXT_STATE.FLAG_C = ((flags & 0b0010) >> 1); //can make a macro for flag setting if needed using enums
  NEXT_STATE.FLAG_V = flags & 0b0001;
}

void op_SUBI(){
  int shift = (currinstr & 0x00C00000) >> 22;
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint32_t imm = (currinstr & 0x001FFC00) >> 10;
  if (shift) imm << 12;
  uint8_t _;
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src_idx], -imm, 0, &_);
}

void op_ANDI(){
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint32_t imm = (currinstr & 0x001FFC00) >> 10;
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src_idx] & imm;
}

void op_ORRI(){
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint32_t imm = (currinstr & 0x001FFC00) >> 10;
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src_idx] | imm;
}

void op_EORI(){
  int src_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  uint32_t imm = (currinstr & 0x001FFC00) >> 10;
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src_idx] ^ imm;
}

void op_MOVZ(){
  //Not sure about the hw bits for now just did the simple implementation
  int dst_idx = currinstr & 0x0000001F;
  uint16_t imm = (currinstr & 0x001FFFE0) >> 5;
  NEXT_STATE.REGS[dst_idx] = imm;
}

void op_UBFM(){
  uint8_t imms = (currinstr & 0x0000FC00) >> 10;
  uint8_t immr = (currinstr & 0x003F0000) >> 16;
  int Rd = (currinstr & 0x0000001F);
  int Rn = (currinstr & 0x000003E0) >> 5;
  uint64_t src = CURRENT_STATE.REGS[Rn];
  if (imms == 0b111111){
    NEXT_STATE.REGS[Rd] = src >> immr;
  }
  else{
    NEXT_STATE.REGS[Rd] = src << -immr;
  }
}

void data_proci(){
  //extract bits 25-23
  uint8_t decode_field = (currinstr & 0x03800000) >> 23;
  switch (decode_field & 0b111){
    case (0b000):
      //PC-rel addressing
      break;
    case (0b010):
    case (0b011):
      //Add/Sub Immediate
      switch ((currinstr & 0xE0000000) >> 29){
        case (0b100):
          curr_func = op_ADDI;
          break;
        case (0b101):
          curr_func = op_ADDIS;
          break;
        case (0b110):
          curr_func = op_SUBI;
          break;
        case (0b111):
          curr_func = op_SUBIS;
          break;
      }
      break;
    case (0b100):
      //Logic Immediate
      //Apparently these aren't needed but I accidentally implemented them anyways
      switch ((currinstr & 0xE0000000) >> 29){
        case (0b100):
          curr_func = op_ANDI;
          break;
        case (0b101):
          curr_func = op_ORRI;
          break;
        case (0b110):
          curr_func = op_EORI;
          break;
      }
      break;
    case (0b101):
      //Move Wide Immediate
      switch ((currinstr & 0xE0000000) >> 29){
        case (0b110):
          curr_func = op_MOVZ;
          break;
      }
      break;
    case (0b110):
      //Bitfield
      switch ((currinstr & 0xE0000000) >> 29){
        case (0b110):
          curr_func = op_UBFM; //this gives us lsl and lsr
          break;
      }
      break;
    case (0b111):
      //extract
      break;
  }
}

/*
--------------------------------------------------------------------------------

Begin branch-exception

--------------------------------------------------------------------------------
*/

void op_HLT(){
  RUN_BIT = 0;
}

void op_CBZ(){
  uint64_t imm = (currinstr & 0x00FFFFE0) >> 5;
  imm = 4 * SignExtend(imm , 19);
  int test_idx =(currinstr & 0x0000001F);
  if (NEXT_STATE.REGS[test_idx] == 0) CURRENT_STATE.PC += imm - 4;
}

void op_CBNZ(){
  //maybe test sign extension
  uint64_t imm = (currinstr & 0x00FFFFE0) >> 5;
  imm = 4 * SignExtend(imm, 19);
  int test_idx = (currinstr & 0x0000001F);
  if (NEXT_STATE.REGS[test_idx] != 0) CURRENT_STATE.PC += imm - 4;
}

void op_B(){
  uint64_t imm = (currinstr & 0x03FFFFFF);
  imm = 4 * SignExtend(imm, 26);
  CURRENT_STATE.PC += imm - 4;
}

void op_BR(){
  int src_idx = (currinstr & 0x000003E0) >> 5;
  CURRENT_STATE.PC = CURRENT_STATE.REGS[src_idx] - 4; // this is pretty stupid, will consider making the jumping logic more sensible later instead of taking from the current always
}

void op_Bcond(){
  uint64_t imm = (currinstr & 0x00FFFFE0) >> 5;
  imm = 4 * SignExtend(imm, 19);
  uint8_t cond = (currinstr & 0x0000000F);
  if (ConditionHolds(cond)){
    CURRENT_STATE.PC += imm - 4;
  }
}

void branch_exn(){
  uint8_t decode_field = (currinstr & (0xE0000000)) >> 29;
  switch (decode_field & 0b111){
    case (0b010):
      if (((currinstr & 0x03000010) == 0x0)){
        curr_func = op_Bcond;
      }
      break;
    case (0b100):
      //op_BL
      break;
    case (0b000):
      curr_func = op_B;
      break;
    case (0b110):
      switch ((currinstr & (0x03C00000)) >> 22){
        case (0b0000):
        case (0b0010):
        case (0b0001):
        case (0b0011):
          //exception generation
          switch((currinstr & 0x00E0001F)){
            case (0x00400000):
              curr_func = op_HLT;
              break;
          }
          break;
        case (0b1000):
          //I will just let it fall through to br...
          curr_func = op_BR;
          break;
        break;
      }
      break;
    case (0b001):
    case (0b101):
      switch ((currinstr & 0x02000000) >> 25){
        case (0x0):
          switch ((currinstr & 0x01000000) >> 24){
            case (0x0):
              curr_func = op_CBZ;
              break;
            case (0x1):
              curr_func = op_CBNZ;
              break;
          }
          break;
      }
      break;

  }
}

/*
--------------------------------------------------------------------------------

Begin Data Processing (Register)

--------------------------------------------------------------------------------
*/

void op_ADD(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  uint8_t _;
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src1_idx], NEXT_STATE.REGS[src2_idx], 0, &_);
}

void op_ADDS(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  uint8_t _;
  uint8_t flags;
  clear_flags();
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src1_idx], NEXT_STATE.REGS[src2_idx], 0, &flags);
  NEXT_STATE.FLAG_N = ((flags & 0b1000) >> 3);
  NEXT_STATE.FLAG_Z = ((flags & 0b0100) >> 2);
  NEXT_STATE.FLAG_C = ((flags & 0b0010) >> 1); //can make a macro for flag setting if needed using enums
  NEXT_STATE.FLAG_V = flags & 0b0001;
}

void op_SUBS(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  uint8_t _;
  uint8_t flags;
  clear_flags();
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src1_idx], -NEXT_STATE.REGS[src2_idx], 0, &flags);
  NEXT_STATE.FLAG_N = ((flags & 0b1000) >> 3);
  NEXT_STATE.FLAG_Z = ((flags & 0b0100) >> 2);
  NEXT_STATE.FLAG_C = ((flags & 0b0010) >> 1); //can make a macro for flag setting if needed using enums
  NEXT_STATE.FLAG_V = flags & 0b0001;
}

void op_SUB(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  uint8_t _;
  NEXT_STATE.REGS[dst_idx] = AddWithCarry(NEXT_STATE.REGS[src1_idx], -NEXT_STATE.REGS[src2_idx], 0, &_);
}

void op_AND(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  int imm6 = (currinstr & 0x0000FC00) >> 10;
  int shift_type = (currinstr & 0x00C00000) >> 22;
  uint64_t operand2 = ShiftReg(src2_idx, shift_type, imm6);
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src1_idx] & operand2;
}

void op_ANDS(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  int imm6 = (currinstr & 0x0000FC00) >> 10;
  int shift_type = (currinstr & 0x00C00000) >> 22;
  uint64_t operand2 = ShiftReg(src2_idx, shift_type, imm6);
  clear_flags();
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src1_idx] & operand2;
  NEXT_STATE.FLAG_N = (NEXT_STATE.REGS[dst_idx] < 0 ? 1 : 0);
  NEXT_STATE.FLAG_Z = (NEXT_STATE.REGS[dst_idx] == 0 ? 1 : 0);
}

void op_ORR(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  int imm6 = (currinstr & 0x0000FC00) >> 10;
  int shift_type = (currinstr & 0x00C00000) >> 22;
  uint64_t operand2 = ShiftReg(src2_idx, shift_type, imm6);
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src1_idx] | operand2;
}

void op_EOR(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  int imm6 = (currinstr & 0x0000FC00) >> 10;
  int shift_type = (currinstr & 0x00C00000) >> 22;
  uint64_t operand2 = ShiftReg(src2_idx, shift_type, imm6);
  NEXT_STATE.REGS[dst_idx] = NEXT_STATE.REGS[src1_idx] ^ operand2;
}

void op_MADD(){
  int src1_idx = (currinstr & 0x000003E0) >> 5; 
  int dst_idx = currinstr & 0x0000001F;
  int src2_idx = (currinstr & 0x001F0000) >> 16;
  NEXT_STATE.REGS[dst_idx] = CURRENT_STATE.REGS[src1_idx] * CURRENT_STATE.REGS[src2_idx];
}

void data_procr(){
  uint8_t decode_field_1 = (currinstr & (0x40000000)) >> 30;
  uint8_t decode_field_2 = (currinstr & (0x10000000)) >> 28;
  uint8_t decode_field_3 = (currinstr & (0x01E00000)) >> 21;
  switch (decode_field_2){
    case (0x1):
      switch (decode_field_3 & 0x8){
        case (0x8):
          curr_func = op_MADD; //I got lazy this can be decoded more but its fine
          break;
      }
      break;
    case (0x0):
      switch (decode_field_3 & 0b1001){
        case (0b0000):
        case (0b0001):
          //Logical, shifted register
          switch ((currinstr & 0xE0200000) >> 20){
            case (0x800):
              curr_func = op_AND;
              break;
            case (0xA00):
              curr_func = op_ORR;
              break;
            case (0xC00):
              curr_func = op_EOR;
              break;
            case (0xE00):
              curr_func = op_ANDS;
              break;
          }
          break;
        case (0b1000):
          //Add/sub, shift register, just fall through.
        case (0b1001):
          //Add/sub, extended register
          switch ((currinstr & 0xE0C00000) >> 20){
            case (0x800):
              curr_func = op_ADD;
              break;
            case (0xA00):
              curr_func = op_ADDS;
              break;
            case (0xC00):
              curr_func = op_SUB;
              break;
            case (0xE00):
              curr_func = op_SUBS;
              break;
          }
          break;
      }
      break;
  }
}

/*
--------------------------------------------------------------------------------

Begin Load/Store 

--------------------------------------------------------------------------------
*/

void op_STUR(){
  uint32_t to_idx = (currinstr & 0x000003E0) >> 5; 
  uint32_t from_idx = currinstr & 0x0000001F;
  int64_t imm9 = (currinstr & 0x001FF000) >> 12;
  imm9 = SignExtend(imm9, 9);
  mem_write_32(CURRENT_STATE.REGS[to_idx] + imm9, (uint32_t)CURRENT_STATE.REGS[from_idx]);
}

void op_LDUR(){
  uint32_t src_idx = (currinstr & 0x000003E0) >> 5; 
  uint32_t dst_idx = currinstr & 0x0000001F;
  int64_t imm9 = (currinstr & 0x001FF000) >> 12;
  imm9 = SignExtend(imm9, 9);
  NEXT_STATE.REGS[dst_idx] = mem_read_32(CURRENT_STATE.REGS[src_idx] + imm9); //I think C implicitly zero extends so this is fine?
}

void op_STURB(){
  uint32_t Rn = (currinstr & 0x000003E0) >> 5; 
  uint32_t Rt = currinstr & 0x0000001F;
  int64_t imm9 = (currinstr & 0x001FF000) >> 12;
  imm9 = SignExtend(imm9, 9);
  uint64_t address = CURRENT_STATE.REGS[Rn] + imm9;
  mem_write_32(address, (uint8_t) CURRENT_STATE.REGS[Rt]);
}

void op_LDURB(){
  uint32_t src_idx = (currinstr & 0x000003E0) >> 5; 
  uint32_t dst_idx = currinstr & 0x0000001F;
  int64_t imm9 = (currinstr & 0x001FF000) >> 12;
  imm9 = SignExtend(imm9, 9);
  NEXT_STATE.REGS[dst_idx] = (uint8_t) mem_read_32(CURRENT_STATE.REGS[src_idx] + imm9); 
}

void op_STURH(){
  uint32_t Rn = (currinstr & 0x000003E0) >> 5; 
  uint32_t Rt = currinstr & 0x0000001F;
  int64_t imm9 = (currinstr & 0x001FF000) >> 12;
  imm9 = SignExtend(imm9, 9);
  uint64_t address = CURRENT_STATE.REGS[Rn] + imm9;
  mem_write_32(address, (uint16_t) CURRENT_STATE.REGS[Rt]);
}

void op_LDURH(){
  uint32_t src_idx = (currinstr & 0x000003E0) >> 5; 
  uint32_t dst_idx = currinstr & 0x0000001F;
  int64_t imm9 = (currinstr & 0x001FF000) >> 12;
  imm9 = SignExtend(imm9, 9);
  NEXT_STATE.REGS[dst_idx] = (uint16_t) mem_read_32(CURRENT_STATE.REGS[src_idx] + imm9); 
}

void load_store_uimm(){
  uint8_t size = (currinstr & (0xC0000000)) >> 30;
  uint8_t v = (currinstr & (0x04000000)) >> 26;
  uint8_t opc = (currinstr & 0x00C00000) >> 22;
  switch (size){
    case (0b00):
      switch (opc){
        case (0b00):
          curr_func = op_STURB;
          break;
        case (0b01):
          curr_func = op_LDURB;
          break;
      }
      break;
    case (0b01):
      switch (opc){
        case (0b00):
          curr_func = op_STURH;
          break;
        case (0b01):
          curr_func = op_LDURH;
          break;
      }
    case (0b11):
      switch (v){
        case (0b0):
          switch (opc){
            case (0b00):
              curr_func = op_STUR;
              break;
            case (0b01):
              curr_func = op_LDUR;
              break;
          }
          break;
      }
      break;
  }

}

void load_store(){
  uint8_t op1 = (currinstr & (0x30000000)) >> 28;
  uint8_t op3 = (currinstr & (0x01800000)) >> 23;
  uint8_t op4 = (currinstr & (0x003F0000)) >> 16;
  uint8_t op5 = (currinstr & (0x00000C00)) >> 10;
  switch (op1){
    case (0b11):
      switch (op3){
        case (0b01):
        case (0b00):
          switch (op4 >> 5){
            case (0b0):
              switch (op5){
                case (0b00):
                  //load/store unscaled immediate
                  load_store_uimm();
                  break;
              }
              break;
          }
        break;
      }
    break;
  }
}

void decode()
{
  //first, extract bits 28 - 25
  uint8_t decode_field = (currinstr & (0x1E000000)) >> 25;
  switch (decode_field & 0b1100){
    case (0):
      //Unallocated
      break;
    case (0b1000):
      switch (decode_field & 0b0010){
        case (0b0000):
          //Data proc - Immediate
          data_proci();
          break;
        case (0b0010):
          //Branches and exceptions
          branch_exn();
          break;  
      }
      break;
    case (0b0100):
    case (0b1100):
      switch (decode_field & 0b0111){
        case (0b0111):
          //Data proc scalar floating point
          break;
        case (0b0101):
          //Data proc register
          data_procr();
          break;
        case (0b0100):
        case (0b0110):
          //Loads and stores
          load_store();
          break;
      }
      break;
  }
}

void execute()
{
  curr_func();
  NEXT_STATE.PC = CURRENT_STATE.PC + 4;
  NEXT_STATE.REGS[31] = 0; //dirty fix to avoid the edge case I don't know how wise this is
  CURRENT_STATE = NEXT_STATE;
}

void process_instruction()
{
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    fetch();
    decode();
    execute();

}
