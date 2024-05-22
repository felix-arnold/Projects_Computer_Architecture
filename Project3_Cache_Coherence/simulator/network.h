/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <list>
#include <vector>

#include "node.h"
#include "sim.h"

class Node;

class Network {
   public:
    Network();
    ~Network();

    std::vector<Node*> nodes;  // first N are procs, last is directory
    uint64_t num_nodes;

    void tick();
    void tock();

    bool isDone();
};

#endif  // NETWORK_H
