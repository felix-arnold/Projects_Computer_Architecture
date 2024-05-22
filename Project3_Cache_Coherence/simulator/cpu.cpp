#include "cpu.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

CPU::CPU(nodeID_t id) {
    this->id = id;
    this->done = false;
    curip = 0;
    load_instrs();
}

CPU::~CPU() {
    if (instrs) free(instrs);
}

void CPU::load_instrs() {
    curip = 0;
    ninsts = 0;
    size_t cap_insts = 0;

    char tracefile[100];
    sprintf(tracefile, "%s/p%d.trace", settings.trace_dir, (int)id);
    FILE* trace = fopen(tracefile, "r");

    if (trace == NULL) {
        perror("fopen");
    }

    while (!feof(trace)) {
        if (ninsts == cap_insts) {
            size_t new_cap = 2 * (cap_insts + 1);

            inst_t* new_instrs = (inst_t*)realloc(instrs, new_cap * sizeof(inst_t));
            if (!new_instrs) {
                perror("realloc");
                goto error;
            }
            cap_insts = new_cap;
            instrs = new_instrs;
        }

        inst_t* inst = instrs + ninsts;
        *inst = inst_t();
        int ret = fscanf(trace, "%c %" PRIx64 "\n", &inst->action, &inst->address);
        if (ret == 2) {
            ninsts++;

        } else {
            if (ferror(trace)) {
                perror("fscanf");
            } else {
                fatal_error("parse failed, is the file corrupt? %d, line %" PRIu64 "\n", ret, ninsts);
            }
            goto error;
        }
    }

    fclose(trace);
    return;

error:
    free(instrs);
    ninsts = 0;
    instrs = NULL;
}

void CPU::tick() {
    if (!isDone()) debug_printf("\tTick for cpu %lu\n", id);

    if (cache_in) {
        assert(cache_in->msg == DATA);
        debug_printf("\t\tCache responded to my request for block 0x%lx\n", cache_in->block);
        outstanding_request = false;
        delete cache_in;
        if (done) {
            debug_printf("\t\tThis node will not tick anymore\n");
        }
    }

    cache_in = NULL;

    if (done || outstanding_request) {
        return;
    }

    debug_printf("\t\tproc %d: instruction %d\n", (int)id, (int)curip);
    inst_t inst = instrs[curip];

    Request* req;
    switch (inst.action) {
        case READ:
            req = new Request(id, id, inst.address & ((uint64_t) -1 << settings.block_size), LOAD);
            break;
        case WRITE:
            req = new Request(id, id, inst.address & ((uint64_t) -1 << settings.block_size), STORE);
            break;
        default:
            fatal_error("Invalid Instruction\n");
            return;
    }

    my_cache->cpu_in_next = req;
    outstanding_request = true;
    sim->cache_accesses++;

    curip++;
    if (curip == ninsts) {
        done = true;
    }
    debug_printf("\t\tdemand request for block 0x%" PRIx64 " with action %s\n", req->block,
                 inst.action == 'r' ? "LOAD" : "STORE");
    if (done) {
        debug_printf("\t\tThis is the final instruction for this proc\n");
    }
}

void CPU::tock() {
    if (cache_in_next) {
        cache_in = cache_in_next;
        cache_in_next = NULL;
    }
}

bool CPU::isDone() { return done && !outstanding_request; }
