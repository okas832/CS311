#include<stdio.h>

#include "run.h"
#include "util.h"
#include "cache.h"

int lsr(int x, int n)
{
  return (int)((unsigned int)x >> n);
}

void stall(CPU_State *nstate)
{
    nstate->IF_stall = 1;
    nstate->IF_ID_stall = 1;
}

void jump_flush(CPU_State *nstate)
{
    nstate->flush[1] = 1;
}

void branch_flush(CPU_State *nstate){
    nstate->flush[0] = 1;
    nstate->flush[1] = 1;
    nstate->flush[2] = 1;
}

instruction* get_inst_info(uint32_t pc)
{
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

void IF_Stage(CPU_State *nstate)
{
    instruction * instr;
    if(CURRENT_STATE.IF_stall || cache_stall > 0)
    {
        nstate->PC = CURRENT_STATE.PC;
        return;
    }
    nstate->PC = CURRENT_STATE.PC;
    instr = get_inst_info(CURRENT_STATE.PC);
    nstate->IF_ID_INST = instr;
    nstate->IF_ID_NPC = CURRENT_STATE.PC;
}

void ID_Stage(CPU_State *nstate)
{
    int fwd_data;
    instruction * instr;
    if(CURRENT_STATE.IF_ID_stall || cache_stall > 0){
        return;
    }
    if(!CURRENT_STATE.PIPE[1]){
        return;
    }
    instr = CURRENT_STATE.IF_ID_INST;
    nstate->ID_EX_NPC = CURRENT_STATE.IF_ID_NPC;
    nstate->ID_EX_REG1 = CURRENT_STATE.REGS[RS(instr)];
    nstate->ID_EX_REG2 = CURRENT_STATE.REGS[RT(instr)];
    nstate->ID_EX_RS  = RS(instr);
    if(CURRENT_STATE.MEM_WB_CTRL & CTRL_REGWRITE)
    {
        if(CURRENT_STATE.MEM_WB_CTRL & CTRL_MEMTOREG)
        {
            fwd_data = CURRENT_STATE.MEM_WB_MEM_OUT;
        }
        else
        {
            fwd_data = CURRENT_STATE.MEM_WB_ALU_OUT;
        }
        if(CURRENT_STATE.MEM_WB_WREG == RS(instr))
        {
            nstate->ID_EX_REG1 = fwd_data;
        }
        if(CURRENT_STATE.MEM_WB_WREG == RT(instr))
        {
            nstate->ID_EX_REG2 = fwd_data;
        }
    }
    nstate->ID_EX_RT  = RT(instr);
    nstate->ID_EX_RD  = RD(instr);
    nstate->ID_EX_SHAMT = SHAMT(instr);
    switch(OPCODE(instr))
    {
        case 0x9: // ADDIU
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_REGWRITE | (0x0 << 6);
            break;
        case 0xC: // ANDi
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_REGWRITE | (0x1 << 6);
            break;
        case 0xF: // LUI
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_REGWRITE | (0x2 << 6);
            break;
        case 0xD: // ORI
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_REGWRITE | (0x3 << 6);
            break;
        case 0xB: // SLTIU
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_REGWRITE | (0x4 << 6);
            break;
        case 0x23: // LW
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_MEMREAD | CTRL_MEMTOREG | CTRL_REGWRITE | (0x5 << 6);
            break;
        case 0x2B: // SW
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_ALUSRC | CTRL_MEMWRITE | (0x5 << 6);
            break;
        case 0x4: // BEQ
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_BRANCH | (0x6 << 6);
            break;
        case 0x5: // BNE
            nstate->ID_EX_IMM  = (uint32_t)IMM(instr);
            nstate->ID_EX_CTRL = CTRL_BRANCH | (0x7 << 6);
            break;
        case 0x0: // Type R
            switch(FUNC(instr))
            {
                case 0x21: // ADDU
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0x0 << 6);
                    break;
                case 0x24: // AND
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0x1 << 6);
                    break;
                case 0x27: // NOR
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0x8 << 6);
                    break;
                case 0x25: // OR
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0x9 << 6);
                    break;
                case 0x2B: // SLTU
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0x4 << 6);
                    break;
                case 0x00: //SLL
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0xA << 6);
                    break;
                case 0x02: //SRL
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0xB << 6);
                    break;
                case 0x23: //SUBU
                    nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | (0xC << 6);
                    break;
                case 0x08: //JR
                    if(!nstate->BJ_taken)
                    {
                        nstate->BJ_taken = 1;
                        nstate->PC = CURRENT_STATE.REGS[RS(instr)] - 4;
                    }
                    jump_flush(nstate);
                    nstate->ID_EX_CTRL = 0xE << 6;
                    break;
                default:
                    fprintf(stderr,"NO SUCH INSTRUCTION\n");
                    exit(-1);
                    break;
            }
            break;
        case 0x2: // J
            if(!nstate->BJ_taken)
            {
                nstate->BJ_taken = 1;
                nstate->PC = (TARGET(instr) << 2) - 4;
            }
            jump_flush(nstate);
            nstate->ID_EX_CTRL = 0;
            break;
        case 0x3: // JAL
            if(!nstate->BJ_taken)
            {
                nstate->BJ_taken = 1;
                nstate->PC = (TARGET(instr) << 2) - 4;
            }
            jump_flush(nstate);
            nstate->ID_EX_CTRL = CTRL_REGDST | CTRL_REGWRITE | 0xD << 6;
            break;
        default:
            fprintf(stderr,"NO SUCH INSTRUCTION : %d\n",OPCODE(instr));
            exit(-1);
            break;
    }
}

void EX_Stage(CPU_State *nstate)
{
    uint32_t reg1;
    uint32_t reg2;
    uint32_t op1;
    uint32_t op2;
    uint32_t imm;
    uint32_t fwd_data;
    if(!CURRENT_STATE.PIPE[2] || cache_stall > 0)
    {
        return;
    }

    reg1 = CURRENT_STATE.ID_EX_REG1;
    reg2 = CURRENT_STATE.ID_EX_REG2;
    imm = CURRENT_STATE.ID_EX_IMM;

    nstate->EX_MEM_CTRL = CURRENT_STATE.ID_EX_CTRL;
    nstate->EX_MEM_WRITE_DATA = CURRENT_STATE.ID_EX_REG2;

    // data fowarding
    // second previous one
    if(CURRENT_STATE.MEM_WB_CTRL & CTRL_REGWRITE)
    {
        if(CURRENT_STATE.MEM_WB_CTRL & CTRL_MEMTOREG)
        {
            fwd_data = CURRENT_STATE.MEM_WB_MEM_OUT;
        }
        else
        {
            fwd_data = CURRENT_STATE.MEM_WB_ALU_OUT;
        }
        if(CURRENT_STATE.MEM_WB_WREG == CURRENT_STATE.ID_EX_RS)
        {
            reg1 = fwd_data;
        }
        if(CURRENT_STATE.MEM_WB_WREG == CURRENT_STATE.ID_EX_RT)
        {
            reg2 = fwd_data;
        }
    }
    // previous one
    if(CURRENT_STATE.EX_MEM_CTRL & CTRL_REGWRITE)
    {
        if(!(CURRENT_STATE.EX_MEM_CTRL & CTRL_MEMTOREG))
        {
            fwd_data = CURRENT_STATE.EX_MEM_ALU_OUT;
            if(CURRENT_STATE.EX_MEM_WREG == CURRENT_STATE.ID_EX_RS)
            {
                reg1 = fwd_data;
            }
            if(CURRENT_STATE.EX_MEM_WREG == CURRENT_STATE.ID_EX_RT)
            {
                reg2 = fwd_data;
            }
        }
    }

    //ALUSrc
    if(CURRENT_STATE.ID_EX_CTRL & CTRL_ALUSRC)
    {
        op1 = reg1;
        op2 = imm;
    }
    else
    {
        op1 = reg1;
        op2 = reg2;
    }

    //RegDst
    if(CURRENT_STATE.ID_EX_CTRL & CTRL_REGDST)
    {
        nstate->EX_MEM_WREG = CURRENT_STATE.ID_EX_RD;
    }
    else
    {
        nstate->EX_MEM_WREG = CURRENT_STATE.ID_EX_RT;
    }

    //ALUOP
    switch((CURRENT_STATE.ID_EX_CTRL >> 6) & 0xF)
    {
        case 0x0:
            nstate->EX_MEM_ALU_OUT = op1 + op2;
            break;
        case 0x1:
            nstate->EX_MEM_ALU_OUT = op1 & op2;
            break;
        case 0x2:
            nstate->EX_MEM_ALU_OUT = op2 << 16;
            break;
        case 0x3:
            nstate->EX_MEM_ALU_OUT = op1 | op2;
            break;
        case 0x4:
            nstate->EX_MEM_ALU_OUT = (op1 < op2) ? 1 : 0;
            break;
        case 0x5:
            nstate->EX_MEM_ALU_OUT = op1 + op2;
            break;
        case 0x6:
            nstate->EX_MEM_ALU_OUT = op1 == op2;
            if(op1 == op2)branch_flush(nstate);
            break;
        case 0x7:
            nstate->EX_MEM_ALU_OUT = op1 != op2;
            if(op1 != op2)branch_flush(nstate);
            break;
        case 0x8:
            nstate->EX_MEM_ALU_OUT = ~(op1 | op2);
            break;
        case 0x9:
            nstate->EX_MEM_ALU_OUT = op1 | op2;
            break;
        case 0xA:
            nstate->EX_MEM_ALU_OUT = op2 << CURRENT_STATE.ID_EX_SHAMT;
            break;
        case 0xB:
            nstate->EX_MEM_ALU_OUT =lsr(op2 ,CURRENT_STATE.ID_EX_SHAMT);
            break;
        case 0xC:
            nstate->EX_MEM_ALU_OUT = op1 - op2;
            break;
        case 0xD:
            nstate->EX_MEM_ALU_OUT = CURRENT_STATE.ID_EX_NPC + 8;
            nstate->EX_MEM_WREG = 31;
            break;
    }

    nstate->EX_MEM_BR_TARGET = CURRENT_STATE.ID_EX_NPC + (CURRENT_STATE.ID_EX_IMM << 2);

    if(CURRENT_STATE.ID_EX_CTRL & CTRL_REGWRITE)
    {
        int hazard = 0;
        instruction *instr = CURRENT_STATE.IF_ID_INST;
        switch(OPCODE(instr))
        {
            case 0x9: // ADDIU
            case 0xC: // ANDI
            case 0xF: // LUI
            case 0xD: // ORI
            case 0xB: // SLTIU
            case 0x23: // LW
                if(nstate->EX_MEM_WREG == RS(instr))
                {
                    hazard = 1;
                }
                break;
            case 0x2B: // SW
                if(nstate->EX_MEM_WREG == RT(instr))
                {
                    hazard = 1;
                }
                break;
            case 0x4: // BEQ
            case 0x5: // BNE
                if(nstate->EX_MEM_WREG == RS(instr) || nstate->EX_MEM_WREG == RT(instr))
                {
                    hazard = 1;
                }
                break;
            case 0x0:
                switch(FUNC(instr))
                {
                    case 0x0:
                    case 0x2:
                        if(nstate->EX_MEM_WREG == RT(instr))
                        {
                            hazard = 1;
                        }
                        break;
                    case 0x8:
                        if(nstate->EX_MEM_WREG == RS(instr))
                        {
                            hazard = 1;
                        }
                        break;
                    default:
                        if(nstate->EX_MEM_WREG == RS(instr) || nstate->EX_MEM_WREG == RT(instr))
                        {
                            hazard = 1;
                        }
                        break;
                }
                break;
        }
        if(hazard)
        {
            if((CURRENT_STATE.ID_EX_CTRL & CTRL_MEMTOREG) && CURRENT_STATE.PIPE[1])
            {
                stall(nstate);
            }
        }
    }
}

void MEM_Stage(CPU_State *nstate)
{
    uint32_t write_reg;
    if(!CURRENT_STATE.PIPE[3] || cache_stall > 0)
    {
        if(cache_stall > 0) cache_stall --;
        return;
    }

    nstate->MEM_WB_CTRL = CURRENT_STATE.EX_MEM_CTRL;
    nstate->MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
    nstate->MEM_WB_WREG = CURRENT_STATE.EX_MEM_WREG;

    nstate->BJ_taken = 0;
    if(CURRENT_STATE.EX_MEM_CTRL & CTRL_BRANCH)
    {
        if(CURRENT_STATE.EX_MEM_ALU_OUT)
        {
            nstate->BJ_taken = 1;
            nstate->PC = CURRENT_STATE.EX_MEM_BR_TARGET;
        }
    }

    write_reg = 0;

    if(CURRENT_STATE.EX_MEM_CTRL & CTRL_MEMREAD)
    {
        if(cache_stall == 0){
            nstate->MEM_WB_MEM_OUT = get_data(CURRENT_STATE.EX_MEM_ALU_OUT);
            cache_stall = -1;
        }
        else{
            int chit = is_in_cache(CURRENT_STATE.EX_MEM_ALU_OUT);
            if(chit){
                nstate->MEM_WB_MEM_OUT = get_data(CURRENT_STATE.EX_MEM_ALU_OUT);
            }
            else{
                cache_stall = 29;
            }
        }
        //nstate->MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.EX_MEM_ALU_OUT);
    }
    else if(CURRENT_STATE.EX_MEM_CTRL & CTRL_MEMWRITE)
    {
        if(cache_stall == 0){
            write_data(CURRENT_STATE.EX_MEM_ALU_OUT,CURRENT_STATE.EX_MEM_WRITE_DATA);
            cache_stall = -1;
        }
        else{
            int chit = is_in_cache(CURRENT_STATE.EX_MEM_ALU_OUT);
            if(chit){
                write_data(CURRENT_STATE.EX_MEM_ALU_OUT,CURRENT_STATE.EX_MEM_WRITE_DATA);
            }
            else{
                cache_stall = 29;
            }
        }
        //mem_write_32(CURRENT_STATE.EX_MEM_ALU_OUT,CURRENT_STATE.EX_MEM_WRITE_DATA);
    }
    /*
    if(CURRENT_STATE.EX_MEM_CTRL & CTRL_REGWRITE)
    {
        int hazard = 0;
        switch(OPCODE(CURRENT_STATE.IF_ID_INST))
        {
            case 0x9: // ADDIU
            case 0xC: // ANDI
            case 0xF: // LUI
            case 0xD: // ORI
            case 0xB: // SLTIU
            case 0x23:// LW
                if(CURRENT_STATE.EX_MEM_WREG == RS(CURRENT_STATE.IF_ID_INST))
                {
                    hazard = 1;
                }
                break;
            case 0x2B:// SW
                if(CURRENT_STATE.EX_MEM_WREG == RT(CURRENT_STATE.IF_ID_INST))
                {
                    hazard = 1;
                }
                break;
            case 0x4: // BEQ
            case 0x5: // BNE
                if(CURRENT_STATE.EX_MEM_WREG == RS(CURRENT_STATE.IF_ID_INST)||
                   CURRENT_STATE.EX_MEM_WREG == RT(CURRENT_STATE.IF_ID_INST))
                {
                    hazard = 1;
                }
                break;
            case 0x0: // R
                switch(FUNC(CURRENT_STATE.IF_ID_INST))
                {
                    case 0x0: // SLL
                    case 0x2: // SRL
                        if(CURRENT_STATE.EX_MEM_WREG == RT(CURRENT_STATE.IF_ID_INST))
                        {
                            hazard = 1;
                        }
                        break;
                    case 0x8: // JR
                        if(CURRENT_STATE.EX_MEM_WREG == RS(CURRENT_STATE.IF_ID_INST))
                        {
                            hazard = 1;
                        }
                        break;
                    default:
                        if(CURRENT_STATE.EX_MEM_WREG == RS(CURRENT_STATE.IF_ID_INST)||
                           CURRENT_STATE.EX_MEM_WREG == RT(CURRENT_STATE.IF_ID_INST))
                        {
                            hazard = 1;
                        }
                        break;
                }
                break;
        }
        if(hazard && CURRENT_STATE.PIPE[1])
        {
            stall(nstate);
        }
    }*/
}

void WB_Stage()
{
    if(!CURRENT_STATE.PIPE[4])
    {
        return;
    }
    if(CURRENT_STATE.MEM_WB_CTRL & CTRL_REGWRITE && CURRENT_STATE.MEM_WB_WREG != 0)
    {
        if(CURRENT_STATE.MEM_WB_CTRL & CTRL_MEMTOREG)
        {
            CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_WREG] = CURRENT_STATE.MEM_WB_MEM_OUT;
        }
        else
        {
            CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB_WREG] = CURRENT_STATE.MEM_WB_ALU_OUT;
        }
    }
    INSTRUCTION_COUNT ++;
}

void set_now_state(CPU_State * nstate)
{
    int i;
    CURRENT_STATE.BJ_taken = nstate->BJ_taken;
    CURRENT_STATE.BRANCH_PC = nstate->BRANCH_PC;
    CURRENT_STATE.PC = nstate->PC;
    if(!nstate->IF_stall && cache_stall < 0){
        CURRENT_STATE.IF_ID_INST = nstate->IF_ID_INST;
        CURRENT_STATE.IF_ID_NPC  = nstate->IF_ID_NPC;
    }
    if (cache_stall < 0) {
        CURRENT_STATE.ID_EX_NPC = nstate->ID_EX_NPC;
        CURRENT_STATE.ID_EX_REG1 = nstate->ID_EX_REG1;
        CURRENT_STATE.ID_EX_REG2 = nstate->ID_EX_REG2;
        CURRENT_STATE.ID_EX_IMM = nstate->ID_EX_IMM;
        CURRENT_STATE.ID_EX_FUNC = nstate->ID_EX_FUNC;
        CURRENT_STATE.ID_EX_RS = nstate->ID_EX_RS;
        CURRENT_STATE.ID_EX_RT = nstate->ID_EX_RT;
        CURRENT_STATE.ID_EX_RD = nstate->ID_EX_RD;
        CURRENT_STATE.ID_EX_CTRL = nstate->ID_EX_CTRL;
        CURRENT_STATE.ID_EX_SHAMT = nstate->ID_EX_SHAMT;
    }
    if (cache_stall < 0) {
        CURRENT_STATE.EX_MEM_ALU_OUT = nstate->EX_MEM_ALU_OUT;
        CURRENT_STATE.EX_MEM_BR_TARGET = nstate->EX_MEM_BR_TARGET;
        CURRENT_STATE.EX_MEM_WRITE_DATA = nstate->EX_MEM_WRITE_DATA;
        CURRENT_STATE.EX_MEM_WREG = nstate->EX_MEM_WREG;
        CURRENT_STATE.EX_MEM_CTRL = nstate->EX_MEM_CTRL;
    }
    if (cache_stall < 0) {
        CURRENT_STATE.MEM_WB_ALU_OUT = nstate->MEM_WB_ALU_OUT;
        CURRENT_STATE.MEM_WB_MEM_OUT = nstate->MEM_WB_MEM_OUT;
        CURRENT_STATE.MEM_WB_WREG = nstate->MEM_WB_WREG;
        CURRENT_STATE.MEM_WB_CTRL = nstate->MEM_WB_CTRL;
    }
    for(i=0;i<PIPE_STAGE;i++){
        CURRENT_STATE.flush[i] = nstate->flush[i];
    }
}

void process_instruction()
{
    int i;
    int run = 0;
    CPU_State *nstate = (CPU_State *)calloc(1,sizeof(CPU_State));
    if(cache_stall >= 0)
    {
        CURRENT_STATE.PIPE[4] = 0;
    }
    else
    {
        for(i = PIPE_STAGE - 1; i > 2 ;i--){
            CURRENT_STATE.PIPE[i] = CURRENT_STATE.PIPE[i - 1];
        }
        if(!CURRENT_STATE.IF_ID_stall){
            CURRENT_STATE.PIPE[2] = CURRENT_STATE.PIPE[1];
        }
        else{
            CURRENT_STATE.PIPE[2] = 0;
        }
        if(!CURRENT_STATE.IF_stall){
            CURRENT_STATE.PIPE[1] = CURRENT_STATE.PIPE[0];
        }
        if(!CURRENT_STATE.IF_stall){
            if(!(CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= MEM_REGIONS[0].start + (NUM_INST * 4))){
                CURRENT_STATE.PIPE[0] = CURRENT_STATE.PC;
            }
            else{
                CURRENT_STATE.PC -= 4;
                CURRENT_STATE.PIPE[0] = 0;
            }
        }
        for(i = 0; i<PIPE_STAGE;i++){
            if(CURRENT_STATE.flush[i]){
                CURRENT_STATE.PIPE[i] = 0;
            }
        }
    }
    CURRENT_STATE.IF_stall = 0;
    CURRENT_STATE.IF_ID_stall = 0;
    IF_Stage(nstate);
    ID_Stage(nstate);
    EX_Stage(nstate);
    MEM_Stage(nstate);
    WB_Stage();

    set_now_state(nstate);

    CURRENT_STATE.IF_stall = nstate->IF_stall;
    CURRENT_STATE.IF_ID_stall = nstate->IF_ID_stall;

    if(!CURRENT_STATE.IF_stall && cache_stall < 0){
        CURRENT_STATE.PC += 4;
    }

    if(CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= MEM_REGIONS[0].start + (NUM_INST * 4))
    {
        for(i = 0;i < PIPE_STAGE;i++)
        {
            run |= CURRENT_STATE.PIPE[i];
        }
        if(!run)
        {
            RUN_BIT = FALSE;
        }
    }
    run = 0;
    for(i = 0;i < PIPE_STAGE - 1;i++){
        run |= CURRENT_STATE.PIPE[i];
    }
    if(run == 0 && CURRENT_STATE.PIPE[4] != 0){
        RUN_BIT = FALSE;
    }
}
