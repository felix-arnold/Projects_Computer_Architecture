#ifndef MOESIF_AGENT_H
#define MOESIF_AGENT_H

#include "agent.h"

/** Cache states.  */
typedef enum {
    MOESIF_CACHE_I = 1,
    MOESIF_CACHE_S,
    MOESIF_CACHE_E,
    MOESIF_CACHE_M,
    MOESIF_CACHE_O,
    MOESIF_CACHE_F,
    MOESIF_CACHE_IS,
    MOESIF_CACHE_IM,
    MOESIF_CACHE_SM,
    MOESIF_CACHE_EM,
    MOESIF_CACHE_FM,
    MOESIF_CACHE_OM
} MOESIF_cache_state_t;

class MOESIF_Agent : public Agent {
   public:
    // Cache state for this cache block
    MOESIF_cache_state_t state;

    MOESIF_Agent(nodeID_t id, Cache *cache, blockID_t tag);
    ~MOESIF_Agent();

    // sim will handle deletion/free
    void process_proc_request(Request *request);

    // sim will handle deletion/free
    void process_ntwk_request(Request *request);

    void print_state();

    void do_proc_I(Request *request);
    void do_proc_S(Request *request);
    void do_proc_E(Request *request);
    void do_proc_M(Request *request);
    void do_proc_F(Request *request);
    void do_proc_O(Request *request);
    void do_proc_in_transient(Request *request);
    void do_ntwk_I(Request *request);
    void do_ntwk_S(Request *request);
    void do_ntwk_E(Request *request);
    void do_ntwk_M(Request *request);
    void do_ntwk_F(Request *request);
    void do_ntwk_O(Request *request);
    void do_ntwk_IM(Request *request);
    void do_ntwk_IS(Request *request);
    void do_ntwk_SM(Request *request);
    void do_ntwk_EM(Request *request);
    void do_ntwk_FM(Request *request);
    void do_ntwk_OM(Request *request);
};

#endif
