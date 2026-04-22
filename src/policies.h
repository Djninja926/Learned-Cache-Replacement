#ifndef POLICIES_H
#define POLICIES_H

#include "cache.h"

// LRU & LFU evict the line that was accessed least recently
void lru_access(Cache *cache, TraceEntry *entry);
void lfu_access(Cache *cache, TraceEntry *entry);

#endif /* POLICIES_H */