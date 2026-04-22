#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stdio.h>

#define NUM_SETS 256 // 256 Buckets/Sets of Cache Lines
#define NUM_WAYS 8 // 8 Cache lines in each set
#define BLOCK_SIZE 64 // 64 Bytes (Standard x86)

typedef struct {
    uint64_t tag;
    int valid;

    uint64_t timestamp; // Clock for LRU
    uint64_t frequency; // "Clock" for LFU

    // uint64_t current_pc;    // The Program Counter that brought this line into cache
    // int hit;            // 1 if this line was reused (good), 0 if never reused (dead block)
} CacheLine;

typedef struct {
    CacheLine lines[NUM_WAYS];
} CacheSet;

typedef struct {
    CacheSet sets[NUM_SETS];
    uint64_t time;
    uint64_t hits;
    uint64_t misses;
} Cache;

typedef struct {
    uint64_t pc;
    uint64_t address;
    int type;
} TraceEntry;


/* Function declarations */

void cache_init(Cache *cache); // Initialize all cache lines to invalid
int parse_trace_line(FILE *f, TraceEntry *entry);
 
// Given a full memory address, extract the set index and tag
uint64_t get_set_index(uint64_t address);
uint64_t get_tag(uint64_t address);

void cache_print(Cache *cache);





#endif /* CACHE_H */