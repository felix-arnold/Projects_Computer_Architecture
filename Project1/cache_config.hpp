#ifndef CACHE_CONFIG_HPP
#define CACHE_CONFIG_HPP

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <chrono>

typedef enum replace_policy {
    // LRU replacement
    REPLACE_POLICY_LRU,
    // LFU replacement
    REPLACE_POLICY_LFU
} replace_policy_t;

typedef enum insert_policy {
    INSERT_POLICY_MIP,
    INSERT_POLICY_LIP,
} insert_policy_t;

typedef enum write_strat {
    // Write back, write-allocate
    WRITE_STRAT_WBWA,
    // Write through, write-no-allocate
    WRITE_STRAT_WTWNA,
} write_strat_t;

typedef struct cache_config {
    bool disabled;
    bool prefetcher_disabled;
    bool strided_prefetch_disabled;
    // (C,B,S) in the Conte Cache Taxonomy (Patent Pending)
    uint64_t c;
    uint64_t b;
    uint64_t s;
    replace_policy_t replace_policy;
    insert_policy_t prefetch_insert_policy;
    write_strat_t write_strat;
} cache_config_t;

typedef struct sim_stats {
    uint64_t reads;
    uint64_t writes;
    uint64_t accesses_l1;
    uint64_t reads_l2;
    uint64_t writes_l2;
    uint64_t accesses_l2;
    uint64_t hits_l1;
    uint64_t read_hits_l2;
    uint64_t misses_l1;
    uint64_t read_misses_l2;
    uint64_t prefetches_l2;
    double hit_ratio_l1;
    double read_hit_ratio_l2;
    double miss_ratio_l1;
    double read_miss_ratio_l2;
    double avg_access_time_l1;
    double avg_access_time_l2;
} sim_stats_t;

// Argument to cache_access rw. Indicates a load
static const char READ = 'R';
// Argument to cache_access rw. Indicates a store
static const char WRITE = 'W';

#endif