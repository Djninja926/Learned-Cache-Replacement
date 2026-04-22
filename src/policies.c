#include "policies.h"
#include <stdlib.h>

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