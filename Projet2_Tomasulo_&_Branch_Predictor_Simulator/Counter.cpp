#include "Counter.hpp"

void Counter_init(Counter_t *x, uint64_t width) {
    x->bits = width;
    x->ctr = (1UL << (width - 1)) - 1; // initialize to weakly not taken;
}

void Counter_update(Counter_t *x, bool taken) {
    if (x->ctr != 0 && !taken) { // if counter is not saturated at 0 and nt, decrement
        x->ctr -= 1;
    } else if (x->ctr < ((1UL << x->bits) - 1) && taken) { // if counter is not positively saturated and taken, increment
        x->ctr += 1;
    }
}

uint64_t Counter_get(Counter_t *x) {
    return x->ctr;
}

bool Counter_isTaken(Counter_t *x) {
    return x->ctr >= (1UL << (x->bits - 1));
}

void Counter_setCount(Counter_t *x, uint64_t count) {
    x->ctr = count;
}

bool Counter_isWeak(Counter_t *x) {
    return (x->ctr == (1UL << (x->bits - 1))) || (x->ctr == (1UL << (x->bits - 1)) - 1);
}

void Counter_reset(Counter_t *x, bool taken) {
    uint64_t weak_taken = (1UL << (x->bits - 1));
    x->ctr = weak_taken + (taken ? 0 : -1);
}
