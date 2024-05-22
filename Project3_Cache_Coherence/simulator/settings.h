/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stddef.h>

#include "types.h"

#define MAX_SHARERS 16

#include "protocols.h"

typedef struct settings_struct {
    protocol_t protocol;
    nodeID_t num_procs;
    cputype_t cputype;
    nettype_t nettype;      // add mesh or torus?

    // memconf_t memconf;      // INDEPENDENT, STRIPED (not used)
    // size_t mem_channels;    // mult be a power of 2 and a factor of the number of nodes
    // size_t row_size;        // log2 bytes (12)
    size_t mem_latency;     // latency in cycles (100 * freq in GHz)

    size_t block_size;      // log2 bytes (6)

    size_t link_width;      // log2 link width in bytes (3)
    size_t payload_flits;   // num flits for payload with block data
                            // 1 << (block size - link width)
    size_t header_flits;    // 1 << (4 - link width). 16 byte header
                            // 8 byte address, 8 bytes for src, dest, opcode, etc


    // size_t cache_c;         // for eviction (not used)
    // size_t cache_b;
    // size_t cache_s;

    char *trace_dir;    

    settings_struct() : block_size(6) {}
} settings_t;

extern settings_t settings;

#endif  // SETTINGS_H
