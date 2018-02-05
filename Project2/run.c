/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc)
{
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(){
    int instr_index;
    instruction * inst = get_inst_info(CURRENT_STATE.PC);
    CURRENT_STATE.PC += 4;
    switch(OPCODE(inst)){
        case 0x00:
            switch(FUNC(inst)){
                case 0b000000: // SLL
                    CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.REGS[RT(inst)] << SHAMT(inst);
                    break;
                case 0b000010: // SRL
                    CURRENT_STATE.REGS[RD(inst)] = LRS(CURRENT_STATE.REGS[RT(inst)],SHAMT(inst));
                    break;
                case 0b001000: // JR
                    CURRENT_STATE.PC = CURRENT_STATE.REGS[RS(inst)];
                    break;
                case 0b100001: // ADDU
                    CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.REGS[RS(inst)] + CURRENT_STATE.REGS[RT(inst)];
                    break;
                case 0b100011: // SUBU
                    CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.REGS[RS(inst)] - CURRENT_STATE.REGS[RT(inst)];
                    break;
                case 0b100100: // AND
                    CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.REGS[RS(inst)] & CURRENT_STATE.REGS[RT(inst)];
                    break;
                case 0b100101: // OR
                    CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.REGS[RS(inst)] | CURRENT_STATE.REGS[RT(inst)];
                    break;
                case 0b100111: // NOR
                    CURRENT_STATE.REGS[RD(inst)] = ~(CURRENT_STATE.REGS[RS(inst)] | CURRENT_STATE.REGS[RT(inst)]);
                    break;
                case 0b101011: // SLTU
                    if(CURRENT_STATE.REGS[RS(inst)] < CURRENT_STATE.REGS[RT(inst)]){
                        CURRENT_STATE.REGS[RD(inst)] = 1;
                    }
                    else{
                        CURRENT_STATE.REGS[RD(inst)] = 0;
                    }
                    break;
                default:
                    CURRENT_STATE.PC -= 4;
                    break;
            }
            break;
        case 0b000010: // J
            JUMP_INST((CURRENT_STATE.PC & 0xF0000000) | (TARGET(inst) << 2));
            break;
        case 0b000011: // JAL
            CURRENT_STATE.REGS[31] = CURRENT_STATE.PC + 4;
            JUMP_INST((CURRENT_STATE.PC & 0xF0000000) | (TARGET(inst) << 2));
            break;
        case 0b000100: // BEQ
            BRANCH_INST((CURRENT_STATE.REGS[RS(inst)] == CURRENT_STATE.REGS[RT(inst)]), CURRENT_STATE.PC + (IOFFSET(inst) << 2), 0);
            break;
        case 0b000101: // BNE
            BRANCH_INST((CURRENT_STATE.REGS[RS(inst)] != CURRENT_STATE.REGS[RT(inst)]), CURRENT_STATE.PC + (IOFFSET(inst) << 2), 0);
            break;
        case 0b001001: // ADDIU
            CURRENT_STATE.REGS[RT(inst)] = CURRENT_STATE.REGS[RS(inst)] + IMM(inst);
            break;
        case 0b001011: // SLTIU
            if(CURRENT_STATE.REGS[RS(inst)] < SIGN_EX(IMM(inst))){
                CURRENT_STATE.REGS[RT(inst)] = 1;
            }
            else{
                CURRENT_STATE.REGS[RT(inst)] = 0;
            }
            break;
        case 0b001100: // ANDI
            CURRENT_STATE.REGS[RT(inst)] = CURRENT_STATE.REGS[RS(inst)] & IMM(inst);
            break;
        case 0b001101: // ORI
            CURRENT_STATE.REGS[RT(inst)] = CURRENT_STATE.REGS[RS(inst)] | IMM(inst);
            break;
        case 0b001111: // LUI
            CURRENT_STATE.REGS[RT(inst)] = IMM(inst) << 16;
            break;
        case 0b100011: // LW
            CURRENT_STATE.REGS[RT(inst)] = mem_read_32(CURRENT_STATE.REGS[RS(inst)] + IOFFSET(inst));
            break;
        case 0b101011: // SW
            mem_write_32(CURRENT_STATE.REGS[RS(inst)] + IOFFSET(inst), CURRENT_STATE.REGS[RT(inst)]);
            break;
        default:
            CURRENT_STATE.PC -= 4;
            break;
    }
    instr_index = (CURRENT_STATE.PC - MEM_TEXT_START) / 4;
    if (instr_index >= NUM_INST) {
         RUN_BIT = FALSE;
    }
}
