#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "policies.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <policy> <trace_file>\n", argv[0]);
        fprintf(stderr, "policies: lru, lfu\n");
        return 1;
    }

    const char *policy = argv[1];
    const char *trace_path = argv[2];

    /* Validate policy before opening anything */
    if (strcmp(policy, "lru") != 0 && strcmp(policy, "lfu") != 0) {
        fprintf(stderr, "Unknown policy: %s\n", policy);
        fprintf(stderr, "policies: lru, lfu\n");
        return 1;
    }

    /* Open trace file */
    FILE *f = fopen(trace_path, "r");
    if (!f) {
        fprintf(stderr, "Error: could not open trace file: %s\n", trace_path);
        return 1;
    }

    // Initialize cache
    Cache cache;
    cache_init(&cache);

    printf("Policy: %s\n", policy);
    printf("Trace: %s\n", trace_path);
    printf("Sets: %d\n", NUM_SETS);
    printf("Ways: %d\n", NUM_WAYS);
    printf("Block size: %d bytes\n", BLOCK_SIZE);

    TraceEntry entry;
    uint64_t access_count = 0;

    while (parse_trace_line(f, &entry)) {
        access_count++;

        if (strcmp(policy, "lru") == 0) {
            lru_access(&cache, &entry);
        } else if (strcmp(policy, "lfu") == 0) {
            lfu_access(&cache, &entry);
        }

        // Progress indicator every 1M accesses
        if (access_count % 1000000 == 0) {
            printf("  Processed %lu M accesses...\n", access_count / 1000000);
        }
    }

    fclose(f);

    printf("\n");
    cache_print(&cache);

    return 0;
}