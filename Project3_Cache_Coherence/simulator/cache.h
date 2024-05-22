#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>

#include <unordered_map>

#include "MESI_Agent.h"
#include "MI_Agent.h"
#include "MOESIF_Agent.h"
#include "MOSI_Agent.h"
#include "MSI_Agent.h"
#include "agent.h"
#include "cpu.h"
#include "node.h"
#include "protocols.h"
#include "settings.h"
#include "sim.h"
#include "types.h"

class Cache;
class CPU;
class Node;

typedef struct cache_entry_struct {
    blockID_t blockID;
    Agent *agent;
    Cache *my_cache;
    cache_entry_struct() : blockID(0ul), agent(NULL), my_cache(NULL) {}
    cache_entry_struct(nodeID_t id, Cache *cache, blockID_t tag, protocol_t protocol) {
        my_cache = cache;
        blockID = tag;
        switch (protocol) {
            case MI_PRO:
                agent = new MI_Agent(id, cache, tag);
                break;
            case MSI_PRO:
                agent = new MSI_Agent(id, cache, tag);
                break;
            case MESI_PRO:
                agent = new MESI_Agent(id, cache, tag);
                break;
            case MOSI_PRO:
                agent = new MOSI_Agent(id, cache, tag);
                break;
            case MOESIF_PRO:
                agent = new MOESIF_Agent(id, cache, tag);
                break;
            default:
                fatal_error("invalid coherence protocol\n");
                break;
        }
    }
    ~cache_entry_struct() {
        if (agent) delete agent;
    }
} cache_entry_t;

class Cache {
   public:
    Cache(nodeID_t id);
    ~Cache();

    nodeID_t id;

    std::unordered_map<blockID_t, cache_entry_t *> entries;

    Request *cpu_in_next = NULL;    // incoming cpu req
                                    // tick: set by cpu
                                    // tock: moves this to in

    Request *cpu_in = NULL;         // cpu req
                                    // tick: consumes this first
                                    // tock: set from in_next

    Request *ntwk_in_next = NULL;   // incoming ntwk req
                                    // tick: set by ntwk
                                    // tock: moves this to in

    Request *ntwk_in = NULL;        // ntwk req
                                    // tick: consumes this after cpu req
                                    // tock: set from in_next

    CPU *my_cpu;                    // where to send responses to cpu (cache_in_next)
                                    // tick: set cpu->cache_in_next
                                    // tock: do nothing

    Node *my_node;                  // where to send messages on ntwk
                                    // tick: set node->ntwk_out_next for any processed reqs
                                    // tock: do nothing

    void tick();
    void tock();

    cache_entry_t *get_entry(blockID_t block);
};

#endif  // CACHE_H
