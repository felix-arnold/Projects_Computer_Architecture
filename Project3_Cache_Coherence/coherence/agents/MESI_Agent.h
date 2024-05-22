#ifndef MESI_AGENT_H
#define MESI_AGENT_H

#include "agent.h"

/** Cache states.  */
typedef enum {
    MESI_CACHE_I = 1,
    MESI_CACHE_S,
    MESI_CACHE_E,
    MESI_CACHE_M,
    MESI_CACHE_IS,
    MESI_CACHE_IM,
    MESI_CACHE_SM,
    MESI_CACHE_EM
} MESI_cache_state_t;

class MESI_Agent : public Agent {
   public:
    // Cache state for this cache block
    MESI_cache_state_t state;

    MESI_Agent(nodeID_t id, Cache *cache, blockID_t tag);
    ~MESI_Agent();

    // sim will handle deletion/free
    void process_proc_request(Request *request);

    // sim will handle deletion/free
    void process_ntwk_request(Request *request);

    void print_state();

    void do_proc_I(Request *request);
    void do_proc_S(Request *request);
    void do_proc_E(Request *request);
    void do_proc_M(Request *request);
    void do_proc_in_transient(Request *request);
    void do_ntwk_I(Request *request);
    void do_ntwk_S(Request *request);
    void do_ntwk_E(Request *request);
    void do_ntwk_M(Request *request);
    void do_ntwk_IM(Request *request);
    void do_ntwk_IS(Request *request);
    void do_ntwk_SM(Request *request);
    void do_ntwk_EM(Request *request);
};

#endif
