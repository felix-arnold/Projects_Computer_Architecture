#include "MESI_Agent.h"

#include "agent.h"

MESI_Agent::MESI_Agent(nodeID_t id, Cache *cache, blockID_t tag) : Agent(id, cache, tag) { state = MESI_CACHE_I; }

MESI_Agent::~MESI_Agent() {}

void MESI_Agent::process_proc_request(Request *request) {
    debug_printf("\t\tHandling proc request: ");
    request->print_state();
    debug_printf("\t\t\tCurrent state: ");
    print_state();
    switch (state) {
        case MESI_CACHE_I:
            do_proc_I(request);
            break;
        case MESI_CACHE_M:
            do_proc_M(request);
            break;
        case MESI_CACHE_S:
            do_proc_S(request);
            break;
        case MESI_CACHE_E:
            do_proc_E(request);
            break;
        case MESI_CACHE_IS:
        case MESI_CACHE_IM:
        case MESI_CACHE_SM:
        case MESI_CACHE_EM:
            do_proc_in_transient(request);
            break;

        default:
            print_state();
            fatal_error("Invalid Cache State for MESI Protocol\n");
    }
    debug_printf("\t\t\tNew state: ");
    print_state();
}

void MESI_Agent::process_ntwk_request(Request *request) {
    debug_printf("\t\tHandling ntwk request: ");
    request->print_state();
    debug_printf("\t\t\tCurrent state: ");
    print_state();
    switch (state) {
        case MESI_CACHE_I:
            do_ntwk_I(request);
            break;
        case MESI_CACHE_M:
            do_ntwk_M(request);
            break;
        case MESI_CACHE_S:
            do_ntwk_S(request);
            break;
        case MESI_CACHE_E:
            do_ntwk_E(request);
            break;
        case MESI_CACHE_IS:
            do_ntwk_IS(request);
            break;
        case MESI_CACHE_IM:
            do_ntwk_IM(request);
            break;
        case MESI_CACHE_SM:
            do_ntwk_SM(request);
            break;
        case MESI_CACHE_EM:
            do_ntwk_EM(request);
            break;

        default:
            print_state();
            fatal_error("Invalid Cache State for MESI Protocol\n");
    }
    debug_printf("\t\t\tNew state: ");
    print_state();
}

void MESI_Agent::print_state() {
    const char *block_states[9] = {"X", "I", "S", "E", "M", "IS", "IM", "SM", "EM"};
    debug_printf("MESI_Agent - state: %s\n", block_states[state]);
}

void MESI_Agent::do_proc_I(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            send_GETS(request->block);
            state = MESI_CACHE_IS;
            sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->block);
            state = MESI_CACHE_IM;
            sim->cache_misses++;
            break;
        default:
            fatal_error("MESI_Agent: I state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_proc_S(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            send_DATA_proc(request->block);
            break;
        case STORE:
            send_GETM(request->block);
            state = MESI_CACHE_SM;
            break;
        default:
            fatal_error("MESI_Agent: S state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_proc_M(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
        case STORE:
            send_DATA_proc(request->block);
            break;
        default:
            fatal_error("MESI_Agent: M state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_proc_E(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            send_DATA_proc(request->block);
            break;
        case STORE:
            send_GETX(request->block);
            state = MESI_CACHE_EM;
            break;
        default:
            fatal_error("MESI_Agent: E state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_proc_in_transient(Request *request) {
    switch (request->msg) {
        /* If the block is in a transient state that means it sent out a GET message
         * and is waiting on DATA. Therefore the processor should be waiting
         * on a pending request. Therefore we should not be getting any requests from
         * the processor.
         */
        case LOAD:
        case STORE:
            fatal_error("Should only have one outstanding request per processor!");
            break;
        default:
            fatal_error("MESI_Agent: Transient state shouldn't see this message\n");
    }
}

void MESI_Agent::do_ntwk_I(Request *request) {
    switch (request->msg) {
        default:
            fatal_error("MESI_Agent: I state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_S(Request *request) {
    switch (request->msg) {
        case REQ_INVALID:
            send_INVACK(request->block);
            state = MESI_CACHE_I;
            break;
        default:
            fatal_error("MESI_Agent: S state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_M(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESI_CACHE_I;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESI_CACHE_S;
            break;
        default:
            fatal_error("MESI_Agent: M state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_E(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESI_CACHE_I;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESI_CACHE_S;
            break;
        default:
            fatal_error("MESI_Agent: E state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_IM(Request *request) {
    switch (request->msg) {
        case DATA:
            send_DATA_proc(request->block);
            state = MESI_CACHE_M;
            break;
        default:
            fatal_error("MESI_Agent: IM state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_IS(Request *request) {
    switch (request->msg) {
        case DATA:
            send_DATA_proc(request->block);
            state = MESI_CACHE_S;
            break;
        case DATA_E:
            send_DATA_proc(request->block);
            state = MESI_CACHE_E;
            break;
        default:
            fatal_error("MESI_Agent: IS state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_SM(Request *request) {
    switch (request->msg) {
        case REQ_INVALID:
            send_INVACK(request->block);
            state = MESI_CACHE_IM;
            break;
        case ACK:
            send_DATA_proc(request->block);
            state = MESI_CACHE_M;
            break;
        default:
            fatal_error("MESI_Agent: SM state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESI_Agent::do_ntwk_EM(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_INVACK(request->block);
            state = MESI_CACHE_IM;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESI_CACHE_SM;
            break;
        case ACK:
            send_DATA_proc(request->block);
            state = MESI_CACHE_M;
            break;
        default:
            fatal_error("MESI_Agent: EM state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}
