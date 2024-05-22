// DON'T MODIFY ANY CODE IN THIS FILE!

#include <getopt.h>
#include <unistd.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "branchsim.hpp"
#include "procsim.hpp"

typedef struct {
    branchsim_conf_t b_sim_conf;
    procsim_conf_t p_sim_conf;
    FILE *trace;
    bool run_branchsim; // true: run branch predictor only, false: run proc sim
} all_conf_t;

typedef struct {
    branchsim_stats_t b_sim_stats;
    procsim_stats_t p_sim_stats;
} all_stats_t;

typedef enum {
    DRIVER_FUNCTION_SUCCESS = 0,
    DRIVER_FUNCTION_FAILURE
} DRIVER_FUNCTION_STATUS;

static void print_err_usage(const char *err)
{
    fprintf(stderr, "%s\n", err);
    fprintf(stderr, "./proc2sim -i <trace file> [Options]\n");

    fprintf(stderr, "-x run branch predictor only\n");

    fprintf(stderr, "branch predictor related options:\n");
    fprintf(stderr, "-b [Select the prediction scheme to use]\n");
    fprintf(stderr, "\t\t0: Always taken\n");
    fprintf(stderr, "\t\t1: Two bit counter\n");
    fprintf(stderr, "\t\t2: Yeh-Patt\n");
    fprintf(stderr, "\t\t3: Gshare\n");
    fprintf(stderr, "-p <log2 of the height of pattern table in Yeh-Patt Predictor>/<log2 of the height of table in Gshare Predictor>\n");
    fprintf(stderr, "-H <log2 of the height of history table in Yeh-Patt Predictor>\n");

    fprintf(stderr, "processor related options\n");
    fprintf(stderr, "-s <number of SchedQ entries per FU>\n");
    fprintf(stderr, "-a <number of ALU FUs>\n");
    fprintf(stderr, "-m <number of MUL FUs>\n");
    fprintf(stderr, "-l <number of load/store FUs>\n");
    fprintf(stderr, "-f <fetch width>\n");
    fprintf(stderr, "-d disables Cache Misses and Mispredictions\n");

    fprintf(stderr, "-h <Print this message>\n");
}

// Function to print the run configuration
static void print_branchsim_config(branchsim_conf_t *sim_conf) {
    printf("SIMULATION CONFIGURATION (BRANCH PREDICTOR)\n");
    printf("P:          %" PRIu64 "\n", sim_conf->P);
    printf("H:          %" PRIu64 "\n", sim_conf->H);
    const size_t str_buf_size = 64;
    char pred_string[str_buf_size];
    memset(&pred_string, 0, str_buf_size);
    switch (sim_conf->predictor) {
    case BRANCHSIM_CONF_PREDICTOR::ALWAYS_TAKEN:
        strncpy(pred_string, "ALWAYS_TAKEN", str_buf_size-1);
        break;
    case BRANCHSIM_CONF_PREDICTOR::TWO_BIT_COUNTER:
        strncpy(pred_string, "TWO_BIT_COUNTER", str_buf_size-1);
        break;
    case BRANCHSIM_CONF_PREDICTOR::YEHPATT:
        strncpy(pred_string, "Yeh-Patt", str_buf_size-1);
        break;
    case BRANCHSIM_CONF_PREDICTOR::GSHARE:
        strncpy(pred_string, "Gshare", str_buf_size-1);
        break;
    default:
        break;
    }
    printf("Predictor (M):  %s\n", pred_string);
}

// Function to print the simulation output (branchsim)
static void print_branchsim_output(branchsim_stats_t *sim_stats) {
    printf("\nSIMULATION OUTPUT\n");
    printf("Total Instructions:                 %" PRIu64 "\n", sim_stats->total_instructions);
    printf("Total Branch Instructions:          %" PRIu64 "\n", sim_stats->num_branch_instructions);
    printf("Branches Correctly Predicted:       %" PRIu64 "\n", sim_stats->num_branches_correctly_predicted);
    printf("Branches Mispredicted:              %" PRIu64 "\n", sim_stats->num_branches_mispredicted);
    printf("Fraction Branch Instructions:       %.8f\n", sim_stats->fraction_branch_instructions);
    printf("Branch Prediction accuracy:         %.8f\n", sim_stats->prediction_accuracy);
}

static void branchsim_conf_t_init(branchsim_conf_t *x) {
    x->P = 13;
    x->H = 12;
    x->predictor = BRANCHSIM_CONF_PREDICTOR::ALWAYS_TAKEN;
}

static void branchsim_stats_t_init(branchsim_stats_t *x) {
    memset(x, 0, sizeof(branchsim_stats_t));
}

static void procsim_conf_t_init(procsim_conf_t *x) {
    memset(x, 0, sizeof(procsim_conf_t));
    // Default configuration
    x->num_rob_entries = 32;
    x->num_schedq_entries_per_fu = 2;
    x->num_alu_fus = 3;
    x->num_mul_fus = 2;
    x->num_lsu_fus = 2;
    x->fetch_width = 4;
    x->dispatch_width = 4;
    x->retire_width = 4;
    x->misses_enabled = true;
}

static void procsim_stats_t_init(procsim_stats_t *x) {
    memset(x, 0, sizeof(procsim_stats_t));
}

static size_t n_insts;
static inst_t *insts;
static uint64_t fetch_inst_idx;

// Function to print the run configuration
static void print_procsim_config(procsim_conf_t *sim_conf) {
    printf("SIMULATION CONFIGURATION (PROCESSOR)\n");
    printf("ROB entries:  %lu\n", sim_conf->num_rob_entries);
    printf("Num. SchedQ entries per FU: %lu\n", sim_conf->num_schedq_entries_per_fu);
    printf("Num. ALU FUs: %lu\n", sim_conf->num_alu_fus);
    printf("Num. MUL FUs: %lu\n", sim_conf->num_mul_fus);
    printf("Num. LSU FUs: %lu\n", sim_conf->num_lsu_fus);
    printf("Fetch width:  %lu\n", sim_conf->fetch_width);
    printf("Dispatch width: %lu\n", sim_conf->dispatch_width);
    printf("Retire width: %lu\n", sim_conf->retire_width);
}

// Function to print the simulation output
static void print_procsim_output(procsim_stats_t *sim_stats) {
    printf("\nSIMULATION OUTPUT\n");
    printf("Cycles:                    %" PRIu64 "\n", sim_stats->cycles);
    printf("Trace instructions:        %" PRIu64 "\n", sim_stats->instructions_in_trace);
    printf("Fetched instructions:      %" PRIu64 "\n", sim_stats->instructions_fetched);
    printf("Retired instructions:      %" PRIu64 "\n", sim_stats->instructions_retired);
    printf("Total branch instructions: %" PRIu64 "\n", sim_stats->num_branch_instructions);
    printf("Branch Mispredictions:     %" PRIu64 "\n", sim_stats->branch_mispredictions);
    printf("I-cache misses:            %" PRIu64 "\n", sim_stats->icache_misses);
    printf("D-cache misses:            %" PRIu64 "\n", sim_stats->dcache_misses);
    printf("Cycles with no fires:      %" PRIu64 "\n", sim_stats->no_fire_cycles);
    printf("No dispatch cycles due to ROB:   %" PRIu64 "\n", sim_stats->rob_no_dispatch_cycles);
    printf("Max DispQ usage:           %" PRIu64 "\n", sim_stats->dispq_max_usage);
    printf("Average DispQ usage:       %.3f\n", sim_stats->dispq_avg_usage);
    printf("Max SchedQ usage:          %" PRIu64 "\n", sim_stats->schedq_max_usage);
    printf("Average SchedQ usage:      %.3f\n", sim_stats->schedq_avg_usage);
    printf("Max ROB usage:             %" PRIu64 "\n", sim_stats->rob_max_usage);
    printf("Average ROB usage:         %.3f\n", sim_stats->rob_avg_usage);
    printf("IPC:                       %.3f\n", sim_stats->ipc);
}

static bool procsim_misses_enabled = true;
static inst_t *read_entire_trace(FILE *trace, size_t *size_insts_out, procsim_conf_t *sim_conf, procsim_stats_t *sim_stats) {
    size_t size_insts = 0;
    size_t cap_insts = 0;
    inst_t *insts_arr = NULL;

    while (!feof(trace)) {
        if (size_insts == cap_insts) {
            size_t new_cap_insts = 2 * (cap_insts + 1);
            // redundant c++ cast #1
            inst_t *new_insts_arr = (inst_t *)realloc(insts_arr, new_cap_insts * sizeof *insts_arr);
            if (!new_insts_arr) {
                perror("realloc");
                goto error;
            }
            cap_insts = new_cap_insts;
            insts_arr = new_insts_arr;
        }

        inst_t *inst = insts_arr + size_insts;
        int br_taken, icache_hit, dcache_hit;
        int ret = fscanf(trace, "%" SCNx64 " %d %" SCNd8 " %" SCNd8 " %" SCNd8 " %" SCNx64 " %d %" SCNx64 " %d %d %" PRIu64 "\n",
            &inst->pc,
            (int *)&inst->opcode,
            &inst->dest,
            &inst->src1,
            &inst->src2,
            &inst->load_store_addr,
            &br_taken,
            &inst->br_target,
            &icache_hit,
            &dcache_hit,
            &inst->dyn_instruction_count);
        inst->br_taken = (bool)br_taken;
        inst->icache_hit = procsim_misses_enabled ? (cache_lentency_t)icache_hit : cache_lentency_t::CACHE_LATENCY_L1_HIT;
        inst->dcache_hit = procsim_misses_enabled ? (cache_lentency_t)dcache_hit : cache_lentency_t::CACHE_LATENCY_L1_HIT;
        // Register 0 is not a real destination
        if (inst->icache_hit != cache_lentency_t::CACHE_LATENCY_L1_HIT) {
            sim_stats->icache_misses++;
        }
        if (inst->dcache_hit != cache_lentency_t::CACHE_LATENCY_L1_HIT) {
            sim_stats->dcache_misses++;
        }
        

        inst->dest = (inst->dest == (int8_t)0) ? (int8_t)-1 : inst->dest; 

        if (ret == 11) {
            size_insts++;
        } else {
            if (ferror(trace)) {
                perror("fscanf");
            } else {
                fprintf(stderr, "could not parse line in trace (only %d input items matched). is it corrupt?\n", ret);
            }
            goto error;
        }
    }

    *size_insts_out = size_insts;
    return insts_arr;

    error:
    free(insts_arr);
    *size_insts_out = 0;
    return NULL;
}

static bool in_mispred = false;
static bool in_icache_miss = false;
static size_t icache_miss_ctr = 0;
static bool finished_miss = false;
static branch_predictor_base_t procsim_predictor;
const inst_t *procsim_driver_read_inst(driver_read_status_t *driver_read_status_output) {
    if (in_mispred) {
        *driver_read_status_output = driver_read_status_t::DRIVER_READ_MISPRED;
        return NULL;
    }
    if (in_icache_miss) {
        *driver_read_status_output = driver_read_status_t::DRIVER_READ_ICACHE_MISS;
        return NULL;
    }

    if (fetch_inst_idx >= n_insts) {
        *driver_read_status_output = driver_read_status_t::DRIVER_READ_END_OF_TRACE;
        return NULL;
    } else {
        *driver_read_status_output = driver_read_status_t::DRIVER_READ_OK;
        inst_t *ret = &insts[fetch_inst_idx];

        if (insts[fetch_inst_idx].icache_hit == cache_lentency_t::CACHE_LATENCY_L2_HIT) {
            if (!finished_miss) { // if didnt just finish a cache miss
                in_icache_miss = true;
                icache_miss_ctr = L2_HIT_CYCLES;
                finished_miss = false;
                *driver_read_status_output = driver_read_status_t::DRIVER_READ_ICACHE_MISS;
                return NULL; // can't give you an instruction that missed in cache
            } else {
                finished_miss = false; // reset state for icache misses
                // carry on to check other details
            }
        } else if (insts[fetch_inst_idx].icache_hit == cache_lentency_t::CACHE_LATENCY_L2_MISS) {
            if (!finished_miss) { // if didnt just finish a cache miss
                in_icache_miss = true;
                icache_miss_ctr = L2_MISS_CYCLES;
                finished_miss = false;
                *driver_read_status_output = driver_read_status_t::DRIVER_READ_ICACHE_MISS;
                return NULL; // can't give you an instruction that missed in cache
            } else {
                finished_miss = false; // reset state for icache misses
                // carry on to check other details
            }
        }

        if (ret->opcode == opcode_t::OPCODE_BRANCH) {
            branch_t br;
            br.ip = ret->pc;
            br.is_taken = ret->br_taken;
            br.inst_num = ret->dyn_instruction_count;
            // get mispredict
            bool prediction = procsim_predictor.predict(&br);
            ret->mispredict = (prediction != ret->br_taken) && procsim_misses_enabled;
            in_mispred = ret->mispredict; // apply mispredict
            // update predictor on retire
        } else {
            ret->mispredict = false;
        }

        fetch_inst_idx++;
        return ret;
    }
}

// update branch predictor
void procsim_driver_update_predictor(uint64_t ip, bool is_taken, uint64_t dyncount) {
    branch_t br;
    br.ip = ip;
    br.is_taken = is_taken;
    br.inst_num = dyncount;
    // branchsim_update_stats(prediction, &br, sim_stats);
    procsim_predictor.update_predictor(&br);
}

static void all_stats_init(all_stats_t *x) {
    branchsim_stats_t_init(&(x->b_sim_stats));
    procsim_stats_t_init(&(x->p_sim_stats));
}


// parse arguments and check input
static int parse_arguments(all_conf_t *x, FILE **trace, int argc, char *const argv[]) {
    x->run_branchsim = false; // default: proc sim
    *trace = NULL;
    branchsim_conf_t_init(&(x->b_sim_conf));
    procsim_conf_t_init(&(x->p_sim_conf));

    int opt;
    int pred_num = -1;
    while (-1 != (opt = getopt(argc, argv, "i:s:a:m:l:f:b:p:H:dxh"))) {
        switch (opt) {
            case 'b':
                pred_num = atoi(optarg);
                if (pred_num > 3) {
                    print_err_usage("Invalid predictor b option");
                    return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
                }
                x->b_sim_conf.predictor = (BRANCHSIM_CONF_PREDICTOR) pred_num;
                break;

            // case 'r':
                // x->p_sim_conf.num_rob_entries = atoi(optarg);
                // break;

            case 's':
                x->p_sim_conf.num_schedq_entries_per_fu = atoi(optarg);
                break;

            case 'a':
                x->p_sim_conf.num_alu_fus = atoi(optarg);
                break;

            case 'm':
                x->p_sim_conf.num_mul_fus = atoi(optarg);
                break;

            case 'l':
                x->p_sim_conf.num_lsu_fus = atoi(optarg);
                break;

            case 'f':
                x->p_sim_conf.fetch_width = atoi(optarg);
                break;

            // case 'w':
                // x->p_sim_conf.retire_width = atoi(optarg);
                // break;

            case 'p':
                x->b_sim_conf.P = (uint64_t) atoi(optarg);
                break;

            case 'H':
                x->b_sim_conf.H = (uint64_t) atoi(optarg);
                break;

            case 'i':
                *trace = fopen(optarg, "r");
                if (*trace == NULL) {
                    print_err_usage("Could not open the input trace file");
                    return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
                }
                break;

            case 'd':
                x->p_sim_conf.misses_enabled = false;
                procsim_misses_enabled = x->p_sim_conf.misses_enabled;
                break;

            case 'x':
                x->run_branchsim = true;
                break;

            case 'h':
                print_err_usage("");
                return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;

            default:
                print_err_usage("Invalid argument to program");
                return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
        }
    }

    // check conf
    if (!*trace) {
        print_err_usage("No trace file provided!");
        return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
    }

    if (x->b_sim_conf.P < 9) {
        print_err_usage("invalid configuration for P");
        return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
    }
    
    if (x->b_sim_conf.H < 9) {
        print_err_usage("invalid configuration for H");
        return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
    }

    x->p_sim_conf.num_rob_entries = x->p_sim_conf.fetch_width * 32;
    x->p_sim_conf.dispatch_width = x->p_sim_conf.fetch_width;
    x->p_sim_conf.retire_width = x->p_sim_conf.fetch_width;

    return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_SUCCESS;
}

static void assign_predictor(branch_predictor_base_t *x, BRANCHSIM_CONF_PREDICTOR pred_enum) {
    // default
    x->init_predictor = always_taken_init_predictor;
    x->predict = always_taken_predict;
    x->update_predictor = always_taken_update_predictor;
    x->cleanup_predictor = always_taken_cleanup_predictor;

    if (pred_enum == BRANCHSIM_CONF_PREDICTOR::ALWAYS_TAKEN) {
        x->init_predictor = always_taken_init_predictor;
        x->predict = always_taken_predict;
        x->update_predictor = always_taken_update_predictor;
        x->cleanup_predictor = always_taken_cleanup_predictor;
    } else if (pred_enum == BRANCHSIM_CONF_PREDICTOR::TWO_BIT_COUNTER) {
        x->init_predictor = two_bit_counter_init_predictor;
        x->predict = two_bit_counter_predict;
        x->update_predictor = two_bit_counter_update_predictor;
        x->cleanup_predictor = two_bit_counter_cleanup_predictor;
    } else if (pred_enum == BRANCHSIM_CONF_PREDICTOR::YEHPATT) {
        x->init_predictor = yeh_patt_init_predictor;
        x->predict = yeh_patt_predict;
        x->update_predictor = yeh_patt_update_predictor;
        x->cleanup_predictor = yeh_patt_cleanup_predictor;
    } else if (pred_enum == BRANCHSIM_CONF_PREDICTOR::GSHARE) {
        x->init_predictor = gshare_init_predictor;
        x->predict = gshare_predict;
        x->update_predictor = gshare_update_predictor;
        x->cleanup_predictor = gshare_cleanup_predictor;
    }
}

static int run_branchsim(branchsim_conf_t *sim_conf, branchsim_stats_t *sim_stats, FILE* const trace) {
    print_branchsim_config(sim_conf);
    printf("SETUP COMPLETE - STARTING SIMULATION\n");

    branch_predictor_base_t predictor;
    assign_predictor(&predictor, sim_conf->predictor);

    // Initialize the predictor
    predictor.init_predictor(sim_conf);

    branch_t br;
    size_t num_branches = 0;
    while (!feof(trace)) {
        // bool fields in branch_t
        int is_taken;

        // unused fields, just for parsing the trace
        opcode_t inst_opcode;
        int8_t inst_dest;
        int8_t inst_src1;
        int8_t inst_src2;
        uint64_t inst_load_store_addr;
        uint64_t inst_br_target;
        int inst_icache_hit;
        int inst_dcache_hit;

	uint64_t dyncount;
        int ret = fscanf(trace, "%" SCNx64 " %d %" SCNd8 " %" SCNd8 " %" SCNd8 " %" SCNx64 " %d %" SCNx64 " %d %d %" PRIu64 "\n",
            &br.ip,
            (int *)&inst_opcode,
            &inst_dest,
            &inst_src1,
            &inst_src2,
            &inst_load_store_addr,
            &is_taken,
            &inst_br_target,
            &inst_icache_hit,
            &inst_dcache_hit,
	    &dyncount);
        br.is_taken = (bool)is_taken;

        // Read branch from the input trace
        if (ret == 11) {
	        if (inst_opcode == 6) {
            	br.inst_num = dyncount;
            
            	bool prediction = predictor.predict(&br);
            	branchsim_update_stats(prediction, &br, sim_stats);
            	predictor.update_predictor(&br);

            	num_branches++;
	        }
            sim_stats->total_instructions = dyncount;
        } else {
            fprintf(stderr, "Corrupt trace: %" PRIu64 "\n", num_branches);
            return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
        }
    }

    // Function cleanup final statistics
    branchsim_finish_stats(sim_stats);

    // This calls the destructor of the predictor before freeing the allocated memory
    predictor.cleanup_predictor();

    print_branchsim_output(sim_stats);

    return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_SUCCESS;
}

static int run_procsim(all_conf_t *sim_conf, all_stats_t *sim_stats, FILE* const trace) {
    // initialize branch predictor
    assign_predictor(&procsim_predictor, sim_conf->b_sim_conf.predictor);
    // Initialize the predictor
    

    insts = read_entire_trace(trace, &n_insts, &(sim_conf->p_sim_conf), &(sim_stats->p_sim_stats));
    // fclose(trace);
    if (!insts) {
        fprintf(stderr, "Corrupt trace\n");
        return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE;
    }

    print_procsim_config(&(sim_conf->p_sim_conf));
    print_branchsim_config(&(sim_conf->b_sim_conf));
    procsim_predictor.init_predictor(&(sim_conf->b_sim_conf));
    // Initialize the processor
    procsim_init(&(sim_conf->p_sim_conf), &(sim_stats->p_sim_stats));
    printf("SETUP COMPLETE - STARTING SIMULATION\n");

    // We made this number up, but it should never take this many cycles to
    // retire something
    static const uint64_t max_cycles_since_last_retire = 256;
    uint64_t cycles_since_last_retire = 0;

    uint64_t retired_inst_idx = 0;
    fetch_inst_idx = 0;
    while (retired_inst_idx < n_insts) {
        bool retired_mispredict = false;

        uint64_t retired_this_cycle = procsim_do_cycle(&(sim_stats->p_sim_stats), &retired_mispredict);
        retired_inst_idx += retired_this_cycle;

        // Check for deadlocks (e.g., an empty submission)
        if (retired_this_cycle > 0) {
            cycles_since_last_retire = 0;
        } else {
            cycles_since_last_retire++;
        }
        if (cycles_since_last_retire == max_cycles_since_last_retire) {
            printf("\nIt has been %" PRIu64 " cycles since the last retirement."
                   " Does the simulator have a deadlock?\n",
                   max_cycles_since_last_retire);
            return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_SUCCESS; // return success to avoid interruption of the validation script
        }

        if (retired_mispredict) {
            fetch_inst_idx = retired_inst_idx;
            in_mispred = false;
        }

        if (icache_miss_ctr != 0) {
            icache_miss_ctr--;
        }
        if (icache_miss_ctr == 0 && in_icache_miss) {
            in_icache_miss = false;
            finished_miss = true;
        }
    }

    sim_stats->p_sim_stats.instructions_in_trace = n_insts;

    // Free memory and generate final statistics
    procsim_finish(&(sim_stats->p_sim_stats));
    free(insts);

    print_procsim_output(&(sim_stats->p_sim_stats));

    // branchsim_finish_stats(&(sim_stats->b_sim_stats));
    procsim_predictor.cleanup_predictor();

    return DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_SUCCESS;
}


int main(int argc, char *const argv[]) {
    all_conf_t acnf;
    all_stats_t asts;
    FILE *trace = NULL;

    all_stats_init(&asts);

    if (parse_arguments(&acnf, &trace, argc, argv) == DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE) {
        return 1;
    }

    if (acnf.run_branchsim) {
        if (run_branchsim(&(acnf.b_sim_conf), &(asts.b_sim_stats), trace) == DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE) {
            return 1;
        }
    } else {
        if (run_procsim(&acnf, &asts, trace) == DRIVER_FUNCTION_STATUS::DRIVER_FUNCTION_FAILURE) {
            return 1;
        }
    }
    fclose(trace);

    return 0;
}
