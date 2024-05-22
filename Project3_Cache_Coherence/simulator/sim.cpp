/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#include "sim.h"

#include <inttypes.h>
#include <stdio.h>

#include "network.h"

const char *proto_str[8] = {"CACHE_PRO", "MI_PRO",   "MSI_PRO",   "MESI_PRO",
                            "MOESI_PRO", "MOSI_PRO", "MESIF_PRO", "MOESIF_PRO"};
const char *cputype_str[4] = {"CPU_TYPE", "FICI_CPU", "FICO_CPU", "FOCO_CPU"};
const char *nettype_str[3] = {"NET_TOP", "RING_TOP", "XBAR_TOP"};

Simulator::Simulator() {
    clock = 0;
    network = new Network();
}

Simulator::~Simulator() { delete network; }

void Simulator::run() {
    static const timestamp_t max_cycles = 500000; // 500000
    printf("Starting simulation\n");
    done = false;
    while (!(done = network->isDone())) {
        debug_printf("======================== BEGIN CYCLE: %lu ========================\n", clock);
        debug_printf("Sim Tick\n");
        network->tick();
        debug_printf("Sim Tock\n");
        network->tock();
        debug_printf("======================== END CYCLE: %lu ========================\n", clock);
        clock++;

        if (!done && clock >= max_cycles) {
            fprintf(stderr, "Simulator has been running for %" PRIu64 " cycles and has not yet "
                        "finished. It should never need this many cycles --- is there a deadlock?\n",
                        clock);
            break;
        }
    }
}

void Simulator::dump_stats() {
    if (done) {
        printf("Simulation complete\n");
    } else {
        printf("Simulation terminated\n");
    }
    printf("Cycles: %lu\n", clock);
    printf("Cache Misses:     %8ld misses\n", cache_misses);
    printf("Cache Accesses:   %8ld accesses\n", cache_accesses);
    printf("Silent Upgrades:  %8ld upgrades\n", silent_upgrades);
    printf("$-to-$ Transfers: %8ld transfers\n", cache_to_cache_transfers);
    printf("Memory Reads:     %8ld reads\n", memory_reads);
    printf("Memory Writes:    %8ld writes\n", memory_writes);
}
