#ifndef COUNTER_H
#define COUNTER_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t bits;
    uint64_t ctr;
} Counter_t;

// This constructor should initialize the counter (which is width bits) to
//       weakly not taken!
void Counter_init(Counter_t *x, uint64_t width);

// Transition in the Saturating Counter state diagram according to this branch
//       being taken or not taken
void Counter_update(Counter_t *x, bool taken);

// Return current counter value
uint64_t Counter_get(Counter_t *x);

// Return true if the current counter is weakly taken or greater
bool Counter_isTaken(Counter_t *x);

// Set counter value
void Counter_setCount(Counter_t *x, uint64_t count);

// Return true if the current counter is in the weakly taken or weakly not-taken states
bool Counter_isWeak(Counter_t *x);

// Set this counter to either weakly taken (if taken==true) or weakly not 
// taken (if taken==false).
void Counter_reset(Counter_t *x, bool taken);

#endif
