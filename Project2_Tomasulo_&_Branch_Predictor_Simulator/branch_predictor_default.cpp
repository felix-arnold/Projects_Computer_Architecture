#include "branchsim.hpp"

#include "Counter.hpp"

void always_taken_init_predictor(branchsim_conf_t *sim_conf) {

}

bool always_taken_predict(branch_t *br) {
    return true;
}

void always_taken_update_predictor(branch_t *br) {

}

void always_taken_cleanup_predictor() {

}

Counter_t _default_counter;
void two_bit_counter_init_predictor(branchsim_conf_t *sim_conf) {
    Counter_init(&_default_counter, 2);
}

bool two_bit_counter_predict(branch_t *br) {
    return Counter_isTaken(&_default_counter);
}

void two_bit_counter_update_predictor(branch_t *br) {
    Counter_update(&_default_counter, br->is_taken);
}

void two_bit_counter_cleanup_predictor() {

}