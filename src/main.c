#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "policies.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <policy> <trace_file>\n", argv[0]);
        fprintf(stderr, "policies: lru, lfu, opt, ml\n");
        return 1;
    }

    const char *policy = argv[1];
    const char *trace_path = argv[2];

    /* Validate policy before opening anything */
    if (strcmp(policy, "lru") != 0 && strcmp(policy, "lfu") != 0 && 
        strcmp(policy, "opt") != 0 && strcmp(policy, "ml") != 0) {
        fprintf(stderr, "Unknown policy: %s\n", policy);
        fprintf(stderr, "policies: lru, lfu, opt, ml\n");
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
    // printf("Trace: %s\n", trace_path);
    // printf("Sets: %d\n", NUM_SETS);
    // printf("Ways: %d\n", NUM_WAYS);
    // printf("Block size: %d bytes\n", BLOCK_SIZE);

    // Branch the execution based on algorithm type
    if (strcmp(policy, "opt") == 0) {
        printf("\nLoading trace into memory for OPT\n");
        TraceEntry *trace = NULL;
        uint64_t n = load_trace(f, &trace);
        
        printf("Loaded %lu accesses. Computing next_use array\n", n);
        uint64_t *next_use = compute_next_use(trace, n);
        
        printf("Running OPT simulation\n");
        opt_simulate(&cache, trace, next_use, n);
        
        // Free the massive arrays to prevent memory leaks
        free(trace);
        free(next_use);
        
    } else {
        printf("\nRunning streaming simulation\n");
        TraceEntry entry;
        uint64_t access_count = 0;
        Perceptron p;
        perceptron_init(&p);
        while (parse_trace_line(f, &entry)) {
            access_count++;

            // Route to the correct online policy
            if (strcmp(policy, "lru") == 0) {
                lru_access(&cache, &entry);
            } else if (strcmp(policy, "lfu") == 0) {
                lfu_access(&cache, &entry);
            } else if (strcmp(policy, "ml") == 0) {
                learned_access(&cache, &p, &entry);
            }
        }
    }

    fclose(f);

    printf("\n");
    cache_print(&cache);

    return 0;
}