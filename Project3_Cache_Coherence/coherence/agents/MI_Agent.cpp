#include "MI_Agent.h"

#include "agent.h"

MI_Agent::MI_Agent(nodeID_t id, Cache *cache, blockID_t tag) : Agent(id, cache, tag) { state = MI_CACHE_I; }

MI_Agent::~MI_Agent() {}

void MI_Agent::process_proc_request(Request *request) {
    debug_printf("\t\tHandling proc request: ");
    request->print_state();
    debug_printf("\t\t\tCurrent state: ");
    print_state();
    switch (state) {
        case MI_CACHE_I:
            do_proc_I(request);
            break;
        case MI_CACHE_IM:
            do_proc_in_transient(request);
            break;
        case MI_CACHE_M:
            do_proc_M(request);
            break;
        default:
            print_state();
            fatal_error("Invalid Cache State for MI Protocol\n");
    }
    debug_printf("\t\t\tNew state: ");
    print_state();
}

void MI_Agent::process_ntwk_request(Request *request) {
    debug_printf("\t\tHandling ntwk request: ");
    request->print_state();
    debug_printf("\t\tCurrent state: ");
    print_state();
    switch (state) {
        case MI_CACHE_I:
            do_ntwk_I(request);
            break;
        case MI_CACHE_IM:
            do_ntwk_IM(request);
            break;
        case MI_CACHE_M:
            do_ntwk_M(request);
            break;
        default:
            print_state();
            fatal_error("Invalid Cache State for MI Protocol\n");
    }
    debug_printf("\t\t\tNew state: ");
    print_state();
}

void MI_Agent::print_state() {
    const char *block_states[4] = {"X", "I", "M", "IM"};
    debug_printf("MI_Agent - state: %s\n", block_states[state]);
}

void MI_Agent::do_proc_I(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
        case STORE:
            send_GETM(request->block);
            /* The IM state means that we have sent the GET message and we are now waiting
             * on DATA
             */
            state = MI_CACHE_IM;
            /* This is a cache miss */
            sim->cache_misses++;
            break;
        default:
            fatal_error("MI_Agent: I state shouldn't see this message from proc\n");
    }
}

void MI_Agent::do_proc_in_transient(Request *request) {
    switch (request->msg) {
        /* If the block is in the IM state that means it sent out a GET message
         * and is waiting on DATA.	Therefore the processor should be waiting
         * on a pending request. Therefore we should not be getting any requests from
         * the processor.
         */
        case LOAD:
        case STORE:
            fatal_error("Should only have one outstanding request per processor!");
            break;
        default:
            fatal_error("MI_Agent: IM state shouldn't see this message\n");
    }
}

void MI_Agent::do_proc_M(Request *request) {
    switch (request->msg) {
        /* The M state means we have the data and we can modify it.  Therefore any request
         * from the processor (read or write) can be immediately satisfied.
         */
        case LOAD:
        case STORE:
            // This is how you send data back to the processor to finish the request
            // Note: There was no need to send anything on the network on a hit.
            send_DATA_proc(request->block);
            break;
        default:
            fatal_error("MI_Agent: M state shouldn't see this message from proc\n");
    }
}

void MI_Agent::do_ntwk_I(Request *request) {
    switch (request->msg) {
        default:
            fatal_error("MI_Agent: I state shouldn't see this message from ntwk\n");
    }
}

void MI_Agent::do_ntwk_IM(Request *request) {
    switch (request->msg) {
        case DATA:
            /** IM state meant that the block had sent the GET and was waiting on DATA.
             * Now that Data is received we can send the DATA to the processor and finish
             * the transition to M.
             */
            send_DATA_proc(request->block);
            state = MI_CACHE_M;
            break;
        default:
            fatal_error("MI_Agent: IM state shouldn't see this message from ntwk\n");
    }
}

void MI_Agent::do_ntwk_M(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MI_CACHE_I;
            break;
        default:
            fatal_error("MI_Agent: M state shouldn't see this message from ntwk\n");
    }
}
