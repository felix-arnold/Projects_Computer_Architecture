#ifndef AGENT_H
#define AGENT_H

#include "request.h"
#include "sim.h"
#include "types.h"

class Cache;

class Agent {
   public:
    blockID_t blockID;
    Cache* my_cache;
    Agent();
    Agent(nodeID_t id, Cache* cache, blockID_t tag);
    virtual ~Agent();

    nodeID_t id;

    virtual void process_proc_request(Request*){};  // called when a proc issues a memory request
    virtual void process_ntwk_request(Request*){};  // called when the node receives a message from the network

    virtual void print_state(){};

    void send_GETM(blockID_t block);
    void send_GETS(blockID_t block);
    void send_INVACK(blockID_t block);
    void send_GETX(blockID_t block);
    void send_DATA_dir(blockID_t block);
    void send_DATA_proc(blockID_t block);
};

#endif  // AGENT_H
