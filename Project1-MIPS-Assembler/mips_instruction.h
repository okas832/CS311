#ifndef __MIPS_INST__
#define __MIPS_INST__
struct oper{
    char * name;
    char * bc;//opcode
    char * funct;
    int    arg_len;
    int    order[4]; // order[i] == j => ith argument go to jth position
    int    type;  // R == 0, I == 1, J == 2
};
// There is no LA operator because it is pesudo instruction
// LA operation will handle in parsing
#endif
