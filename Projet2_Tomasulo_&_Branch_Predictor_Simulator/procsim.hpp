#ifndef PROCSIM_H
#define PROCSIM_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>

#define L1_HIT_CYCLES (2)
#define L2_HIT_CYCLES (10)
#define L2_MISS_CYCLES (100)

#define ALU_STAGES 1
#define MUL_STAGES 3
#define LSU_STAGES 1

typedef enum {
    CACHE_LATENCY_L1_HIT = 0,
    CACHE_LATENCY_L2_HIT,
    CACHE_LATENCY_L2_MISS
} cache_lentency_t;

typedef enum {
    OPCODE_ADD = 2,
    OPCODE_MUL,
    OPCODE_LOAD,
    OPCODE_STORE,
    OPCODE_BRANCH,
} opcode_t;

typedef enum {
    DRIVER_READ_OK = 0,
    DRIVER_READ_MISPRED,
    DRIVER_READ_ICACHE_MISS,
    DRIVER_READ_END_OF_TRACE,
} driver_read_status_t;

typedef struct {
    uint64_t pc;
    opcode_t opcode;
    int8_t dest;
    int8_t src1;
    int8_t src2;
    uint64_t load_store_addr;

    bool br_taken;
    uint64_t br_target;  
    cache_lentency_t icache_hit;
    cache_lentency_t dcache_hit;

    // the driver fill these fields for you
    bool mispredict;
    uint64_t dyn_instruction_count;
} inst_t;

// This config struct is populated by the driver for you
typedef struct {
    size_t num_rob_entries;
    size_t num_schedq_entries_per_fu;
    size_t num_alu_fus;
    size_t num_mul_fus;
    size_t num_lsu_fus;
    size_t fetch_width;
    size_t dispatch_width;
    size_t retire_width;
    bool misses_enabled;
} procsim_conf_t;

typedef struct {
    uint64_t cycles;
    uint64_t instructions_fetched;
    uint64_t instructions_retired;

    uint64_t no_fire_cycles;
    uint64_t rob_no_dispatch_cycles;
    uint64_t dispq_max_usage;
    uint64_t schedq_max_usage;
    uint64_t rob_max_usage;
    double dispq_avg_usage;
    double schedq_avg_usage;
    double rob_avg_usage;
    double ipc;
    
    // The driver populates the stats below for you
    uint64_t icache_misses;
    uint64_t dcache_misses;
    uint64_t num_branch_instructions;
    uint64_t branch_mispredictions; // you need to set *retired_mispredict_out correctly in stage_state_update to get this matched
    uint64_t instructions_in_trace;
} procsim_stats_t;


extern const inst_t *procsim_driver_read_inst(driver_read_status_t *driver_read_status_output);
extern void procsim_driver_update_predictor(uint64_t ip,
                                            bool is_taken,
                                            uint64_t dyncount);

// There is more information on these functions in procsim.cpp
extern void procsim_init(const procsim_conf_t *sim_conf,
                         procsim_stats_t *stats);
extern uint64_t procsim_do_cycle(procsim_stats_t *stats,
                                 bool *retired_mispredict_out);
extern void procsim_finish(procsim_stats_t *stats);

#endif
