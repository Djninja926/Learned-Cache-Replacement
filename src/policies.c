#include "policies.h"

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