/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef CPU_H
#define CPU_H

#include <list>

#include "cache.h"
#include "instruction.h"
#include "request.h"
#include "sim.h"
#include "types.h"

class Cache;
class Request;

class CPU {
   public:
    CPU(nodeID_t id);
    ~CPU();

    nodeID_t id;

    Request *cache_in_next = NULL;  // upcoming cache response
                                    // tick: cache sets this
                                    // tock: move this to in

    Request *cache_in = NULL;       // cache response
                                    // tick: consumes this
                                    // tock: set from in_next

    Cache *my_cache = NULL;         // where we send reqs for memory
                                    // tick: send a req to cpu_in_next
                                    // tock: do nothing

    bool outstanding_request = false;
    bool done = false;

    inst_t *instrs = NULL;  // array of "instructions"
    uint64_t curip = 0;     // current instruction
    uint64_t ninsts = 0;    // size of array

    void tick();
    void tock();
    bool isDone();

   private:
    void load_instrs();
};

#endif  // CPU_H
