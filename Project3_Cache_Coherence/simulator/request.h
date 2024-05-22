#ifndef REQUEST_H
#define REQUEST_H

#include "messages.h"
#include "types.h"
#include "settings.h"

class Request {
   public:
    Request(nodeID_t src, nodeID_t dest, blockID_t _block, message_t type);
    ~Request();

    nodeID_t srcID;
    nodeID_t destID;

    blockID_t block;

    message_t msg;

    timestamp_t req_time;

    int flits;
    int tof;

    void print_state();
};

#endif
