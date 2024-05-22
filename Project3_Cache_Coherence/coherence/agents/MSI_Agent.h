#ifndef MSI_AGENT_H
#define MSI_AGENT_H

#include "agent.h"

/** Cache states.  */
typedef enum {
    MSI_CACHE_I = 1,
    MSI_CACHE_S,
    MSI_CACHE_M,
    MSI_CACHE_IS,
    MSI_CACHE_SM,
    MSI_CACHE_IM
} MSI_cache_state_t;

class MSI_Agent : public Agent {
   public:
    // Cache state for this cache block
    MSI_cache_state_t state;

    MSI_Agent(nodeID_t id, Cache *cache, blockID_t tag);
    ~MSI_Agent();

    // sim will handle deletion/free
    void process_proc_request(Request *request);

    // sim will handle deletion/free
    void process_ntwk_request(Request *request);

    void print_state();

    void do_proc_I(Request *request);
    void do_proc_S(Request *request);
    void do_proc_M(Request *request);
    void do_proc_in_transient(Request *request);
    void do_ntwk_I(Request *request);
    void do_ntwk_S(Request *request);
    void do_ntwk_M(Request *request);
    void do_ntwk_IM(Request *request);
    void do_ntwk_IS(Request *request);
    void do_ntwk_SM(Request *request);
};

#endif
