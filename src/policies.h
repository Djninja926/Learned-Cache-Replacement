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

#define PERC_FEATURES 3
#define PC_TABLE_SIZE 1024
#define PERC_THRESHOLD 0
#define PERC_MAX_WEIGHT 127
#define PERC_MIN_WEIGHT -128
#define LEARNING_RATE 1
 
typedef struct {
    int weights[PC_TABLE_SIZE];
    int w_recency;
    int w_frequency;
    uint64_t correct;
    uint64_t total;
} Perceptron;
 
void perceptron_init(Perceptron *p);
int perceptron_score(Perceptron *p, CacheLine *line, uint64_t current_time);
void perceptron_update(Perceptron *p, CacheLine *line, uint64_t current_time, int was_wrong);
void learned_access(Cache *cache, Perceptron *p, TraceEntry *entry);

#endif /* POLICIES_H */
