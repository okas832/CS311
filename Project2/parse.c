/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "parse.h"

int text_size;
int data_size;
instruction parsing_instr(const char *buffer, const int index)
{
    instruction *instr = (instruction *)malloc(sizeof(instruction)); // == struct inst_s instr;
    char * tmp;
    char * buf;
    instr->value = index;
    buf = (char *)buffer;
    tmp = strndup(buf , 6);
    buf += 6;
    instr->opcode = fromBinary(tmp);
    switch(instr->opcode)
    {
        case 0x9:
        case 0xc:
        case 0xf:
        case 0xd:
        case 0xb:
        case 0x23:
        case 0x2b:
        case 0x4:
        case 0x5:
            tmp = strndup(buf, 5);
            buf += 5;
            instr->r_t.r_i.rs = fromBinary(tmp);
            tmp = strndup(buf, 5);
            buf += 5;
            instr->r_t.r_i.rt = fromBinary(tmp);
            tmp = strndup(buf, 16);
            buf += 16;
            instr->r_t.r_i.r_i.imm = fromBinary(tmp);
            break;
        case 0x0:
            tmp = strndup(buf, 5);
            buf += 5;
            instr->r_t.r_i.rs = fromBinary(tmp);
            tmp = strndup(buf, 5);
            buf += 5;
            instr->r_t.r_i.rt = fromBinary(tmp);
            tmp = strndup(buf, 5);
            buf += 5;
            instr->r_t.r_i.r_i.r.rd = fromBinary(tmp);
            tmp = strndup(buf, 5);
            buf += 5;
            instr->r_t.r_i.r_i.r.shamt = fromBinary(tmp);
            tmp = strndup(buf, 6);
            buf += 6;
            instr->func_code = fromBinary(tmp);
            break;
        case 0x2:
        case 0x3:
            tmp = strndup(buf,26);
            instr->r_t.target = fromBinary(tmp);
            break;
        default:
            printf("Not available instruction\n");
            assert(0);
    }
    return *instr;
}

//parsing_data: write the data value in the virtual memory.
void parsing_data(const char *buffer, const int index)
{
    uint32_t addr = 0x10000000 + index;
    mem_write_32(addr, fromBinary((char *)buffer));
}

void print_parse_result()
{
    int i;
    printf("Instruction Information\n");

    for(i = 0; i < text_size/4; i++)
    {
	printf("INST_INFO[%d].value : %x\n",i, INST_INFO[i].value);
	printf("INST_INFO[%d].opcode : %d\n",i, INST_INFO[i].opcode);

	switch(INST_INFO[i].opcode)
	{
	    //Type I
	    case 0x9:		//(0x001001)ADDIU
	    case 0xc:		//(0x001100)ANDI
	    case 0xf:		//(0x001111)LUI	
	    case 0xd:		//(0x001101)ORI
	    case 0xb:		//(0x001011)SLTIU
	    case 0x23:		//(0x100011)LW	
	    case 0x2b:		//(0x101011)SW
	    case 0x4:		//(0x000100)BEQ
	    case 0x5:		//(0x000101)BNE
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].imm : %d\n",i, INST_INFO[i].r_t.r_i.r_i.imm);
		break;

    	    //TYPE R
	    case 0x0:		//(0x000000)ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU  if JR
		printf("INST_INFO[%d].func_code : %d\n",i, INST_INFO[i].func_code);
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].rd : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.rd);
		printf("INST_INFO[%d].shamt : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.shamt);
		break;

    	    //TYPE J
	    case 0x2:		//(0x000010)J
	    case 0x3:		//(0x000011)JAL
		printf("INST_INFO[%d].target : %d\n",i, INST_INFO[i].r_t.target);
		break;

	    default:
		printf("Not available instruction\n");
		assert(0);
	}
    }

    printf("Memory Dump - Text Segment\n");
    for(i = 0; i < text_size; i+=4)
	printf("text_seg[%d] : %x\n", i, mem_read_32(MEM_TEXT_START + i));
    for(i = 0; i < data_size; i+=4)
	printf("data_seg[%d] : %x\n", i, mem_read_32(MEM_DATA_START + i));
    printf("Current PC: %x\n", CURRENT_STATE.PC);
}
