#include <stdio.h>
#include "shell.h"

/*
defining globals
*/

uint32_t currinstr;
void (*curr_func)(void); //can change this to a queue if needed for parallelism


void fetch()
{
  currinstr = mem_read_32(CURRENT_STATE.PC);
}

op_ADDI(){

}

data_proci(){
  //extract bits 25-23
  uint8_t decode_field = (currinstr & 0x01C00000) >> 20;
  switch (decode_field & 0b111){
    case (0b000):
      //PC-rel addressing
      break;
    case (0b010):
    case (0b011):
      //Add/Sub Immediate
      switch ((currinstr & 0xE0000000) >> 29){
        case (0b000):
          op_ADDI();
          break;
        case (0b001):
          op_ADDS();
          break;
        
      }
      break;
    case (0b100):
      //Logic Immediate
      //IMPLEMENT ME
      break;
    case (0b101):
      //Move Wide Immediate
      break;
    case (0b110):
      //Bitfield
      break;
    case (0b111):
      //extract
      break;
  }
}

void decode()
{
  //first, extract bits 28 - 25
  uint8_t decode_field = (currinstr & (0x1E000000)) >> 24;
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
