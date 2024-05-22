/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef SIM_H
#define SIM_H

#include <assert.h>

#include <cstddef>
#include <list>

#include "settings.h"
#include "types.h"

#define Global_Clock sim->clock

class Network;

class Simulator {
   public:
    Simulator();
    ~Simulator();

    timestamp_t clock;
    bool done;
    Network *network;

    uint64_t cache_misses = 0;
    uint64_t cache_accesses = 0;
    uint64_t memory_reads = 0;
    uint64_t memory_writes = 0;
    uint64_t silent_upgrades = 0;
    uint64_t cache_to_cache_transfers = 0;

    void run(void);
    void dump_stats(void);
};

extern Simulator *sim;

void init_cleanup();

#endif
