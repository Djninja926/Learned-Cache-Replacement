#include "cache.h"
#include <string.h>
#include <math.h>
#include <inttypes.h>

#define BLOCK_OFFSET 6
#define SET_INDEX 8

void cache_init(Cache *cache) {
    memset(cache, 0, sizeof(Cache));
}

uint64_t get_set_index(uint64_t address) {
    return (address >> BLOCK_OFFSET) & (NUM_SETS - 1);
}

uint64_t get_tag(uint64_t address) {
    return address >> (BLOCK_OFFSET + SET_INDEX);
}

int parse_trace_line(FILE *f, TraceEntry *entry) {
    char line[256];
 
    while (fgets(line, sizeof(line), f)) {
        // Skip blank lines and comments
        if (line[0] == '\n' || line[0] == '#') continue;
 
        // Try to parse: pc address type
        unsigned long long pc, address;
        int type;
 
        int parsed = sscanf(line, "%llx %llx %d", &pc, &address, &type);
 
        if (parsed == 3) {
            entry->pc = (uint64_t)pc;
            entry->address = (uint64_t)address;
            entry->type = type;
            return 1;
        } else if (parsed == 2) {
            // Some traces omit the type field; default to load
            entry->pc = (uint64_t)pc;
            entry->address = (uint64_t)address;
            entry->type = 0;
            return 1;
        }
        // If we can't parse the line, skip it and try the next
    }
 
    return 0; /* EOF */
}

void cache_print(Cache *cache) {
    uint64_t total = cache->hits + cache->misses;
    double hit_rate  = total > 0 ? (double) cache->hits  / total * 100.0 : 0.0;
    double miss_rate = total > 0 ? (double) cache->misses / total * 100.0 : 0.0;
 
    printf("Cache Statistics\n");
    printf("Total accesses: %lu\n", total);
    printf("Hits: %lu (%.2f%%)\n", cache->hits, hit_rate);
    printf("Misses: %lu (%.2f%%)\n\n", cache->misses, miss_rate);
}