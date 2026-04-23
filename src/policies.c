#include "policies.h"
#include <stdlib.h>
#include <string.h>

// LRU (Least Recently Used)
// Evicts the line whose timestamp timestamp is smallest meaning it was accessed the longest time ago.
void lru_access(Cache *cache, TraceEntry *entry) {
    cache->time++;

    uint64_t set_index = get_set_index(entry->address);
    uint64_t tag = get_tag(entry->address);
    CacheSet *set = &cache->sets[set_index];

    // search for a hit
    for (int i = 0; i < NUM_WAYS; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].timestamp = cache->time;
            set->lines[i].frequency++;
            cache->hits++;
            return;
        }
    }

    // If theres a miss but an empty slot
    cache->misses++;
    for (int i = 0; i < NUM_WAYS; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = 1;
            set->lines[i].tag = tag;
            set->lines[i].timestamp = cache->time;
            set->lines[i].frequency = 1;
            return;
        }
    }

    // There are no empty slots
    int victim = 0;
    for (int i = 1; i < NUM_WAYS; i++) {
        if (set->lines[i].timestamp < set->lines[victim].timestamp) {
            victim = i;
        }
    }

    set->lines[victim].tag = tag;
    set->lines[victim].timestamp = cache->time;
    set->lines[victim].frequency = 1;
}


// LFU (Least Frequently Used)
// Evicts the line with the smallest frequency count
void lfu_access(Cache *cache, TraceEntry *entry) {
    cache->time++;

    uint64_t set_index = get_set_index(entry->address);
    uint64_t tag = get_tag(entry->address);
    CacheSet *set = &cache->sets[set_index];

    /* Step 1: search for a hit */
    for (int i = 0; i < NUM_WAYS; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].timestamp = cache->time;
            set->lines[i].frequency++;
            cache->hits++;
            return;
        }
    }

    // If theres a miss but an empty slot
    cache->misses++;
    for (int i = 0; i < NUM_WAYS; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = 1;
            set->lines[i].tag = tag;
            set->lines[i].timestamp = cache->time;
            set->lines[i].frequency = 1;
            return;
        }
    }

    // There are no empty slots
    // Tie break with LRU
    int victim = 0;
    for (int i = 1; i < NUM_WAYS; i++) {
        if (set->lines[i].frequency < set->lines[victim].frequency ||
           (set->lines[i].frequency == set->lines[victim].frequency &&
            set->lines[i].timestamp  < set->lines[victim].timestamp)) {
            victim = i;
        }
    }

    set->lines[victim].tag = tag;
    set->lines[victim].timestamp = cache->time;
    set->lines[victim].frequency = 1;
}

uint64_t load_trace(FILE *f, TraceEntry **trace_out) {
    uint64_t capacity = 1024;
    uint64_t count = 0;

    TraceEntry *trace = malloc(capacity * sizeof(TraceEntry));
    if (!trace) {
        fprintf(stderr, "OOM in load_trace\n");
        exit(1);
    }

    TraceEntry entry;
    while (parse_trace_line(f, &entry)) {
        if (count == capacity) {
            capacity *= 2;
            TraceEntry *tmp = realloc(trace, capacity * sizeof(TraceEntry));
            if (!tmp) {
                fprintf(stderr, "OOM in realloc\n");
                free(trace);
                exit(1);
            }
            trace = tmp;
        }
        trace[count++] = entry;
    }
    *trace_out = trace;
    return count;
}

#define HT_SIZE (1 << 20)
#define HT_EMPTY UINT64_MAX

typedef struct {
    uint64_t key;
    uint64_t value;
} HashTableEntry;

static HashTableEntry ht[HT_SIZE];

static uint64_t ht_get(uint64_t key) {
    uint64_t s = (key * 2654435761ULL) & (HT_SIZE - 1);
    while (ht[s].key != HT_EMPTY && ht[s].key != key) {s = (s+1) & (HT_SIZE-1);}
    return ht[s].key == key ? ht[s].value : HT_EMPTY;
}

static void ht_set(uint64_t key, uint64_t val) {
    uint64_t s = (key * 2654435761ULL) & (HT_SIZE - 1);
    while (ht[s].key != HT_EMPTY && ht[s].key != key) {s = (s+1) & (HT_SIZE-1);}
    ht[s].key = key; ht[s].value = val;
}

uint64_t *compute_next_use(TraceEntry *trace, uint64_t n) {
    uint64_t *next_use = malloc(n * sizeof(uint64_t));
    if (!next_use) {
        fprintf(stderr, "OOM in compute_next_use\n");
        exit(1);
    }
    
    // Initialize le hashtable
    for (int i = 0; i < HT_SIZE; i++) ht[i].key = HT_EMPTY;
    
    // Loop through ts
    for (int64_t i = n - 1; i >= 0; i--) {
        uint64_t addr = trace[i].address >> 6;
        next_use[i] = ht_get(addr);
        ht_set(addr, (uint64_t)i);
    }
 
    return next_use;
}


/* OPT Access */
void opt_simulate(Cache *cache, TraceEntry *trace, uint64_t *next_use, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        uint64_t set_index = get_set_index(trace[i].address);
        uint64_t tag = get_tag(trace[i].address);
        CacheSet *set = &cache->sets[set_index];

        // hit check
        int hit = 0;
        for (int w = 0; w < NUM_WAYS; w++) {
            if (set->lines[w].valid && set->lines[w].tag == tag) {
                cache->hits++;
                set->lines[w].timestamp = next_use[i]; 
                hit = 1;
                break;
            }
        }
        if (hit) continue;

        // Empty slot
        cache->misses++;
        int installed = 0;
        for (int w = 0; w < NUM_WAYS; w++) {
            if (!set->lines[w].valid) {
                set->lines[w].valid = 1;
                set->lines[w].tag = tag;
                set->lines[w].timestamp = next_use[i]; 
                installed = 1;
                break;
            }
        }
        if (installed) continue;

        // Evict line with farthest next use
        int victim = 0;
        uint64_t farthest = set->lines[0].timestamp;

        for (int w = 0; w < NUM_WAYS; w++) {
            if (set->lines[w].timestamp > farthest) { 
                farthest = set->lines[w].timestamp;
                victim = w;
            }
        }

        set->lines[victim].tag = tag;
        set->lines[victim].valid = 1;
        set->lines[victim].timestamp = next_use[i];
    }
}

/* Perceptron-based learned replacement policy */

void perceptron_init(Perceptron *p) {
    memset(p, 0, sizeof(Perceptron));
}

int perceptron_score(Perceptron *p, CacheLine *line, uint64_t current_time) {
    uint64_t recency = current_time - line->timestamp;
    uint64_t frequency = line->frequency;
    
    // Shift the PC to ignore the 2-byte alignment, then mask
    uint64_t pc_idx = (line->current_pc >> 2) & (PC_TABLE_SIZE - 1);

    if (recency > 1000) recency = 1000;

    // Scale recency down by a factor of 64 so it ranges from 0-15
    int scaled_recency = (int)(recency >> 6); 
    // Cap frequency so it doesn't blow up the score
    int scaled_freq = (frequency > 15) ? 15 : (int)frequency;

    int score = 0;
    score += p->w_frequency * scaled_freq;
    score -= p->w_recency * scaled_recency;
    score += p->weights[pc_idx];

    return score;
}

/* Update weights after an eviction */
void perceptron_update(Perceptron *p, CacheLine *line, uint64_t current_time, int was_wrong) {
    uint64_t recency = current_time - line->timestamp;
    uint64_t frequency = line->frequency;
    
    // Hardware-realistic bitwise masking for PC indexing
    uint64_t pc_idx = (line->current_pc >> 2) & (PC_TABLE_SIZE - 1);

    if (recency > 1000) recency = 1000;

    // Feature Scaling: Bring continuous variables down to the Perceptron's weight range
    int scaled_recency = (int)(recency >> 6);
    int scaled_freq = (frequency > 15) ? 15 : (int)frequency;


    int delta = was_wrong ? LEARNING_RATE : -LEARNING_RATE;

    // Update global weights by multiplying the delta by the actual feature values
    p->w_frequency += delta * scaled_freq;
    
    // Recency had an inverted delta in your original logic
    int recency_delta = was_wrong ? -LEARNING_RATE : LEARNING_RATE;
    p->w_recency += recency_delta * scaled_recency;
    p->weights[pc_idx] += delta;

    // Clamp all weights to simulate physical 8-bit saturating counters
    if (p->w_frequency > PERC_MAX_WEIGHT) p->w_frequency = PERC_MAX_WEIGHT;
    if (p->w_frequency < PERC_MIN_WEIGHT) p->w_frequency = PERC_MIN_WEIGHT;
    
    if (p->w_recency > PERC_MAX_WEIGHT) p->w_recency = PERC_MAX_WEIGHT;
    if (p->w_recency < PERC_MIN_WEIGHT) p->w_recency = PERC_MIN_WEIGHT;
    
    if (p->weights[pc_idx] > PERC_MAX_WEIGHT) p->weights[pc_idx] = PERC_MAX_WEIGHT;
    if (p->weights[pc_idx] < PERC_MIN_WEIGHT) p->weights[pc_idx] = PERC_MIN_WEIGHT;

    p->total++;
    if (!was_wrong) p->correct++;
}

#define EVICT_HISTORY 64

typedef struct {
    uint64_t tag;
    uint64_t set_index;
    CacheLine line_snapshot;
    uint64_t evict_time;
    int valid;
} EvictRecord;

static EvictRecord evict_history[EVICT_HISTORY];
static int evict_head = 0;

void learned_access(Cache *cache, Perceptron *p, TraceEntry *entry) {
    cache->time++;

    uint64_t set_index = get_set_index(entry->address);
    uint64_t tag = get_tag(entry->address);
    CacheSet *set = &cache->sets[set_index];

    // Hit check
    for (int w = 0; w < NUM_WAYS; w++) {
        if (set->lines[w].valid && set->lines[w].tag == tag) {
            set->lines[w].timestamp = cache->time;
            set->lines[w].frequency++;
            set->lines[w].hit = 1;
            cache->hits++;
            return;
        }
    }

    // Check eviction history for wrong evictions
    cache->misses++;
    for (int h = 0; h < EVICT_HISTORY; h++) {
        if (evict_history[h].valid &&
            evict_history[h].tag == tag &&
            evict_history[h].set_index == set_index) {
            /* This address was recently evicted and just came back: wrong eviction */
            perceptron_update(p, &evict_history[h].line_snapshot, evict_history[h].evict_time, 1);
            evict_history[h].valid = 0;
            break;
        }
    }

    // Empty slot
    for (int w = 0; w < NUM_WAYS; w++) {
        if (!set->lines[w].valid) {
            set->lines[w].valid = 1;
            set->lines[w].tag = tag;
            set->lines[w].timestamp = cache->time;
            set->lines[w].frequency = 1;
            set->lines[w].current_pc = entry->pc;
            set->lines[w].hit = 0;
            return;
        }
    }

    // Score all ways and evict the lowest-scoring line
    int victim = 0;
    int lowest = perceptron_score(p, &set->lines[0], cache->time);

    for (int w = 1; w < NUM_WAYS; w++) {
        int score = perceptron_score(p, &set->lines[w], cache->time);
        if (score < lowest) {
            lowest = score;
            victim = w;
        }
    }

    // Record the eviction so we can update weights if it comes back
    evict_history[evict_head].tag = set->lines[victim].tag;
    evict_history[evict_head].set_index = set_index;
    evict_history[evict_head].line_snapshot = set->lines[victim];
    evict_history[evict_head].evict_time = cache->time;
    evict_history[evict_head].valid = 1;
    evict_head = (evict_head + 1) % EVICT_HISTORY;

    // If the evicted line was never reused (hit = 0), it was a good eviction
    if (!set->lines[victim].hit) {
        perceptron_update(p, &set->lines[victim], cache->time, 0);
    }

    // Install new line
    set->lines[victim].tag = tag;
    set->lines[victim].timestamp = cache->time;
    set->lines[victim].frequency = 1;
    set->lines[victim].current_pc = entry->pc;
    set->lines[victim].hit = 0;
}