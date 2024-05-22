#include "network.h"

#include <stdio.h>

Network::Network() {
    for (nodeID_t i = 0; i < settings.num_procs; i++) {
        debug_printf("Creating cpu on node:       %lu\n", i);
        nodes.push_back(new Node(false, i, this));  // create n cpus
    }
    debug_printf("Creating directory on node: %lu\n", settings.num_procs);
    nodes.push_back(new Node(true, settings.num_procs, this));  // create 1 directory
    num_nodes = nodes.size();
}

Network::~Network() {
    for (auto &n : nodes) {
        delete n;
    }
    nodes.clear();
}

bool Network::isDone() {
    bool done = true;
    for (auto n = nodes.begin(); n != nodes.end(); n++) {
        done = done && (*n)->isDone();
        if (!done) break;
    }
    return done;
}

void Network::tick() {
    for (auto n = nodes.begin(); n != nodes.end(); n++) {
        (*n)->tick();
    }
}

void Network::tock() {
    for (auto n = nodes.begin(); n != nodes.end(); n++) {
        (*n)->tock();
    }
}
