#include "Directory_Controller.h"
#include "messages.h"
#include "sim.h"

void DirectoryController::MI_tick() {
    Request *request;
    dir_entry_t *entry;

    if (!request_in_progress) {
        if (((request = poll_queue()) != NULL)) {
            entry = get_entry(request->block);

            debug_printf("\t\tProcessing req from node %lu for block 0x%lx, msg: %s\n", request->srcID, request->block,
                         message_t_str[request->msg]);

            if (entry->state == I && request->msg == GETM) {
                // requesting data from memory. directory stalls till memory response
                // once memory response comes in, we will be in M
                request_in_progress = true;
                tag_to_send = request->block;
                target_node = request->srcID;
                response_time = Global_Clock + mem_latency;
                entry->state = M;
                entry->presence[request->srcID] = true;
                entry->active_sharers++;
                debug_printf("\t\t\tRequesting data from memory\n");
                dequeue();
            } else if (entry->state == M && request->msg == GETM) {
                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        debug_printf("\t\t\tNode %d will provide data for $-$ transfer\n", i);
                    }
                }

                debug_printf("\t\t\tRecall_goto_I: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_I);
                entry->state = MM;
                entry->req_node_in_transient = request->srcID;
                dequeue();
            } else if (entry->state == MM && request->msg == DATA) {
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;
                // now that the original in M has gone to I:
                entry->presence[request->srcID] = false;
                entry->active_sharers--;
                // Directory serves as $-$ transfer agent:
                sim->cache_to_cache_transfers++;
                // Memory is updated since the original agent left the M state
                sim->memory_writes++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);
                entry->state = M;
                entry->presence[target_node] = true;
                entry->active_sharers++;
                dequeue();
            } else if (entry->state == MM && request->msg == GETM) {
                // cycle the queue
                debug_printf("\t\t\tCycling queue\n");
                cycle_queue();
            } else {
                fatal_error("\t\t\tInvalid state in directory for this request\n");
            }

            print_presence(entry);
        }
    }

    if (request_in_progress && Global_Clock >= response_time) {
        // Requisite time for Memory Access has passed:
        sim->memory_reads++;
        debug_printf("\t\t\tsending data from memory: ");
        send_Request(target_node, tag_to_send, DATA);
        request_in_progress = false;
    }
}
