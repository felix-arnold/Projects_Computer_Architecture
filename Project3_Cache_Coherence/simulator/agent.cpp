#include "agent.h"

#include "cache.h"
#include "node.h"

Agent::Agent() {}

Agent::Agent(nodeID_t id, Cache* cache, blockID_t tag) {
    my_cache = cache;
    blockID = tag;
    this->id = id;
}

Agent::~Agent() {}

void Agent::send_GETM(blockID_t block) {
    Request* getm = new Request(id, settings.num_procs, block, GETM);
    if (!getm) {
        // alloc fail
        fatal_error("Allocation for Request Failed\n");
    }
    debug_printf("\t\t\tSending Request: ");
    getm->print_state();
    my_cache->my_node->ntwk_out_next.push_back(getm);
}

void Agent::send_GETS(blockID_t block) {
    Request* gets = new Request(id, settings.num_procs, block, GETS);
    if (!gets) {
        // alloc fail
        fatal_error("Allocation for Request Failed\n");
    }
    debug_printf("\t\t\tSending Request: ");
    gets->print_state();
    my_cache->my_node->ntwk_out_next.push_back(gets);
}

void Agent::send_INVACK(blockID_t block) {
    Request* invack = new Request(id, settings.num_procs, block, INVACK);
    if (!invack) {
        // alloc fail
        fatal_error("Allocation for Request Failed\n");
    }
    debug_printf("\t\t\tSending Request: ");
    invack->print_state();
    my_cache->my_node->ntwk_out_next.push_back(invack);
}

void Agent::send_GETX(blockID_t block) {
    Request* su = new Request(id, settings.num_procs, block, GETX);
    if (!su) {
        // alloc fail
        fatal_error("Allocation for Request Failed\n");
    }
    debug_printf("\t\t\tSending Request: ");
    su->print_state();
    my_cache->my_node->ntwk_out_next.push_back(su);
}

void Agent::send_DATA_dir(blockID_t block) {
    Request* data = new Request(id, settings.num_procs, block, DATA);
    if (!data) {
        // alloc fail
        fatal_error("Allocation for Request Failed\n");
    }
    debug_printf("\t\t\tSending Request: ");
    data->print_state();
    my_cache->my_node->ntwk_out_next.push_back(data);
}

void Agent::send_DATA_proc(blockID_t block) {
    Request* data = new Request(id, id, block, DATA);
    if (!data) {
        // alloc fail
        fatal_error("Allocation for Request Failed\n");
    }
    debug_printf("\t\t\tSending Data to proc: ");
    data->print_state();
    my_cache->my_cpu->cache_in_next = data;
}
