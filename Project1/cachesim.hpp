#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include "cache_config.hpp"
#include "cache.hpp"

typedef struct sim_config {
    cache_config_t l1_config;
    cache_config_t l2_config;
} sim_config_t;

extern void sim_setup(sim_config_t *config);
extern void sim_access(char rw, uint64_t addr, sim_stats_t* p_stats);
extern void sim_finish(sim_stats_t *p_stats);

// Sorry about the /* comments */. C++11 cannot handle basic C99 syntax,
// unfortunately
static const sim_config_t DEFAULT_SIM_CONFIG = {
    /*.l1_config =*/ {/*.disabled =*/ false,
                      /*.prefetcher_disabled =*/ true,
                      /*.strided_prefetch_disabled =*/ true,
                      /*.c =*/ 10, // 1KB Cache
                      /*.b =*/ 6,  // 64-byte blocks
                      /*.s =*/ 1,  // 2-way
                      /*.replace_policy =*/ REPLACE_POLICY_LRU,
                      /*.prefetch_insert_policy =*/ INSERT_POLICY_MIP,
                      /*.write_strat =*/ WRITE_STRAT_WBWA},

    /*.l2_config =*/ {/*.disabled =*/ false,
                      /*.prefetcher_disabled =*/ false,
                      /*.strided_prefetch_disabled =*/ false,
                      /*.c =*/ 15, // 32KB Cache
                      /*.b =*/ 6,  // 64-byte blocks
                      /*.s =*/ 3,  // 8-way
                      /*.replace_policy =*/ REPLACE_POLICY_LRU,
                      /*.prefetch_insert_policy =*/ INSERT_POLICY_LIP,
                      /*.write_strat =*/ WRITE_STRAT_WTWNA}
};

static const double DRAM_ACCESS_TIME = 100;
static const double L1_HIT_K0 = 1;
static const double L1_HIT_K1 = 0.15;
static const double L1_HIT_K2 = 0.15;
static const double L2_HIT_K3 = 4;
static const double L2_HIT_K4 = 0.3;
static const double L2_HIT_K5 = 0.3;

#endif /* CACHESIM_HPP */
