#ifndef _PARSER_DEF_
#define _PARSER_DEF_
struct data{
    char * name; // data name
    int addr; // real address
    int cnt; // count
    int * size; // data size(each of the element)
    int * dat; // real data
    struct data * next; // next data
};

struct text{
    int op; // operation number == oper[op]
    int addr; // text addr
    int arg_cnt; // argument_count
    char ** arg; // arguments
    struct text * next; // next argument
};

struct label{ //ex: lab1, lab2, lab3, lab4 ..
    char * name; // label name
    int addr; // label addr
    struct label * next; // next label
};

#endif
