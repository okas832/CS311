#include "cache.h"
#include "util.h"

/* cache.c : Implement your functions declared in cache.h */


/***************************************************************/
/*                                                             */
/* Procedure: setupCache                                 */
/*                                                             */
/* Purpose: Allocates memory for your cache                    */
/*                                                             */
/***************************************************************/

void setupCache(int capacity, int num_way, int block_size)
{
/*  code for initializing and setting up your cache    */
/*  You may add additional code if you need to    */
    int i,j,k; //counter
    int nset=0; // number of sets
    int _wpb=0; //words per block
    nset=capacity / (block_size * num_way);
    _wpb = block_size / BYTES_PER_WORD;

    Cache = (uint32_t  ***)malloc(nset * sizeof(uint32_t **));
    cache_label = (mcache *)malloc(nset * sizeof(mcache));
    for (i=0;i<nset;i++)
    {
        Cache[i] = (uint32_t ** )malloc(num_way * sizeof(uint32_t*));
    }
    for (i=0; i<nset; i++)
    {
        for (j=0; j<num_way; j++)
        {
            Cache[i][j] = (uint32_t*)malloc(sizeof(uint32_t)*(_wpb));
            for (k = 0; k < _wpb; k++){
                Cache[i][j][k] = 0;
            }
        }
    }
    cache_label[0].LRU = 0b11100100;
    cache_label[1].LRU = 0b11100100;
    for (i=0;i<2;i++)//initialize to all 0.
    {
        cache_label[i].valid_dirtybit = 0;
        cache_label[i].way_tag[0] = 0;
        cache_label[i].way_tag[1] = 0;
        cache_label[i].way_tag[2] = 0;
        cache_label[i].way_tag[3] = 0;
    }
    cache_stall = -1;
}

/***************************************************************/
/*                                                             */
/* Procedure: setCacheMissPenalty                             */
/*                                                             */
/* Purpose: Sets how many cycles your pipline will stall       */
/*                                                             */
/***************************************************************/

void setCacheMissPenalty(int penalty_cycles)
{
    miss_penalty = penalty_cycles;
}


int is_in_cache(uint32_t mem)
{
    if (search_index(mem) == -1)
    {
        return FALSE;
    }
    return TRUE;
}
//search_index : return ijjk if the data in input memory is in the cache, and
//return -1 if it's not in the cache.
int8_t search_index(uint32_t mem)
{
    uint32_t memtag;
    int8_t result;
    int i,j;
    result = 0;
    memtag = (mem >> 4);
    i = (mem & 0b1000)>>3;
    for(j=0; j<4; j++)
    {
        if((cache_label[i].way_tag[j] == memtag) && ((cache_label[i].valid_dirtybit >> (7 - 2 * j)) & 1) == 1)
        {
            result |= i << 3; // set 'set value'at the return value.
            result |= j << 1; // set 'way'.  2 bit. 0~3.
            result |= (mem & 0b100)>>2; // set 'byte'. 1 bit. 0/1.
            return result;
        }
    }
    return -1;
}
//write_cache : read memory and write to cache.
//set valid bit to 1 and dirtybit to 0
//set LRU of this to 0.
void write_cache(int i, int j, uint32_t addr)
{
    uint8_t mask[4];
    mask[0] = 0b00111111;
    mask[1] = 0b11001111;
    mask[2] = 0b11110011;
    mask[3] = 0b11111100;
    //write to cache.
    addr = addr & (~0b111); //set memory address in order to write in way as
    //         way[j] address
    //word[0] : ~...000
    //word[1] : ~...100
    Cache[i][j][0] = mem_read_32(addr);
    Cache[i][j][1] = mem_read_32(addr + 4);
    //set valid & dirty bit.
    cache_label[i].valid_dirtybit &= mask[j]; // cleaning
    cache_label[i].valid_dirtybit += (1 << (7 - (2 * j))); // make valid bit to 1
    //set tag & index.
    cache_label[i].way_tag[j] = addr >> 4;

}
//update_cache: update the data in the given position.
//set dirty bit to 1.
void update_cache(int i, int j, int k, uint32_t data)
{
    Cache[i][j][k] = data;
    cache_label[i].valid_dirtybit |= (1 << (2 * j)); //used "or" in case we updated more than once.
}

// evict_cache: set valid bit to 0. update LRU
// if dirty bit was 1, write to memory.
// return value: the idx of [way] ( == j) which was currently evicted.
int evict_cache(int i)
{
    int idx;
    uint32_t addr;
    uint8_t mask[4];
    uint8_t LRU;
    int j;
    mask[0] = 0b11000000;
    mask[1] = 0b00110000;
    mask[2] = 0b00001100;
    mask[3] = 0b00000011;
    LRU = cache_label[i].LRU;
    for(j=0;j<4;j++){
        if((((LRU & mask[j]) >> (2 * (3 - j))) & 0b11) == 3){
            idx = j;
        }
    }
    if ((cache_label[i].valid_dirtybit) & (1 << (2 * idx))) // if the dirty bit was 1, write to data.
    {
        addr = ((cache_label[i].way_tag[idx]) << 4) + (i<<3);
        mem_write_32(addr, Cache[i][idx][0]);
        addr += 4;
        mem_write_32(addr, Cache[i][idx][1]);
    }

    cache_label[i].valid_dirtybit &= ~mask[idx]; // set valid & dirty bit to 0.
    // clean off the tag & index just in case.
    cache_label[i].way_tag[idx] = 0;

    // update LRU
    return idx;
}

// get_data_from_cache
// precondition : Cache hit on that memaddr
// return value : the data that exist in memaddr
uint32_t get_data_from_cache(uint32_t memaddr)
{
    uint32_t cache_idx = search_index(memaddr);
    uint32_t set, way, bnum;
    set = cache_idx >> 3;
    way = (cache_idx & 0b110) >> 1;
    bnum = (cache_idx & 0b1);
    update_LRU(set,way);
    return Cache[set][way][bnum];
}

void update_LRU(int i, int j)
{
    uint32_t temp;
    uint8_t idx;
    uint8_t mask[4];
    int k;
    temp = cache_label[i].LRU;
    mask[0] = 0b11000000;
    mask[1] = 0b00110000;
    mask[2] = 0b00001100;
    mask[3] = 0b00000011;
    idx = 0;
    idx = (temp & mask[j]) >> (6 - 2 * j);
    for (k = 0; k < 4; k++)
    {
        if (((temp & (mask[k])) >> (6 - 2 * k)) < idx)
        {
            cache_label[i].LRU += (1 << (6 - 2 * k));
        }
    }
    cache_label[i].LRU &= (~mask[j]); //set to 0(recently used)
}

uint32_t get_data(uint32_t mem)
{
    int i,j;
    i = (mem >> 3) & 1; // i = set
    if (is_in_cache(mem)) // cache_hit
    {
        return get_data_from_cache(mem);
    }
    else // cache_miss
    {
        j = evict_cache(i);
        write_cache(i, j, mem);
        return get_data_from_cache(mem);
    }
}

void write_data(uint32_t mem, uint32_t data)
{
    int i,j,k, val;
    i = (mem >> 3) & 1;
    if (is_in_cache(mem)) // cache_hit - it is in cache.
    {
        val = search_index(mem);
        i = val >> 3;
        j = (val & 0b110) >> 1;
        k = (val & 0b1);
        update_cache(i, j, k, data);
    }
    else // cache_miss
    {    // if cache_miss, get data from the memory, (write to cache) and update data.
        j = evict_cache(i);
        write_cache(i, j, mem);
        k = (mem&0b100) >> 2;
        update_cache(i, j, k, data);
    }
    update_LRU(i,j);
}
