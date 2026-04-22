#ifndef POLICIES_H
#define POLICIES_H

#include "cache.h"

// LRU & LFU evict the line that was accessed least recently
void lru_access(Cache *cache, TraceEntry *entry);
void lfu_access(Cache *cache, TraceEntry *entry);

// OPT Functions
uint64_t load_trace(FILE *f, TraceEntry **trace_out); // Load entire trace into memory, returns number of accesses
uint64_t *compute_next_use(TraceEntry *trace, uint64_t n); // Precompute next_use array via backward scan
void opt_simulate(Cache *cache, TraceEntry *trace, uint64_t *next_use, uint64_t n); // Run full OPT simulation on preloaded trace

#endif /* POLICIES_H */