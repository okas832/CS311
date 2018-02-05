
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mips_instruction.h"
#include "parser.h"

//list of functions that we'll use
void parse_data_section(FILE * fin);
void parse_text_section(FILE * fin);
void parse_to_binary(FILE * fout);
void debug_data_section();
void debug_text_section_inst();
void debug_text_section_label();
int search_data(char * str);

/* Structures : store data, store text, store label
 oper[]:list that contains all the needed information corresponding to
 set of instructions  */

struct data * lst_data = NULL;
struct text * lst_text = NULL;
struct label* lst_label= NULL;
extern struct oper oper[];

int data_size = 0;
int text_size = 0;
int DEBUG = 0;

/* main function: <Assembler>
   runs assembly text files, parse and converts it to a binary file
   input: assembly file (*.s)
   output: outfile that contains the converted binary data. (*.o)
   parse_data_section -> parse_text_section : parsing
   parse_to_binary : makes binary according to saved data,text,and label
*/
int main(int argc, char* argv[]){
    char * outfile;
    if(argc != 2){
        printf("Usage : ./runfile <Assembly File>\n");
        return -1;
    }
    FILE * fin = fopen(argv[1],"r");
    if(fin == NULL){
        fprintf(stderr,"fopen error\n");
        return -1;
    }
    parse_data_section(fin);
    parse_text_section(fin);
    if(DEBUG){
        debug_data_section();
        debug_text_section_inst();
        debug_text_section_label();
    }
    outfile = strcat(strcpy((char *)calloc(1,strlen(argv[1]) + 10),strtok(argv[1],".")),".o");
    if(DEBUG){
        printf("output : %s\n",outfile);
    }
    FILE * fout = fopen(outfile,"w");
    parse_to_binary(fout);
    return 0;
}

int parse_size(char * str){
    if(strcmp(str,".word") == 0) return 4;
    return 4;
}

//parse data: return hex/decimal typed string into correct corresponding number.
int parse_data(char * str){
    unsigned int res = 0;
    if(strncmp(str,"0x",2) == 0){
        sscanf(str,"%x",&res);
    }
    else{
        sscanf(str,"%d",&res);
    }
    return res;
}
/* parse_data_section: initialize & set data structure
 * according to the type(data / array)
 * saves name, address, count, size(=4), data(real value) of it
 * address starts from 0x10000000
 */

void parse_data_section(FILE * fin){
    char * line = NULL;
    size_t len = 0;
    struct data * it = NULL;
    unsigned int addr = 0x10000000;
    if(getline(&line,&len,fin) != -1){
        if(strcmp(line,"\t.data\n")){
            fprintf(stderr,"parse error : cannot find data section\n");
            exit(-1);
        }
    }
    while(getline(&line,&len,fin) != -1){
        if(!strcmp(line,"\t.text\n")){
            return;
        }
        if(line[0] != '\t'){
            if(lst_data == NULL){ // initiation process
                lst_data = (struct data *)malloc(sizeof(struct data));
                it = lst_data;
            }
            else{ // after initiation
                it -> next = (struct data *)malloc(sizeof(struct data));
                it = it -> next;
            }
            it -> name    = strdup(strtok(line,":"));
            it -> addr    = addr;
            it -> cnt     = 1;
            it -> size    = (int *) malloc(sizeof(int));
            it -> size[0] = parse_size(strtok(NULL,"\t")); // returns the address of size part(ex: .word)
            it -> dat     = (int *) malloc(4);
            it -> dat[0]  = parse_data(strtok(NULL,"\t"));
            it -> next    = NULL;
        }
        else{ //if array
            it -> cnt += 1;
            it -> size                = (int *)realloc(it -> size, sizeof(it -> size) + 4);
            it -> size[it -> cnt - 1] = parse_size(strtok(line,"\t"));
            it -> dat                 = (int *)realloc(it -> dat, sizeof(it -> dat) + 4);
            it -> dat [it -> cnt - 1] = parse_data(strtok(NULL,"\t"));
        }
        addr += 4;
        data_size += 4;
    }
    return;
}
/*parse_text_section : initialize & set text/label structure.
 * when label, save name & address
 * when text,  save operation number, argument count, argument pointer, address.
 *         -> if the instruction is "la", convert it to one or two lui/ori
 *         instructions.
 */
void parse_text_section(FILE * fin){
    char * line = NULL;
    size_t len = 0;
    struct text * inst = NULL;
    struct label *lab = NULL;
    unsigned int addr = 0x400000;
    char * tmp_str;
    char * arg1, * arg2, * arg3;
    int i;
    while(getline(&line, &len, fin)!= -1){
        if (line[0] != '\t'){
            if(lst_label == NULL){ // label initiation as main.
                lst_label = (struct label *)calloc(1,sizeof(struct label));
                lab = lst_label;
            }
            else{
                lab->next = (struct label *)calloc(1,sizeof(struct label));
                lab = lab->next;
            }
            lab->name   = strdup(strtok(line, ":"));
            lab->addr   = addr; // do not need to add +4 b/c this is label, and it's already +4 by the before instruction.
        }
        else{//if not label. So when "text" type.
            if(lst_text == NULL){ //text initiation.
                lst_text = (struct text *)calloc(1,sizeof(struct text));
                inst = lst_text;
            }
            else{
                inst->next = (struct text *)calloc(1,sizeof(struct text));
                inst = inst->next;
            }
            tmp_str = strtok(line, "\t"); //get the first instruction.
            for(i = 0; i < 20; i++){
                if (!strcmp(oper[i].name, tmp_str)){
                    inst->op = i;
                    break;
                }
            }
            inst->addr = addr;
            if(!strcmp("la",tmp_str)){
                inst->op = 21;
                addr += 4;
                text_size += 4;
            }
            inst -> arg = (char **)calloc(sizeof(char *),3); // allocate the memory size of text argument pointer.
            arg1 = strtok(NULL, "\n, ");
            inst -> arg[0] = strdup(arg1);
            inst -> arg_cnt = 1;
            if ((arg2 = strtok(NULL, "\n, (")) != NULL){
                inst -> arg[1] = strdup(arg2);
                inst -> arg_cnt ++;
                if ((arg3 = strtok(NULL, "\n, )")) != NULL){
                    inst -> arg[2] = strdup(arg3);
                    inst -> arg_cnt ++;
                }
            }
            if(!strcmp("la",tmp_str)){
                if((search_data(inst->arg[1]) & 0xFFFF) == 0){
                    addr -= 4;
                    text_size -= 4;
                }
            }
            addr += 4;
            text_size += 4;
        }
    }
}

char * itob(int num,int digit){ // int to binary. extend it to "digit" bit
    char *ret = (char *)calloc(digit + 1,1);
    int i;
    for(i = digit - 1;i >= 0;i--){
        if((num >> i) & 1){
            ret[digit - i - 1] = '1';
        }
        else{
            ret[digit - i - 1] = '0';
        }
    }
    return ret;
}

int inorder(int n,int len, int * arr){ // return -1 if the arguments is not in right order, and not -1 otherwise.
    int i;
    for(i=0;i<len;i++){
        if(arr[i] == n){
            return i;
        }
    }
    return -1;
}

int search_data(char * str){ // returns the address of the structure "data" which name is str, and return -1 if there isn't one.
    struct data * it;
    it = lst_data;
    while(it != NULL){
        if(!strcmp(it->name,str)){
            return it->addr;
        }
        it = it -> next;
    }
    return -1;
}

int search_label(char * str){ // returns address of structure "label" named str and -1 otherwise.
    struct label * it;
    it = lst_label;
    while(it != NULL){
        if(!strcmp(it->name, str)){
            return it->addr;
        }
        it = it->next;
    }
    return -1;
}
/* parse_arg: parse the arguments 'str' according to the type(number, register,
 * label, data)
 *  number -> return number
 *  register -> return register number
 *  structure data -> shift 2 bit right and return the value.
 *  structure label -> shift 2 bit right , adjust according to now_addr and
 *  return the value.
 *
 *  give current address now_addr as arguement for instructions which should
 *  return the relative address. give now_addr 0 for the absolute address
 *  instruction.
 */
int parse_arg(char * str,int now_addr){
    int addr;
    if(isdigit(str[0])){
        return parse_data(str);
    }
    else if(str[0] == '$'){ //ascii to int
        return atoi(strtok(str,"$"));
    }
    else if((addr = search_data(str)) != -1){
        return addr >> 2;
    }
    else if((addr = search_label(str)) != -1){
        if(now_addr == 0){
            return addr >> 2;
        }
        else{
            return (addr - now_addr - 4) >> 2;
        }
    }
    else if(str[0] == '-' && isdigit(str[1])){ // deal with negative number.
        return atoi(str);
    }
    printf("Error : parse_arg %s\n",str);
    return -1;
}
/* parse_to_binary: final writing to the file
 */
void parse_to_binary(FILE * fout){
    fprintf(fout,"%s",itob(text_size,32));
    fprintf(fout,"%s",itob(data_size,32));
    struct text * it = lst_text;
    struct data * dit = lst_data;
    int i, ord;
    while(it != NULL){
        if(it->op == 21){ // for la instruction.
            int regn = parse_arg(it->arg[0],0);
            int addr = parse_arg(it->arg[1],0) << 2;
            int aup  = addr >> 16;
            int adn  = addr & 0xFFFF;
            fprintf(fout,"00111100000"); //implement lui
            fprintf(fout,"%s",itob(regn,5));
            fprintf(fout,"%s",itob(aup,16));
            if(adn != 0){ // if the last 16 bits are nonzero, we need to implement ori
                fprintf(fout,"001101");
                fprintf(fout,"%s%s",itob(regn,5),itob(regn,5));
                fprintf(fout,"%s",itob(adn,16));
            }
        }
        else if(oper[it->op].type == 0){ //R type
            fprintf(fout,"%s",oper[it->op].bc);
            for(i=1;i<=4;i++){
                if((ord = inorder(i,oper[it->op].arg_len,oper[it->op].order)) != -1){
                    fprintf(fout,"%s",itob(parse_arg(it->arg[ord],0),5));
                }
                else{ //fill the sa part in R type.
                    fprintf(fout, "00000");
                }
            }
            fprintf(fout,"%s",oper[it->op].funct); //fill funct part
        }
        else if(oper[it->op].type == 1){ //I type
            fprintf(fout, "%s", oper[it->op].bc);
            for(i=1 ; i<=2; i++){
                if ((ord = inorder(i, oper[it->op].arg_len, oper[it->op].order)) != -1){
                    fprintf(fout, "%s", itob(parse_arg(it->arg[ord],0),5));
                }
                else{
                    fprintf(fout, "00000");
                }
            }
            if ((ord = inorder(3, oper[it->op].arg_len, oper[it->op].order)) != -1){
                fprintf(fout, "%s", itob(parse_arg(it->arg[ord],it->addr), 16));
            }
        }
        else if(oper[it->op].type == 2){ // J type
            fprintf(fout, "%s", oper[it->op].bc);
            fprintf(fout, "%s", itob(parse_arg(it->arg[0],0), 26));
        }
        it = it -> next;
    }
    while(dit != NULL){ // writing data
        for(i=0;i<dit->cnt;i++){
            fprintf(fout, "%s", itob(dit->dat[i],32));
        }
        dit = dit->next;
    }
    fprintf(fout,"\n");
}

void debug_text_section_inst(){ //for debugging
    struct text * it = lst_text;
    int i;
    printf("----------------------------------------\n");
    printf("|      text section instruction        |\n");
    printf("________________________________________\n");
    while(it != NULL){
        printf("inst : %s\t",oper[it->op].name);
        for(i = 0;i < it->arg_cnt;i++){
            printf("arg%d : %s\t",i,it->arg[i]);
        }
        printf("\n\n");
        it = it -> next;
    }
}

void debug_text_section_label(){
    struct label * it = lst_label;
    printf("----------------------------------------\n");
    printf("|          text section label          |\n");
    printf("________________________________________\n");
    while(it != NULL){
        printf("name : %s\n"  , it -> name);
        printf("addr : %x\n", it -> addr);
        printf("\n");
        it = it -> next;
    }
}

void debug_data_section(){
    struct data * it = lst_data;
    int i;
    printf("----------------------------------------\n");
    printf("|             data section             |\n");
    printf("________________________________________\n");
    while(it != NULL){
        printf("Name  : %s\n", it -> name);
        printf("Addr  : %x\n", it -> addr);
        printf("Count : %d\n", it -> cnt );
        for(i = 0 ; i < it -> cnt ; i++){
            printf("%d -> size : %d, data : %d\n",i,it -> size[i], it -> dat[i]);
        }
        printf("\n");
        it = it -> next;
    }
}
