#include "node.h"
#include <iostream>

// #define OLDNTWK

Node::Node(bool isDir, nodeID_t id, Network *ntwk) {
    this->id = id;
    this->isDir = isDir;
    if (isDir) {
        dir = new DirectoryController(id);
        dir->my_node = this;
    } else {
        switch (settings.cputype) {
            case FICI_CPU:
                cpu = new CPU(id);
                break;
            default:
                cpu = NULL;
        }
        if (cpu == NULL) {
            fatal_error("Invalid CPU Type: %s\n", cputype_str[settings.cputype]);
        }
        cache = new Cache(id);
        cpu->my_cache = cache;
        cache->my_cpu = cpu;
        cache->my_node = this;
    }
    this->ntwk = ntwk;
    if (settings.nettype == RING_TOP) {
        outgoing_packets = new Request*[2](); // left and right
        incoming_packets = new Request*[2](); // left and right
        num_ports = 2;
    } else if (settings.nettype == XBAR_TOP) {
        outgoing_packets = new Request*[settings.num_procs + 1](); // one per target proc and 1 for directory node
        incoming_packets = new Request*[settings.num_procs + 1](); // one per target proc and 1 for directory node
        num_ports = settings.num_procs + 1;
    } else {
        fatal_error("unsupported network type\n");
    }
}

Node::~Node() {
    if (cpu) delete cpu;
    if (cache) delete cache;
    if (dir) delete dir;
    delete[] outgoing_packets;
    delete[] incoming_packets;
}

bool Node::isDone() {
    done = cpu ? cpu->isDone() : true;
    return done;
}

void Node::tick() {
    if (!isDone()) debug_printf("Tick for node %lu\n", id);
    if (!isDir) {
        cpu->tick();
        cache->tick();
    } else {
        dir->tick();
    }


#ifdef OLDNTWK
    // ntwk out queue processes one item per cycle
    if (!ntwk_out_queue.empty()) {
        Request *r = ntwk_out_queue.front();
        ntwk_out_queue.pop_front();
        // RING topology hard coded
        // nodeID_t target; set the target for each protocol
        // only digest 1 request per cycle to avoid complexity
            // realistically, want to do a FIFO until you cannot send due to unavailable output port
        if (settings.nettype == RING_TOP) {
            uint64_t distance = (r->destID + ntwk->num_nodes - id) % ntwk->num_nodes;
            nodeID_t target =
                (((distance > ntwk->num_nodes / 2) ? id - 1 : id + 1) + ntwk->num_nodes) % ntwk->num_nodes;
            if (target == r->destID) {
                // place in ntwk_in_next
                ntwk->nodes[target]->ntwk_in_next.push_back(r);
            } else {
                // place in ntwk_out_next
                ntwk->nodes[target]->ntwk_out_next.push_back(r);
            }
        } else if (settings.nettype == XBAR_TOP){
            ntwk->nodes[r->destID]->ntwk_in_next.push_back(r);
        } else {
            fatal_error("Unsupported Network Type\n");
        }
    }

    // ntwk in queue
    if (!ntwk_in_queue.empty()) {
        if (isDir) {
            Request *r = ntwk_in_queue.front();
            ntwk_in_queue.pop_front();
            dir->request_next.push_back(r);
        } else {
            if (cache->ntwk_in_next == NULL) {
                Request *r = ntwk_in_queue.front();
                ntwk_in_queue.pop_front();
                cache->ntwk_in_next = r;
            }
        }
    }
#else
    if (!ntwk_out_queue.empty()) {
        int sent = 0;
        for (auto it = ntwk_out_queue.begin(); it != ntwk_out_queue.end(); it++) {
            Request *r = *it;
            nodeID_t out_port = settings.num_procs + 2; // out of bounds for all topologies
            nodeID_t targetnode = settings.num_procs + 2;
            nodeID_t rcv_port = settings.num_procs + 2;
            if (settings.nettype == RING_TOP) {
                uint64_t distance = (r->destID + ntwk->num_nodes - id) % ntwk->num_nodes;
                targetnode = (((distance > ntwk->num_nodes / 2) ? id - 1 : id + 1) + ntwk->num_nodes) % ntwk->num_nodes;
                out_port = ((distance > ntwk->num_nodes / 2) ? 0 : 1);
                rcv_port = 1 - out_port;
            } else if (settings.nettype == XBAR_TOP) {
                out_port = r->destID;
                targetnode = r->destID;
                rcv_port = this->id;
            } else {
                fatal_error("Unsupported Network Type\n");
            }

            // check that I can output to that side and that the receiver is not currently receiving from me
            if (!outgoing_packets[out_port] && !ntwk->nodes[targetnode]->incoming_packets[rcv_port]) {
                outgoing_packets[out_port] = r;
                ntwk->nodes[targetnode]->incoming_packets[rcv_port] = r;
                r->tof = r->flits;
                debug_printf("\t\tPushing packet along network via %lu: ", targetnode);
                r->print_state();
                sent++;
            } else {
                debug_printf("\t\tUnable to send request this cycle due to port %lu unavailability: ", out_port);
                r->print_state();
                break;
            }
        }
        for (int i = 0; i < sent; i++) {
            ntwk_out_queue.pop_front();
        }
    }

    // incoming side goes into queue
    // data forwarding from request next since sim is artificially delaying by 1
    for (uint64_t i = 0; i < num_ports; i++) {
        if (incoming_packets[i]) {
            Request *r = incoming_packets[i];
            if (r->tof <= 0) { // done sending
                if (r->destID == this->id) {
                    ntwk_in_queue.push_back(r);
                    debug_printf("\tArrival TOF complete: ");
                    r->print_state();
                } else {
                    ntwk_out_queue.push_back(r);
                    debug_printf("\tRequest Completed 1 hop: ");
                    r->print_state();
                }
                incoming_packets[i] = NULL;
            }
        }
    }

    // ntwk in queue
    for (uint64_t i = 0; i < num_ports; i++) {
        if (!ntwk_in_queue.empty()) {
            if (isDir) {
                Request *r = ntwk_in_queue.front();
                ntwk_in_queue.pop_front();
                dir->request_next.push_back(r);
            } else {
                if (cache->ntwk_in_next == NULL) {
                    Request *r = ntwk_in_queue.front();
                    ntwk_in_queue.pop_front();
                    cache->ntwk_in_next = r;
                } else {
                    break; // cache can only process 1 incoming at a time
                }
            }
        }
    }

#endif

}

void Node::tock() {
    if (!isDir) {
        cpu->tock();
        cache->tock();
    } else {
        dir->tock();
    }

    // ntwk out next
    ntwk_out_queue.splice(ntwk_out_queue.end(), ntwk_out_next);


#ifndef OLDNTWK
    // progress output side
    for (uint64_t i = 0; i < num_ports; i++) {
        // debug_printf("checking port: %s\n", i == 0 ? "left" : "right");
        if (outgoing_packets[i]) {
            outgoing_packets[i]->tof--;
            if (outgoing_packets[i]->tof <= 0) {
                debug_printf("\tNode %lu, Send TOF complete: ", this->id);
                outgoing_packets[i]->print_state();
                outgoing_packets[i] = NULL;
            } else {
                debug_printf("\tNode %lu, TOF dec by 1: ", this->id);
                outgoing_packets[i]->print_state();
            }
        }
    }
#endif

    // // ntwk out next
    // while (!ntwk_out_next.empty()) {
    //     Request *r = ntwk_out_next.front();
    //     ntwk_out_next.pop_front();
    //     ntwk_out_queue.push_back(r);
    // }
    ntwk_in_queue.splice(ntwk_in_queue.end(), ntwk_in_next);
    // // ntwk in next
    // while (!ntwk_in_next.empty()) {
    //     Request *r = ntwk_in_next.front();
    //     ntwk_in_next.pop_front();
    //     ntwk_in_queue.push_back(r);
    // }
}
