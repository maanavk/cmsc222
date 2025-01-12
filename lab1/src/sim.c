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

int AddWithCarry(uint64_t operand1, uint64_t operand2, uint8_t carry_bit, uint8_t* nzcv_mask){
  //Adds and returns flags. Flags returned as side effect. Function defined on page 6065
  uint64_t u_sum = operand1 + operand2 + carry_bit;
  uint64_t result = u_sum;
  *nzcv_mask = 0;
  *nzcv_mask |= (result >> 63 ? 0b1000 : 0b0000); //N flag
  *nzcv_mask |= (result == 0 ? 0b0100 : 0b0000); //Z flag
  *nzcv_mask |= (UINT64_MAX - operand1 < operand2 + carry_bit ? 0b0010 : 0b0000); //C flag
  *nzcv_mask |= (INT64_MAX - (int64_t)operand1 < (int64_t)operand2 + carry_bit && (int64_t)operand1 >= 0 ||
                  INT64_MIN - (int64_t)operand1 > (int64_t)operand2 + carry_bit && (int64_t)operand1 < 0? 0b0001 : 0b0000); //V flag
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

uint64_t SignExtend(uint64_t input, uint8_t numbits){
  if (input >> (numbits - 1) == 1){
    return input | (UINT64_MAX << numbits);
  }
  else {
    return input;
  }
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
  uint64_t imm = (currinstr & 0x00FFFFE0) >> 5;
  imm = 4 * SignExtend(imm, 19);
  int test_idx = (currinstr & 0x0000001F);
  if (NEXT_STATE.REGS[test_idx] != 0) CURRENT_STATE.PC += imm - 4;
}

void branch_exn(){
  uint8_t decode_field = (currinstr & (0xE0000000)) >> 29;
  switch (decode_field & 0b111){
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
          }
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


void data_procr(){
  uint8_t decode_field_1 = (currinstr & (0x40000000)) >> 30;
  uint8_t decode_field_2 = (currinstr & (0x10000000)) >> 28;
  uint8_t decode_field_3 = (currinstr & (0x01E00000)) >> 21;
  switch (decode_field_2){
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
          break;
      }
      break;
  }
}

void execute()
{
  curr_func();
  NEXT_STATE.PC = CURRENT_STATE.PC + 4;
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
