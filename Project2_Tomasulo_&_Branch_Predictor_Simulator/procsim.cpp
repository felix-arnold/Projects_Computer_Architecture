#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <list>
#include <queue>
#include <iostream>
#include <array>
#include <tuple>

#include "procsim.hpp"

#define NUM_REGS 32
#define NUM_MATS 64

typedef struct {
    const inst_t* inst_ptr;
    opcode_t opcode;
    int8_t dest_reg;
    uint64_t dest_reg_tag;
    bool src1_rdy;
    uint64_t src1_tag;
    bool src2_rdy;
    uint64_t src2_tag;
    uint64_t ms_tag;
    bool ms_rdy;
    uint64_t md_tag;
    bool dispatched;
} res_sta_t;

std::queue<const inst_t*> dispatch_queue;
uint64_t fetch_width;
uint64_t num_rob;
std::vector<res_sta_t> sched_queue;
uint64_t rs_num;
uint64_t dispatch_width;
std::array<std::pair<bool, uint64_t>, NUM_REGS> register_file;
std::array<std::pair<bool, uint64_t>, NUM_MATS> mem_alias_table;
uint64_t tag_counter = 96;

std::vector<uint64_t> alu_index;
std::vector<std::array<uint64_t, 3>> mul_index;
std::vector<std::array<uint64_t, 2>> lsu_index;

std::vector<std::pair<const inst_t* , bool>> reorder_buffer;

// The helper functions in this#ifdef are optional and included here for your
// convenience so you can spend more time writing your simulator logic and less
// time trying to match debug trace formatting! (If you choose to use them)
#ifdef DEBUG
static void print_operand(int8_t rx) {
    if (rx < 0) {
        printf("(none)"); //  PROVIDED
    } else {
        printf("R%" PRId8, rx); //  PROVIDED
    }
}

// Useful in the fetch and dispatch stages
static void print_instruction(const inst_t *inst) {
    if (!inst) {
        return;
    }
    static const char *opcode_names[] = {NULL, NULL, "ADD", "MUL", "LOAD", "STORE", "BRANCH"};

    printf("opcode=%s, dest=", opcode_names[inst->opcode]); // PROVIDED
    print_operand(inst->dest); // PROVIDED
    printf(", src1="); // PROVIDED
    print_operand(inst->src1); // PROVIDED
    printf(", src2="); // PROVIDED
    print_operand(inst->src2); // PROVIDED
    printf(", dyncount=%lu", inst->dyn_instruction_count); //  PROVIDED
}

// This will print the state of the ROB where instructions are identified by their dyn_instruction_count
static void print_rob(void) {
    size_t printed_idx = 0;
    printf("\tAllocated Entries in ROB: %lu\n", reorder_buffer.size());
    for (auto rob_entry : reorder_buffer) { 
        if (printed_idx == 0) {
            printf("    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d }", rob_entry.first->dyn_instruction_count, rob_entry.second, rob_entry.first->mispredict); 
        } else if (!(printed_idx & 0x3)) {
            printf("\n    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d }", rob_entry.first->dyn_instruction_count, rob_entry.second, rob_entry.first->mispredict); 
        } else {
            printf(", { dyncount=%05" PRIu64 " completed: %d, mispredict: %d }", rob_entry.first->dyn_instruction_count, rob_entry.second, rob_entry.first->mispredict); 
        }
        printed_idx++;
    }
    if (!printed_idx) {
        printf("    (ROB empty)"); //  PROVIDED
    }
    printf("\n"); //  PROVIDED
}

static void print_messy_rf(void) {
    for (size_t i = 0; i < NUM_REGS; i++) {
        if (!(i & 0x3)) {
            printf("\t");
        } else {
            printf(", ");
        }
            printf("R%" PRIu64 "={tag: %" PRIu64 ", ready: %d}", i, register_file[i].second, register_file[i].first);
        if ((!((i+1) & 0x3)) && ((i+1) != NUM_REGS)) {
            printf("\n"); // PROVIDED
        }
    }
    printf("\n"); // PROVIDED
}

static void print_mat(void) {
    for (size_t i = 0; i < NUM_MATS; i++) {
        if (!(i & 0x3)) {
            printf("\t");
        } else {
            printf(", ");
        }
            printf("M%" PRIu64 "={tag: %" PRIu64 ", ready: %d}", i, mem_alias_table[i].second, mem_alias_table[i].first);
        if ((!((i+1) & 0x3)) && ((i+1) != NUM_MATS)) {
            printf("\n");
        }
    }
    printf("\n"); // PROVIDED
}
#endif


// Optional helper function which retires instructions in ROB in program
// order. (In a real system, the destination register value from the ROB would be written to the
// architectural registers, but we have no register values in this
// simulation.) This function returns the number of instructions retired.
// Immediately after retiring a mispredicting branch, this function will set
// *retired_mispredict_out = true and will not retire any more instructions. 
// Note that in this case, the mispredict must be counted as one of the retired instructions.
static uint64_t stage_state_update(procsim_stats_t *stats, bool *retired_mispredict_out) {
    
#ifdef DEBUG
    printf("Stage State Update: \n");
    printf("Checking ROB: \n");
#endif

    uint64_t retired_instr = 0;
    *retired_mispredict_out = false;
    
    for (uint64_t i = 0; i < fetch_width; i++) {
        if (reorder_buffer.size() > 0) {
            if (reorder_buffer.front().second) {
            #ifdef DEBUG
                printf("\tRetiring: ");
                print_instruction(reorder_buffer.front().first);
                printf("\n");
            #endif

                stats->instructions_retired++;
                retired_instr++;

                if (reorder_buffer.front().first->opcode == OPCODE_BRANCH) {
                    procsim_driver_update_predictor(reorder_buffer.front().first->pc, reorder_buffer.front().first->br_taken, reorder_buffer.front().first->dyn_instruction_count);
                    stats->num_branch_instructions++;
                    if (reorder_buffer.front().first->mispredict) {
                        *retired_mispredict_out = true;
                        reorder_buffer.erase(reorder_buffer.begin());
                        break;
                    }
                }
                reorder_buffer.erase(reorder_buffer.begin());
            }
            else {
            #ifdef DEBUG
                printf("\tROB entry still in flight: dyncount=%lu\n", reorder_buffer.front().first->dyn_instruction_count);
            #endif
                break;
            }
        }
        else
            break;
    }
#ifdef DEBUG
    printf("\t%" PRIu64 " instructions retired\n", retired_instr);
#endif

    return retired_instr;
}

// Optional helper function which is responsible for moving instructions
// through pipelined Function Units and then when instructions complete (that
// is, when instructions are in the final pipeline stage of an FU and aren't
// stalled there), setting the ready bits in the register file. This function 
// should remove an instruction from the scheduling queue when it has completed.
static void stage_exec(procsim_stats_t *stats) {

#ifdef DEBUG
    printf("Stage Exec: \n"); 
#endif

    std::vector<uint64_t> completed_dyncounts;

    // Progress ALUs
#ifdef DEBUG
    printf("\tProgressing ALU units\n"); 
#endif
    for (uint64_t i = 0; i < alu_index.size(); i++) {
        if (alu_index[i] != UINT64_MAX) {
        #ifdef DEBUG
            printf("\tCompleting ALU: %ld, for dyncount=%lu\n", i, alu_index[i]);
        #endif
            completed_dyncounts.push_back(alu_index[i]);
            alu_index[i] = UINT64_MAX;
        }
    }

    // Progress MULs
#ifdef DEBUG
    printf("\tProgressing MUL units\n");
#endif
    for (uint64_t i = 0; i < mul_index.size(); i++) {
        if (mul_index[i][2] != UINT64_MAX) {
        #ifdef DEBUG
            printf("\tCompleting MUL: %ld, for dyncount=%lu\n", i, mul_index[i][2]);
        #endif
            completed_dyncounts.push_back(mul_index[i][2]);
            mul_index[i][2] = UINT64_MAX;
        }
        mul_index[i][2] = mul_index[i][1];
        mul_index[i][1] = mul_index[i][0];
        mul_index[i][0] = UINT64_MAX;
    }

    // Progress LSU loads / stores
#ifdef DEBUG
    printf("\tProgressing LSU units\n");
#endif
    for (uint64_t i = 0; i < lsu_index.size(); i++) {
        if (lsu_index[i][0] != UINT64_MAX) {
            if (lsu_index[i][1]-- <= 1) {
            #ifdef DEBUG
                printf("\tCompleting LSU: %ld, for dyncount=%lu\n", i, lsu_index[i][0]);
            #endif
                completed_dyncounts.push_back(lsu_index[i][0]);
                lsu_index[i] = {UINT64_MAX, 0};
            }
            else {
            #ifdef DEBUG
                printf("\tLSU: %ld, needs to wait %" PRIu64 " cycles to complete\n", i, lsu_index[i][1]);
            #endif
            }
        }
    }

    // Apply Result Busses
#ifdef DEBUG
    printf("\tProcessing Result Busses\n");
#endif

    for (uint64_t dyn_count : completed_dyncounts) {
        uint64_t index;
        for (uint64_t i = 0; i < sched_queue.size(); i++) {
            if (sched_queue[i].inst_ptr->dyn_instruction_count == dyn_count) {
                index = i;
                break;
            }
        }

        uint64_t rd_tag = sched_queue[index].dest_reg_tag;
        uint64_t md_tag = sched_queue[index].md_tag;
        uint64_t dyncount = sched_queue[index].inst_ptr->dyn_instruction_count;

    #ifdef DEBUG
        printf("\tProcessing Result Bus for: ");
        print_instruction(sched_queue[index].inst_ptr);
        printf("\n");
    #endif

        if (sched_queue[index].opcode == OPCODE_LOAD || sched_queue[index].opcode == OPCODE_STORE) {
            uint8_t hash_addr = ((sched_queue[index].inst_ptr->load_store_addr & (0b111111 << 6)) >> 6);
            if (mem_alias_table[hash_addr].second == sched_queue[index].md_tag) {
                mem_alias_table[hash_addr].first = true;
            #ifdef DEBUG
                printf("Instr is load/store, marking ready=1 for M%" PRId8 " (tag = %" PRIu64 ") in MAT\n", hash_addr, mem_alias_table[hash_addr].second);
            #endif
            }
        }

        if (register_file[sched_queue[index].inst_ptr->dest].second == rd_tag) {
            register_file[sched_queue[index].inst_ptr->dest].first = true;
        #ifdef DEBUG
            printf("Instr has dest, marking ready=1 for R%" PRId8 " (tag = %" PRIu64 ") in messy RF\n", sched_queue[index].inst_ptr->dest, rd_tag);
        #endif
        }

    #ifdef DEBUG
        printf("Removing instruction with tag %" PRIu64 " from the scheduling queue\n", rd_tag);
    #endif
        sched_queue.erase(sched_queue.begin()+index);
        
        for (res_sta_t&  rs : sched_queue) {
            if (rs.src1_tag == rd_tag) {
                rs.src1_rdy = true;
            #ifdef DEBUG
                printf("Setting rs1_ready = 1 for rd_tag %" PRIu64 " from the scheduling queue\n", rd_tag);
            #endif
            }
            if (rs.src2_tag == rd_tag) {
                rs.src2_rdy = true;
            #ifdef DEBUG
                printf("Setting rs2_ready = 1 for rd_tag %" PRIu64 " from the scheduling queue\n", rd_tag);
            #endif
            }
            if (rs.ms_tag == md_tag) {
                rs.ms_rdy = true;
            #ifdef DEBUG
                printf("Setting ms_ready = 1 for md_tag %" PRIu64 " from the scheduling queue\n", md_tag);
            #endif
            }
        }

        for (auto& rob_entry: reorder_buffer) {
            if (rob_entry.first->dyn_instruction_count == dyncount)
                rob_entry.second = true;
        }
    #ifdef DEBUG
        printf("Setting instruction with rd_tag %" PRIu64 " as done in the ROB\n", rd_tag);
    #endif
    }

#ifdef DEBUG
    printf("\t%" PRIu64 " instructions completed\n", completed_dyncounts.size());
#endif
}

// Optional helper function which is responsible for looking through the
// scheduling queue and firing instructions that have their source pregs
// marked as ready. Note that when multiple instructions are ready to fire
// in a given cycle, they must be fired in program order. 
// Also, load and store instructions must be fired according to the 
// memory disambiguation algorithm described in the assignment PDF. Finally,
// instructions stay in their reservation station in the scheduling queue until
// they complete (at which point stage_exec() above should free their RS).
static void stage_schedule(procsim_stats_t *stats) {

#ifdef DEBUG
    printf("Stage Schedule: \n");
    if (!sched_queue.size())
        printf("\tCould not find scheduling queue entry to fire this cycle\n");
    uint64_t num_inst_sched = 0;
#endif

    bool inst_fired = false;
    for (size_t i = 0; i < sched_queue.size(); i++) {
        if (sched_queue.at(i).ms_rdy && sched_queue.at(i).src1_rdy && sched_queue.at(i).src2_rdy && !sched_queue.at(i).dispatched) {
        #ifdef DEBUG
            printf("\tAttempting to fire instruction: ");
            print_instruction(sched_queue.at(i).inst_ptr);
            printf(" to ");
            printf("\n");
            bool sched = false;
        #endif
            switch (sched_queue.at(i).opcode) {
                case OPCODE_ADD:
                    for (uint64_t j = 0; j < alu_index.size(); j++) {
                        if (alu_index.at(j) == UINT64_MAX) {
                            alu_index[j] = sched_queue[i].inst_ptr->dyn_instruction_count;
                        #ifdef DEBUG
                            printf("\t\tFired to ALU #%" PRIu64 "\n", j);
                            sched = true;
                        #endif
                            inst_fired = true;
                            sched_queue[i].dispatched = true;
                            break;
                        }
                    }
                    break;
                case OPCODE_MUL:
                    for (uint64_t j = 0; j < mul_index.size(); j++) {
                        if (mul_index.at(j).at(0) == UINT64_MAX) {
                            mul_index[j][0] = sched_queue[i].inst_ptr->dyn_instruction_count;
                        #ifdef DEBUG
                            printf("\t\tFired to MUL #%" PRIu64 "\n", j);
                            sched = true;
                        #endif
                            inst_fired = true;
                            sched_queue[i].dispatched = true;
                            break;
                        }
                    }
                    break;
                case OPCODE_LOAD:
                    for (uint64_t j = 0; j < lsu_index.size(); j++) {
                        uint64_t stall_num;
                        if (lsu_index[j][0] == UINT64_MAX) {
                            if (sched_queue[i].inst_ptr->dcache_hit == CACHE_LATENCY_L1_HIT)
                                stall_num = 2;
                            else {
                                if (sched_queue[i].inst_ptr->dcache_hit == CACHE_LATENCY_L2_HIT)
                                    stall_num = 10;
                                else
                                    stall_num = 100;
                            }
                            lsu_index[j] = {sched_queue[i].inst_ptr->dyn_instruction_count, stall_num};
                        #ifdef DEBUG
                            printf("\t\tFired to LSU #%" PRIu64 "\n", j);
                            sched = true;
                        #endif
                            inst_fired = true;
                            sched_queue[i].dispatched = true;
                            break;
                        }
                    }
                    break;
                case OPCODE_STORE:
                    for (uint64_t j = 0; j < lsu_index.size(); j++) {
                        if (lsu_index[j][0] == UINT64_MAX) {
                            lsu_index[j][0] = sched_queue[i].inst_ptr->dyn_instruction_count;
                        #ifdef DEBUG
                            printf("\t\tFired to LSU #%" PRIu64 "\n", j);
                            sched = true;
                        #endif
                            inst_fired = true;
                            sched_queue[i].dispatched = true;
                            break;
                        }
                    }
                    break;
                case OPCODE_BRANCH:
                    for (uint64_t j = 0; j < alu_index.size(); j++) {
                        if (alu_index[j] == UINT64_MAX) {
                            alu_index[j] = sched_queue[i].inst_ptr->dyn_instruction_count;
                        #ifdef DEBUG
                            printf("\t\tFired to ALU #%" PRIu64 "\n", j);
                            sched = true;
                        #endif
                            inst_fired = true;
                            sched_queue[i].dispatched = true;
                            break;
                        }
                    }
                    break;
            }
        #ifdef DEBUG
            if (!sched)
                printf("\t\tCannot fire instruction\n");
            else
                num_inst_sched++;
        #endif
        }
    }
    if (!inst_fired)
        stats->no_fire_cycles++;
#ifdef DEBUG
    if (!num_inst_sched && sched_queue.size())
        printf("\tCould not find scheduling queue entry to fire this cycle\n");
    printf("\t%" PRIu64 " instructions fired\n", num_inst_sched);
#endif
}

// Optional helper function which looks through the dispatch queue, decodes
// instructions, and inserts them into the scheduling queue. Dispatch should
// not add an instruction to the scheduling queue unless there is space for it
// in the scheduling queue and the ROB; 
// effectively, dispatch allocates reservation stations and ROB space for 
// each instruction dispatched and stalls if there any are unavailable. 
// Note the scheduling queue has a configurable size and the ROB has F * 32 entries.
// The PDF has details.
// dispatch width is fetch width
static void stage_dispatch(procsim_stats_t *stats) {

#ifdef DEBUG
    printf("Stage Dispatch: \n");
#endif
    uint64_t nop_counter = 0;
    uint64_t ins_counter = 0;

    while (!dispatch_queue.empty() && sched_queue.size() < rs_num && nop_counter < dispatch_width && ins_counter < dispatch_width) {
        if (reorder_buffer.size() < num_rob) {
            const inst_t* inst_pnt = dispatch_queue.front();

            if (inst_pnt == nullptr) {
                dispatch_queue.pop();
                nop_counter++;
                continue;
            }

        #ifdef DEBUG
            printf("\tAttempting Dispatch for: ");
            print_instruction(inst_pnt);
            printf("\n");
        #endif

            sched_queue.emplace_back();
            sched_queue.back().inst_ptr = inst_pnt;
            sched_queue.back().opcode = inst_pnt->opcode;
            sched_queue.back().dest_reg = inst_pnt->dest;
            
            sched_queue.back().md_tag = tag_counter++;

            if (inst_pnt->opcode == OPCODE_LOAD || inst_pnt->opcode == OPCODE_STORE) {
                uint64_t hash_addr = ((inst_pnt->load_store_addr & (0b111111 << 6)) >> 6);
                sched_queue.back().ms_rdy = mem_alias_table[hash_addr].first;
                sched_queue.back().ms_tag = mem_alias_table[hash_addr].second;

                mem_alias_table[hash_addr].second = sched_queue.back().md_tag;
                mem_alias_table[hash_addr].first = false;

            #ifdef DEBUG
                printf("\t\tInstr has load_store_addr=0x%" PRIx64 ", addr_class=0x%" PRIx64 ", ", inst_pnt->load_store_addr, hash_addr);
                printf("setting ms_tag=%" PRIu64 ", ms_ready=%" PRIu64 ", md_tag=%" PRIu64 "\n", sched_queue.back().ms_tag, (uint64_t) sched_queue.back().ms_rdy, sched_queue.back().md_tag);
                printf("\t\tIncrease tag counter by 1\n");
                printf("\t\tInstr is load/store, marking ready=0 and assigning tag=%" PRIu64 " for M%" PRIu64 " in MAT\n", mem_alias_table[hash_addr].second, hash_addr);
            #endif
            }
            else {
                sched_queue.back().ms_rdy = 1;
                sched_queue.back().ms_tag = 0;
            
            #ifdef DEBUG
                printf("\t\tInstr is not load/store, ");
                printf("setting ms_tag=%" PRIu64 ", ms_ready=%" PRIu64 ", md_tag=%" PRIu64 "\n", sched_queue.back().ms_tag, (uint64_t) sched_queue.back().ms_rdy, sched_queue.back().md_tag);
                printf("\t\tIncrease tag counter by 1\n");
            #endif
            }            

            if (inst_pnt->src1 != -1) {
                sched_queue.back().src1_rdy = register_file.at(inst_pnt->src1).first;
                sched_queue.back().src1_tag = register_file.at(inst_pnt->src1).second;
            
            #ifdef DEBUG
                printf("\t\tInstr has src1, ");
            #endif
            }
            else {
                sched_queue.back().src1_rdy = true;
                sched_queue.back().src1_tag = 0;

            #ifdef DEBUG
                printf("\t\tInstr does not have src1, ");
            #endif
            }

        #ifdef DEBUG
            printf("setting src1_tag=%" PRIu64 ", src1_ready=%" PRIu64, sched_queue.back().src1_tag, (uint64_t) sched_queue.back().src1_rdy);
            printf("\n");
        #endif

            if (inst_pnt->src2 != -1) {
                sched_queue.back().src2_rdy = register_file.at(inst_pnt->src2).first;
                sched_queue.back().src2_tag = register_file.at(inst_pnt->src2).second;
            
            #ifdef DEBUG
                printf("\t\tInstr has src2, ");
            #endif
            }
            else {
                sched_queue.back().src2_rdy = true;
                sched_queue.back().src2_tag = 0;

            #ifdef DEBUG
                printf("\t\tInstr does not have src2, ");
            #endif
            }

        #ifdef DEBUG
            printf("setting src2_tag=%" PRIu64 ", src2_ready=%" PRIu64, sched_queue.back().src2_tag, (uint64_t) sched_queue.back().src2_rdy);
            printf("\n");
            printf("\t\tsetting rd_tag=%" PRIu64 "\n", tag_counter);
            printf("\t\tIncrease tag counter by 1\n");
        #endif

            sched_queue.back().dest_reg_tag = tag_counter++;
            if (inst_pnt->dest != -1) {
                register_file.at(inst_pnt->dest).second = sched_queue.back().dest_reg_tag;
                register_file.at(inst_pnt->dest).first = false;
            #ifdef DEBUG
                printf("\tInstr has dest, marking ready=0 and assigning tag=%" PRIu64 " for R%" PRId8 " in messy RF\n", register_file.at(inst_pnt->dest).second, inst_pnt->dest);
            #endif
            }

        #ifdef DEBUG
            printf("\tDispatching and adding the instruction to the scheduling queue\n");
        #endif
            sched_queue.back().dispatched = false;
            reorder_buffer.push_back({inst_pnt, 0});
            dispatch_queue.pop();
            ins_counter++;
        }
        else {
            stats->rob_no_dispatch_cycles++;
        #ifdef DEBUG
            printf("\tDispatch stalled due to insufficient ROB space\n");
        #endif
            break;
        }
    }

#ifdef DEBUG
    printf("\t%" PRIu64 " instructions dispatched and added to the scheduling queue\n", ins_counter);
#endif
}

// Optional helper function which fetches instructions from the instruction
// cache using the provided procsim_driver_read_inst() function implemented
// in the driver and appends them to the dispatch queue. 
// The size of the dispatch queue is same as number of rob entries.
// Fetch should not add an instruction to the dispatch queue unless there
// is space for it in the dispatch queue 
// If a NULL pointer is fetched from the procsim_driver_read_inst, and driver_read_status is DRIVER_READ_ICACHE_MISS
// insert a NOP to the dispatch queue
// that NOP should be dropped at the dispatch stage
static void stage_fetch(procsim_stats_t *stats) {

#ifdef DEBUG
    printf("Stage Fetch: \n");
#endif

    driver_read_status_t driver_read_status_output;
    for (uint64_t i = 0; i < fetch_width; i ++) {
        if (dispatch_queue.size() < num_rob) {
            const inst_t* inst_pnt = procsim_driver_read_inst(&driver_read_status_output);
        #ifdef DEBUG
            switch (driver_read_status_output) {
                case DRIVER_READ_MISPRED:
                    printf("\tFetched NOP because of branch misprediction\n");
                    break;
                case DRIVER_READ_ICACHE_MISS:
                    printf("\tFetched NOP because of icache miss\n");
                    break;
                case DRIVER_READ_END_OF_TRACE:
                    printf("\tFetched NOP because of end of trace is reached\n");
                    break;        
                default:
                    printf("\tFetched Instruction: ");
                    print_instruction(inst_pnt);
                    printf("\n");
                    break;
            }
        #endif
            if (driver_read_status_output != DRIVER_READ_MISPRED && driver_read_status_output != DRIVER_READ_END_OF_TRACE) {
                dispatch_queue.push(inst_pnt);
                if (driver_read_status_output != DRIVER_READ_ICACHE_MISS)
                    stats->instructions_fetched++;
            #ifdef DEBUG
                if (driver_read_status_output == DRIVER_READ_ICACHE_MISS)
                    printf("\tPushed an NOP into the dispatch queue on icache miss\n");
                else if (inst_pnt != nullptr && inst_pnt->mispredict)
                    printf("\t\tBranch Misprediction will be handled by driver\n");
            #endif
            }
        }
        else {
        #ifdef DEBUG
            printf("\tNot fetching because the dispatch queue is full\n");
        #endif
            break;
        }
    }
#ifdef DEBUG
//printf("\t\tI-Cache miss repaired by driver\n");
#endif
}

// Use this function to initialize all your data structures, simulator
// state, and statistics.
void procsim_init(const procsim_conf_t *sim_conf, procsim_stats_t *stats) {
    
    num_rob = sim_conf->num_rob_entries;
    fetch_width = sim_conf->fetch_width;
    dispatch_width = sim_conf->dispatch_width;
    rs_num = sim_conf->num_schedq_entries_per_fu * (sim_conf->num_alu_fus + sim_conf->num_lsu_fus + sim_conf->num_mul_fus);

    for (int i = 0; i < NUM_REGS; i++)
        register_file.at(i) = std::make_pair(true, i);

    for (int i = 0; i < NUM_MATS; i++)
        mem_alias_table.at(i) = std::make_pair(true, i+32);

    for (size_t i = 0; i < sim_conf->num_alu_fus; i++)
        alu_index.push_back(UINT64_MAX);

    for (size_t i = 0; i < sim_conf->num_mul_fus; i++)
        mul_index.push_back({UINT64_MAX, UINT64_MAX, UINT64_MAX});

    for (size_t i = 0; i < sim_conf->num_lsu_fus; i++)
        lsu_index.push_back({UINT64_MAX, 0});

#ifdef DEBUG
    printf("\nScheduling queue capacity: %lu instructions\n", rs_num); 
    printf("\n"); //  PROVIDED
#endif
}

// To avoid confusion, we have provided this function for you. Notice that this
// calls the stage functions above in reverse order! This is intentional and
// allows you to avoid having to manage pipeline registers between stages by
// hand. This function returns the number of instructions retired, and also
// returns if a mispredict was retired by assigning true or false to
// *retired_mispredict_out, an output parameter.
uint64_t procsim_do_cycle(procsim_stats_t *stats,
                          bool *retired_mispredict_out) {
#ifdef DEBUG
    printf("================================ Begin cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
#endif

    // stage_state_update() should set *retired_mispredict_out for us
    uint64_t retired_this_cycle = stage_state_update(stats, retired_mispredict_out);

    if (*retired_mispredict_out) {
#ifdef DEBUG
        printf("%" PRIu64 " instructions retired. Retired mispredict, so notifying driver to fetch correctly!\n", retired_this_cycle); //  PROVIDED
#endif

        // After we retire a misprediction, the other stages don't need to run
        stats->branch_mispredictions++;
    } else {
#ifdef DEBUG
        printf("%" PRIu64 " instructions retired. Did not retire mispredict, so proceeding with other pipeline stages.\n", retired_this_cycle); //  PROVIDED
#endif

        // If we didn't retire an interupt, then continue simulating the other
        // pipeline stages
        stage_exec(stats);
        stage_schedule(stats);
        stage_dispatch(stats);
        stage_fetch(stats);
    }

#ifdef DEBUG
    printf("End-of-cycle dispatch queue usage: %lu\n", dispatch_queue.size());
    printf("End-of-cycle sched queue usage: %lu\n", sched_queue.size());
    printf("End-of-cycle ROB usage: %lu\n", reorder_buffer.size());
    printf("End-of-cycle Messy RF state:\n"); // PROVIDED
    print_messy_rf(); // PROVIDED
    printf("End-of-cycle MAT state:\n"); // PROVIDED
    print_mat(); // PROVIDED
    printf("End-of-cycle ROB state:\n"); // PROVIDED
    print_rob(); // PROVIDED
    printf("================================ End cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
    print_instruction(NULL); // this makes the compiler happy, ignore it
#endif

    // Increment max_usages and avg_usages in stats here!
    stats->cycles++;
    if (stats->dispq_max_usage < dispatch_queue.size())
        stats->dispq_max_usage = dispatch_queue.size();
    if (stats->schedq_max_usage < sched_queue.size())
        stats->schedq_max_usage = sched_queue.size();
    if (stats->rob_max_usage < reorder_buffer.size())
        stats->rob_max_usage = reorder_buffer.size();
    stats->dispq_avg_usage = (stats->dispq_avg_usage*(stats->cycles-1) + dispatch_queue.size())/stats->cycles;
    stats->schedq_avg_usage = (stats->schedq_avg_usage*(stats->cycles-1) + sched_queue.size())/stats->cycles;
    stats->rob_avg_usage = (stats->rob_avg_usage*(stats->cycles-1) + reorder_buffer.size())/stats->cycles;
    stats->ipc = (stats->ipc * (stats->cycles-1) + retired_this_cycle)/stats->cycles;

    // Return the number of instructions we retired this cycle (including the
    // interrupt we retired, if there was one!)
    return retired_this_cycle;
}

// Use this function to free any memory allocated for your simulator and to
// calculate some final statistics.
void procsim_finish(procsim_stats_t *stats) {
    // TODO: fill me in
}
