#ifndef BRANCHSIM_H
#define BRANCHSIM_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <complex>
#include <iostream>
#include <bitset>

#include "Counter.hpp"

typedef struct {
    uint64_t ip; // ip = instruction pointer aka. address of the branch instruction or PC
    uint64_t inst_num; // Instruction count of the branch
    bool is_taken;  // Is the branch actually taken
} branch_t;

typedef enum {
    ALWAYS_TAKEN = 0,
    TWO_BIT_COUNTER,
    YEHPATT,
    GSHARE,
} BRANCHSIM_CONF_PREDICTOR;

// Structure to hold configuration details for a branchsim simulation run
typedef struct {
    // Global Parameters:
    BRANCHSIM_CONF_PREDICTOR predictor; // The predictor being used 1: Yeh-Patt, 2: Perceptron, 3: Tournament between the two

    
    uint64_t P; // log2 Height of the Pattern Table (Yeh Patt & Gshare)
    uint64_t H; // log2 Height of the Local History Table (Yeh Patt only)

} branchsim_conf_t;

// Structure to hold statistics for a branchsim simulation run
typedef struct {
    uint64_t total_instructions;            // Use the inst num field of the final branch
    uint64_t num_branch_instructions;
    uint64_t num_branches_correctly_predicted;
    uint64_t num_branches_mispredicted;

    double fraction_branch_instructions;    // fraction of the entire program that is branches
    double prediction_accuracy;
} branchsim_stats_t;

// Base class for a branch predictor
typedef void (*init_predictor_func_ptr)(branchsim_conf_t *);
typedef bool (*predict_func_ptr)(branch_t *);
typedef void (*update_predictor_func_ptr)(branch_t *);
typedef void (*cleanup_predictor_func_ptr)();

typedef struct {
    init_predictor_func_ptr init_predictor;
    predict_func_ptr predict;
    update_predictor_func_ptr update_predictor;
    cleanup_predictor_func_ptr cleanup_predictor;
} branch_predictor_base_t;

// Function to update branch prediction statistics after the prediction
extern void branchsim_update_stats(bool prediction, branch_t *br, branchsim_stats_t *sim_stats);

// Function to perform cleanup and final statistics calculations, etc
extern void branchsim_finish_stats(branchsim_stats_t *sim_stats);

extern void always_taken_init_predictor(branchsim_conf_t *sim_conf);
extern bool always_taken_predict(branch_t *branch);
extern void always_taken_update_predictor(branch_t *branch);
extern void always_taken_cleanup_predictor();

extern void two_bit_counter_init_predictor(branchsim_conf_t *sim_conf);
extern bool two_bit_counter_predict(branch_t *branch);
extern void two_bit_counter_update_predictor(branch_t *branch);
extern void two_bit_counter_cleanup_predictor();

extern void yeh_patt_init_predictor(branchsim_conf_t *sim_conf);
extern bool yeh_patt_predict(branch_t *branch);
extern void yeh_patt_update_predictor(branch_t *branch);
extern void yeh_patt_cleanup_predictor();

extern void gshare_init_predictor(branchsim_conf_t *sim_conf);
extern bool gshare_predict(branch_t *branch);
extern void gshare_update_predictor(branch_t *branch);
extern void gshare_cleanup_predictor();

#endif