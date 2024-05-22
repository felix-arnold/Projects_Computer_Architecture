#ifndef MI_AGENT_H
#define MI_AGENT_H

#include "agent.h"

/** Cache states.  */
typedef enum {
    MI_CACHE_I = 1,
    MI_CACHE_M,
    MI_CACHE_IM,
} MI_cache_state_t;

class MI_Agent : public Agent {
   public:
    // Cache state for this cache block
    MI_cache_state_t state;

    MI_Agent(nodeID_t id, Cache *cache, blockID_t tag);
    ~MI_Agent();

    // sim will handle deletion/free
    void process_proc_request(Request *request);

    // sim will handle deletion/free
    void process_ntwk_request(Request *request);

    void print_state();

    /* Functions that specify the actions to take on requests from the processor
     * when the cache is in various states
     */
    void do_proc_I(Request *request);
    void do_proc_in_transient(Request *request);
    void do_proc_M(Request *request);

    /* Functions that specify the actions to take on fetch requests
     * when the cache is in various states
     */
    void do_ntwk_I(Request *request);
    void do_ntwk_IM(Request *request);
    void do_ntwk_M(Request *request);
};

#endif
