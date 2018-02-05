#ifndef __CACHE_H__
#define __CACHE_H__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* cache.h : Declare functions and data necessary for your project*/

int cache_stall;

int miss_penalty;  // number of cycles to stall when a cache miss occurs
uint32_t ***Cache; // "data" cache storing data [set][way][byte]

void setupCache(int, int, int);
void setCacheMissPenalty(int);

//  struct my_cache for a set(one line)
typedef struct my_cache {
//  uint32_t data; //32bit
    uint8_t  LRU; //8bit
    uint8_t valid_dirtybit; // 00000000, 00000001, 00000010, 00000011
    uint32_t way_tag[4]; // 28bit of the memory address except the index part.
}mcache;

mcache * cache_label;

int evict_cache(int);
void write_cache(int, int, uint32_t); // write the data to the cache from the memory.
void update_cache(int, int, int, uint32_t); // update the data in the cache
int8_t search_index(uint32_t mem);
int is_in_cache(uint32_t mem);//mem = memory address of the data.
void update_LRU(int, int);
uint32_t get_data_from_cache(uint32_t);

uint32_t get_data(uint32_t mem);
void write_data(uint32_t mem, uint32_t data);

#endif
