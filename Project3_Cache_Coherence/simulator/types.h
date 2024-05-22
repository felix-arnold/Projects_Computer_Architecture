/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

void fatal_error(const char *fmt, ...);
void debug_printf(const char *fmt, ...);

extern const char *cputype_str[4];

extern const char *nettype_str[3];

typedef enum {
    CPU_TYPE = 0,
    FICI_CPU,
    FICO_CPU,
    FOCO_CPU
} cputype_t;

typedef enum {
    NET_TOP = 0,
    RING_TOP,
    XBAR_TOP
} nettype_t;

typedef uint64_t timestamp_t;

typedef uint64_t addr_t;     // a 64 bit address
typedef uint64_t blockID_t;  // a (64 - B) bit identifier for a cache block
typedef uint64_t nodeID_t;   // node specifier

#endif
