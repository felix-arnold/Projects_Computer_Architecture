#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cachesim.hpp"

static void print_help(void);
static int parse_insert_policy(const char *arg, insert_policy_t *policy_out);
static int parse_replace_policy(const char *arg, replace_policy_t *policy_out);
static int validate_config(sim_config_t *config);
static void print_cache_config(cache_config_t *cache_config, const char *cache_name);
static void print_statistics(sim_stats_t* stats);

int main(int argc, char **argv) {
    sim_config_t config = DEFAULT_SIM_CONFIG;
    int opt;
    char trace_fn[512];

    /* Read arguments */
    while(-1 != (opt = getopt(argc, argv, "c:b:s:f:r:C:S:I:P:Dh"))) {
        switch(opt) {
        case 'c':
            config.l1_config.c = atoi(optarg);
            break;
        case 'b':
            config.l1_config.b = atoi(optarg);
            config.l2_config.b = config.l1_config.b;
            break;
        case 's':
            config.l1_config.s = atoi(optarg);
            break;
        case 'f':
	    strncpy(trace_fn, optarg, 511);
            break;
        case 'r':
            if (parse_replace_policy(optarg, &config.l2_config.replace_policy)) {
                return 1;
            }
            config.l1_config.replace_policy = config.l2_config.replace_policy;
            break;
        case 'C':
            config.l2_config.c = atoi(optarg);
            break;
        case 'S':
            config.l2_config.s = atoi(optarg);
            break;
        case 'I':
            if (parse_insert_policy(optarg, &config.l2_config.prefetch_insert_policy)) {
                return 1;
            }
            break;
        case 'P':
	    if (atoi(optarg) == 0) {
            	config.l2_config.prefetcher_disabled = true;
            	config.l2_config.prefetcher_disabled = true;
	    } else if (atoi(optarg) == 1) {
            	config.l2_config.prefetcher_disabled = false;
            	config.l2_config.strided_prefetch_disabled = true;
	    } else if (atoi(optarg) == 2) {
            	config.l2_config.prefetcher_disabled = false;
            	config.l2_config.strided_prefetch_disabled = false;
	    } else {
                    printf("Unknown prefetcher option `%s'\n", optarg);
		    return 1;
	    }
            break;
        case 'D':
            config.l2_config.disabled = true;
            break;
        case 'h':
            /* Fall through */
        default:
            print_help();
            return 0;
        }
    }

    if (strlen(trace_fn) == 0) {
	    printf("ERROR: need input file name, use -f <tracefile>\n");
	    fflush(stdout);
	    return 1;
    }

    printf("Cache Settings\n");
    printf("--------------\n");
    print_cache_config(&config.l1_config, "L1");
    print_cache_config(&config.l2_config, "L2");
    printf("\n");

    if (validate_config(&config)) {
        return 1;
    }

    /* Setup the cache */
    sim_setup(&config);

    /* Setup statistics */
    sim_stats_t stats;
    memset(&stats, 0, sizeof stats);

    /* Begin reading the file */
    char rw;
    uint64_t address;

    FILE *f;
    f = fopen(trace_fn, "r");

    if (!f) {
	    printf("ERROR: can't open file %s\n", trace_fn);
	    fflush(stdout);
	    return 1;
    }

    while (!feof(f)) {
        int ret = fscanf(f, "%c 0x%" PRIx64 "\n", &rw, &address);
        if(ret == 2) {
            sim_access(rw, address, &stats);
        }
    }

    sim_finish(&stats);

    print_statistics(&stats);

    fclose(f);

    return 0;
}

static int parse_replace_policy(const char *arg, replace_policy_t *policy_out) {
    if (!strcmp(arg, "lru") || !strcmp(arg, "LRU")) {
        *policy_out = REPLACE_POLICY_LRU;
        return 0;
    } else if (!strcmp(arg, "lfu") || !strcmp(arg, "LFU")) {
        *policy_out = REPLACE_POLICY_LFU;
        return 0;
    } else {
        printf("Unknown cache replacement policy `%s'\n", arg);
        return 1;
    }
}

static int parse_insert_policy(const char *arg, insert_policy_t *policy_out) {
    if (!strcmp(arg, "mip") || !strcmp(arg, "MIP")) {
        *policy_out = INSERT_POLICY_MIP;
        return 0;
    } else if (!strcmp(arg, "lip") || !strcmp(arg, "LIP")) {
        *policy_out = INSERT_POLICY_LIP;
        return 0;
    } else {
        printf("Unknown cache insertion policy `%s'\n", arg);
        return 1;
    }
}

static void print_help(void) {
    printf("cachesim [OPTIONS] < traces/file.trace\n");
    printf("-h\t\tThis helpful output\n");
    printf("L1 parameters:\n");
    printf("  -c C1\t\tTotal size for L1 in bytes is 2^C1\n");
    printf("  -b B1\t\tSize of each block for L1 in bytes is 2^B1\n");
    printf("  -s S1\t\tNumber of blocks per set for L1 is 2^S1\n");
    printf("L1 & L2 parameters:\n");
    printf("  -f <tracefile>\t\tTrace filename\n");
    printf("  -r r12\t\tReplacement policy for both L1 and L2 (lru or lfu)\n");
    printf("L2 parameters:\n");
    printf("  -C C2\t\tTotal size in bytes for L2 is 2^C1\n");
    printf("  -S S2\t\tNumber of blocks per set for L2 is 2^S1\n");
    printf("  -I I2\t\tInsertion policy for L2 prefetching (mip or lip)\n");
    printf("  -P <0,1,2> \t\tPrefetcher: 0 is no prefetch, 1 is +1 prefetch, and 2 is strided.\n");
    printf("  -D   \t\tDisable L2 cache\n");
}

static int validate_config(sim_config_t *config) {
    if (config->l1_config.b > 7 || config->l1_config.b < 4) {
        printf("Invalid configuration! The block size must be reasonable: 4 <= B <= 7\n");
        return 1;
    }

    if (!config->l2_config.disabled && config->l1_config.s > config->l2_config.s) {
        printf("Invalid configuration! L1 associativity must be less than or equal to L2 associativity\n");
        return 1;
    }

    if (!config->l2_config.disabled && config->l1_config.c >= config->l2_config.c) {
        printf("Invalid configuration! L1 size must be strictly less than L2 size\n");
        return 1;
    }

    return 0;
}

static const char *replace_policy_str(replace_policy policy) {
    switch (policy) {
        case REPLACE_POLICY_LRU: return "LRU";
        case REPLACE_POLICY_LFU: return "LFU";
        default: return "Unknown policy";
    }
}

static const char *insert_policy_str(insert_policy_t policy) {
    switch (policy) {
        case INSERT_POLICY_MIP: return "MIP";
        case INSERT_POLICY_LIP: return "LIP";
        default: return "Unknown policy";
    }
}

static void print_cache_config(cache_config_t *cache_config, const char *cache_name) {
    printf("%s ", cache_name);
    if (cache_config->disabled) {
        printf("disabled\n");
    } else {
        printf("(C,B,S): (%" PRIu64 ",%" PRIu64 ",%" PRIu64 "). Replacement policy: %s.",
           cache_config->c, cache_config->b, cache_config->s,
           replace_policy_str(cache_config->replace_policy)
           );

        if (cache_config->prefetcher_disabled) {
            printf(" Prefetcher disabled.");
        } else {
            //printf(" Prefetch enabled.");
            if (cache_config->strided_prefetch_disabled) {
                printf(" +1 prefetcher.");
            } else {
                printf(" Strided prefetcher.");
            }
            printf(" Prefetch insertion policy: %s.",
            insert_policy_str(cache_config->prefetch_insert_policy)
            );
        }
        printf("\n");
    }
}

static void print_statistics(sim_stats_t* stats) {
    printf("Cache Statistics\n");
    printf("----------------\n");
    printf("Reads: %" PRIu64 "\n", stats->reads);
    printf("Writes: %" PRIu64 "\n", stats->writes);
    printf("\n");
    printf("L1 accesses: %" PRIu64 "\n", stats->accesses_l1);
    printf("L1 hits: %" PRIu64 "\n", stats->hits_l1);
    printf("L1 misses: %" PRIu64 "\n", stats->misses_l1);
    printf("L1 hit ratio: %.3f\n", stats->hit_ratio_l1);
    printf("L1 miss ratio: %.3f\n", stats->miss_ratio_l1);
    printf("L1 average access time (AAT): %.3f\n", stats->avg_access_time_l1);
    printf("\n");
    printf("L2 reads: %" PRIu64 "\n", stats->reads_l2);
    printf("L2 writes: %" PRIu64 "\n", stats->writes_l2);
    printf("L2 read hits: %" PRIu64 "\n", stats->read_hits_l2);
    printf("L2 read misses: %" PRIu64 "\n", stats->read_misses_l2);
    printf("L2 prefetches: %" PRIu64 "\n", stats->prefetches_l2);
    printf("L2 read hit ratio: %.3f\n", stats->read_hit_ratio_l2);
    printf("L2 read miss ratio: %.3f\n", stats->read_miss_ratio_l2);
    printf("L2 average access time (AAT): %.3f\n", stats->avg_access_time_l2);
}
