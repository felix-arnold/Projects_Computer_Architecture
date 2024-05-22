#ifndef MOSI_AGENT_H
#define MOSI_AGENT_H

#include "agent.h"

/** Cache states.  */
typedef enum {
    MOSI_CACHE_I = 1,
    MOSI_CACHE_S,
    MOSI_CACHE_M,
    MOSI_CACHE_O,
    MOSI_CACHE_IS,
    MOSI_CACHE_SM,
    MOSI_CACHE_IM,
    MOSI_CACHE_OM,
} MOSI_cache_state_t;

class MOSI_Agent : public Agent {
   public:
    // Cache state for this cache block
    MOSI_cache_state_t state;

    MOSI_Agent(nodeID_t id, Cache *cache, blockID_t tag);
    ~MOSI_Agent();

    // sim will handle deletion/free
    void process_proc_request(Request *request);

    // sim will handle deletion/free
    void process_ntwk_request(Request *request);

    void print_state();

    void do_proc_I(Request *request);
    void do_proc_S(Request *request);
    void do_proc_M(Request *request);
    void do_proc_O(Request *request);
    void do_proc_in_transient(Request *request);
    void do_ntwk_I(Request *request);
    void do_ntwk_S(Request *request);
    void do_ntwk_M(Request *request);
    void do_ntwk_O(Request *request);
    void do_ntwk_IM(Request *request);
    void do_ntwk_IS(Request *request);
    void do_ntwk_SM(Request *request);
    void do_ntwk_OM(Request *request);
};

#endif
