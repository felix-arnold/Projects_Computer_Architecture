#include "request.h"

Request::Request(nodeID_t src, nodeID_t dest, blockID_t _block, message_t type) {
    srcID = src;
    destID = dest;
    block = _block;
    msg = type;
    flits = settings.header_flits;
    if (type == DATA || type == DATA_E || type == DATA_F || type == DATA_WB) {
        flits += settings.payload_flits;
    }
    tof = 0;
}

Request::~Request() {}

void Request::print_state() {
    debug_printf("Request {src: %lu, dest: %lu, block: 0x%lx, msg: %s, tof: %d}\n", srcID, destID, block, message_t_str[msg], tof);
}
