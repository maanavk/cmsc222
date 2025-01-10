#include <stdio.h>
#include "shell.h"

/*
defining globals
*/

uint32_t currinstr;

void fetch()
{
  currinstr = mem_read_32(CURRENT_STATE.PC);
}

void decode()
{

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
