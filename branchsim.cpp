#include "branchsim.hpp"

uint64_t p;
uint64_t h;
uint64_t pattern_index;
uint64_t history_index;
uint64_t ghr;

std::unique_ptr<std::unordered_map<uint64_t, Counter_t>> pattern_table(new std::unordered_map<uint64_t, Counter_t>());
std::unique_ptr<std::unordered_map<uint64_t, uint64_t>> history_table(new std::unordered_map<uint64_t, uint64_t>());

// Initialize Yeh-Patt predictor
void yeh_patt_init_predictor(branchsim_conf_t *sim_conf) {

    p = sim_conf->P;
    h = sim_conf->H;

    // create history table of 2^H entries (all entries initialized to 0)
    for (int i = 0; i < pow(2, h); i++)
        history_table->emplace(i, 0);
    
    // create pattern table of 2^P entries init to weakly not taken
    for (int i = 0; i < pow(2, p); i++) {
        pattern_table->emplace(i, Counter_t{});
        Counter_init(&(pattern_table->at(i)), 2);
    }

#ifdef DEBUG
    printf("Yeh-Patt: Creating a history table of %" PRIu64 " entries of length %" PRIu64 "\n", uint64_t(pow(2, h)), p);
    printf("Yeh-Patt: Creating a pattern table of %" PRIu64 " 2-bit saturating counters\n", uint64_t(pow(2, p)));
#endif
}

// Predict using Yeh-Patt predictor
bool yeh_patt_predict(branch_t *br) {
#ifdef DEBUG
    printf("\tYeh-Patt: Predicting... \n");
#endif

    history_index = ((br->ip >> 2) ^ ghr) & ((1UL << h) - 1);
    pattern_index = history_table->at(history_index);
    
#ifdef DEBUG
    printf("\t\tHT index: 0x%" PRIx64 ", History: 0x%" PRIx64 ", PT index: 0x%" PRIx64 ", Prediction: %d\n", history_index, history_table->at(history_index), pattern_index, Counter_isTaken(&(pattern_table->at(pattern_index))));
#endif

    return Counter_isTaken(&(pattern_table->at(pattern_index)));
}

// Update Yeh-Patt predictor
void yeh_patt_update_predictor(branch_t *br) {
#ifdef DEBUG
    printf("\tYeh-Patt: Updating based on actual behavior: %d\n", (int) br->is_taken);
    uint64_t old_history = history_table->at(history_index);
#endif

    Counter_update(&(pattern_table->at(pattern_index)), br->is_taken);
    history_table->at(history_index) = ((pattern_index << 1) + br->is_taken) & (~(1UL << p));

#ifdef DEBUG
    printf("\t\tHT index: 0x%" PRIx64 ", History: 0x%" PRIx64 ", PT index: 0x%" PRIx64 ", New Counter: 0x%" PRIx64 ", New History: 0x%" PRIx64 "\n", history_index, old_history, pattern_index, Counter_get(&(pattern_table->at(pattern_index))), history_table->at(history_index));
#endif
}

// Cleanup Yeh-Patt predictor, not useful here
void yeh_patt_cleanup_predictor() {
    // add your code here
}


// Initialize Gshare predictor
void gshare_init_predictor(branchsim_conf_t *sim_conf) {

    p = sim_conf->P;

    // create pattern table of 2^P entries init to weakly not taken
    for (int i = 0; i < pow(2, p); i++) {
        pattern_table->emplace(i, Counter_t{});
        Counter_init(&(pattern_table->at(i)), 2);
    }

    // initialize GHR to 0
    ghr = 0;

#ifdef DEBUG
    printf("Gshare: Creating a pattern table of %" PRIu64 " 2-bit saturating counters\n", uint64_t(pow(2, p)));
#endif
}

// Predict using Gshare predictor
bool gshare_predict(branch_t *br) {
#ifdef DEBUG
    printf("\tGshare: Predicting... \n");
#endif

    pattern_index = ((br->ip >> 2) ^ ghr) & ((1UL << p) - 1);
    
#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", Prediction: %d\n", (ghr << (64 - p)) >> (64 - p), pattern_index, Counter_isTaken(&(pattern_table->at(pattern_index))));
#endif

    return Counter_isTaken(&(pattern_table->at(pattern_index)));
}

// Update Gshare predictor
void gshare_update_predictor(branch_t *br) {
#ifdef DEBUG
    printf("\tGshare: Updating based on actual behavior: %d\n", (int) br->is_taken);
    uint64_t old_ghr = ghr;
#endif

    pattern_index = ((br->ip >> 2) ^ ghr) & ((1UL << p) - 1);

    Counter_update(&(pattern_table->at(pattern_index)), br->is_taken);
    ghr = (ghr << 1) + br->is_taken;

#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", New Counter value: 0x%" PRIx64 ", New History: 0x%" PRIx64 "\n", (old_ghr << (64 - p)) >> (64 - p), pattern_index, Counter_get(&(pattern_table->at(pattern_index))), (ghr << (64 - p)) >> (64 - p));
#endif
}

// Cleanup Gshare predictor, not useful here
void gshare_cleanup_predictor() {
    // add your code here
}

/**
 *  Function to update the branchsim stats per prediction
 *
 *  @param prediction The prediction returned from the predictor's predict function
 *  @param br Pointer to the branch_t that is being predicted -- contains actual behavior
 *  @param sim_stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_update_stats(bool prediction, branch_t *br, branchsim_stats_t *sim_stats) {
    
    sim_stats->num_branch_instructions++;
    
    if (prediction == br->is_taken)
        sim_stats->num_branches_correctly_predicted ++;
    else 
        sim_stats->num_branches_mispredicted++;
}

/**
 *  Function to cleanup branchsim statistic computations such as prediction rate, etc.
 *
 *  @param sim_stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_finish_stats(branchsim_stats_t *sim_stats) {
    sim_stats->fraction_branch_instructions = ((double) sim_stats->num_branch_instructions / sim_stats->total_instructions);
    sim_stats->prediction_accuracy = ((double) sim_stats->num_branches_correctly_predicted / sim_stats->num_branch_instructions);
}