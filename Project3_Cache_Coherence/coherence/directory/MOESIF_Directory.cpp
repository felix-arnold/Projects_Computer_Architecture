#include "Directory_Controller.h"
#include "sim.h"

void DirectoryController::MOESIF_tick() {
    Request *request;
    dir_entry_t *entry;

    if (!request_in_progress) {
        if (((request = poll_queue()) != NULL)) {
            entry = get_entry(request->block);

            debug_printf("\t\tProcessing req from node %lu for block 0x%lx, state: %s, msg: %s\n", request->srcID,
                         request->block, dir_state_t_str[entry->state], message_t_str[request->msg]);

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
            } else if (entry->state == I && request->msg == GETS) {
                // requesting data from memory. directory stalls till memory response
                // once memory response comes in, we will be in M
                request_in_progress = true;
                tag_to_send = request->block;
                target_node = request->srcID;
                response_time = Global_Clock + mem_latency;
                entry->state = E;
                send_E = true;
                entry->presence[request->srcID] = true;
                entry->active_sharers++;
                debug_printf("\t\t\tRequesting data from memory\n");
                dequeue();
            } else if (entry->state == M && request->msg == GETM) {
                // recall from the current M and send it to I
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
            } else if (entry->state == M && request->msg == GETS) {
                // recall from the current M and set to O
                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        entry->O_F_id = i;
                        debug_printf("\t\t\tNode %d will go to O and provide data for $-$ transfer\n", i);
                    }
                }
                debug_printf("\t\t\tRecall_goto_S: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_S);
                entry->state = MO;
                entry->req_node_in_transient = request->srcID;
                
                dequeue();
            } else if (entry->state == M && request->msg == GETX) {
                // recall from the current M and send it to I
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
                // now that the original in M has gone to S:
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
            } else if (entry->state == MO && request->msg == DATA) {
                // recall from the current M and set to O
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;
                // do not modify presence for O proc since it stays in O
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);
                entry->state = O;
                entry->presence[target_node] = true;
                entry->active_sharers++;
                
                dequeue();
            } else if (entry->state == E && request->msg == GETM) {
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
                entry->state = EM;
                entry->req_node_in_transient = request->srcID;
                
                dequeue();
            } else if (entry->state == E && request->msg == GETS) {
                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        entry->O_F_id = i;
                        debug_printf("\t\t\tNode %d will provide data for $-$ transfer\n", i);
                    }
                }
                debug_printf("\t\t\tUpgrading %lu to F\n", target_node);
                debug_printf("\t\t\tRecall_goto_S: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_S);
                entry->state = EF;
                entry->req_node_in_transient = request->srcID;
                
                dequeue();
            } else if (entry->state == E && request->msg == GETX) {
                tag_to_send = request->block;
                target_node = request->srcID;
                sim->silent_upgrades++;
                debug_printf("\t\t\tAllowing upgrade from E to M: ");
                send_Request(target_node, tag_to_send, ACK);
                entry->state = M;
                dequeue();
            } else if (entry->state == EF && request->msg == DATA) {
                
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;
                // do not need to modify presence for node that was in M since it is now in S
                // Directory serves as $-$ transfer agent:
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);
                entry->state = F;
                entry->presence[target_node] = true;
                entry->active_sharers++;
                
                dequeue();
            } else if (entry->state == EM && request->msg == DATA) {
                
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;
                entry->presence[request->srcID] = false;
                entry->active_sharers--;
                // Directory serves as $-$ transfer agent:
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);
                entry->state = M;
                entry->presence[target_node] = true;
                entry->active_sharers++;
                
                dequeue();
            } else if (entry->state == F && request->msg == GETM) {
                debug_printf("\t\t\tHave node %lu in F, invalidating non-requesters in S: \n", entry->O_F_id);
                entry->inv_ack_waiting = 0;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        if (i != request->srcID && i != entry->O_F_id) {
                            // invalidate the others
                            tag_to_send = request->block;
                            target_node = i;
                            entry->inv_ack_waiting++;
                            debug_printf("\t\t\tSending Invalidate to %lu: ", i);
                            send_Request(i, tag_to_send, REQ_INVALID);
                        }
                    }
                }
                if (entry->inv_ack_waiting == 0) {
                    // if in F, we know there are at least 2 procs with presence.
                    // if no invalidations were sent then requester must have been in S
                    //          AND there are only 2 active sharers
                    // cannot be in F if only 1 proc has presence
                    // means there is 1 in F, one in S, and the one in S sent the getM:
                    debug_printf("\t\t\tNo non-requester in S and requester was not in F:");
                    tag_to_send = request->block;
                    debug_printf("\t\t\tRecalling from F proc %lu: ", entry->O_F_id);
                    send_Request(entry->O_F_id, tag_to_send, RECALL_GOTO_I);
                }
                entry->state = FM;
                entry->req_node_in_transient = request->srcID;
                dequeue();
            } else if (entry->state == F && request->msg == GETS) {
                if (entry->O_F_id == request->srcID) {
                    fatal_error("Saw getS from proc already in F\n");
                } else {
                    tag_to_send = request->block;
                    debug_printf("\t\t\tRecalling from F proc %lu: ", entry->O_F_id);
                    send_Request(entry->O_F_id, tag_to_send, RECALL_GOTO_S);
                    entry->req_node_in_transient = request->srcID;
                    entry->state = FF;
                }
                dequeue();

            } else if (entry->state == F && request->msg == GETX) {
                // wanted to go EM but ended up going to F first due to message ordering
                debug_printf("\t\t\tHave node %lu in F, invalidating non-requesters in S: \n", entry->O_F_id);
                entry->inv_ack_waiting = 0;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        if (i != request->srcID) {
                            // invalidate the others since requester must be the F
                            tag_to_send = request->block;
                            target_node = i;
                            entry->inv_ack_waiting++;
                            debug_printf("\t\t\tSending Invalidate to %lu: ", i);
                            send_Request(i, tag_to_send, REQ_INVALID);
                        }
                    }
                }
                entry->state = FM;
                entry->req_node_in_transient = request->srcID;
                dequeue();
            } else if (entry->state == FF && request->msg == DATA) {
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);
                entry->presence[target_node] = true;
                entry->active_sharers++;
                entry->state = F;
                dequeue();
            } else if ((entry->state == FM) && (request->msg == INVACK)) {
                // decrement until we get to the last invalidate:
                if (entry->inv_ack_waiting >= 1) {
                    entry->presence[request->srcID] = false;
                    entry->inv_ack_waiting--;
                    entry->active_sharers--;
                    debug_printf("\t\t\tMarking node %lu as invalid, %d acks remaining\n", request->srcID,
                                 entry->inv_ack_waiting);
                    
                } else {
                    fatal_error("\t\t\tReceived too many invacks\n");
                }
                if (entry->inv_ack_waiting == 0) {
                    if (entry->active_sharers == 1 && entry->req_node_in_transient == entry->O_F_id) {
                        debug_printf("\t\t\tOriginal requester was already in F, upgrade: ");
                        tag_to_send = request->block;
                        target_node = entry->req_node_in_transient;
                        send_Request(target_node, tag_to_send, ACK);
                        entry->state = M;
                    } else {
                        tag_to_send = request->block;
                        debug_printf("\t\t\tRecalling from F proc %lu: ", entry->O_F_id);
                        send_Request(entry->O_F_id, tag_to_send, RECALL_GOTO_I);
                    }
                    
                }
                dequeue();

            } else if ((entry->state == FM) && (request->msg == DATA)) {
                entry->presence[entry->O_F_id] = false;
                entry->active_sharers--;
                if (entry->presence[entry->req_node_in_transient]) {
                    // send upgrade since the node already has the data
                    tag_to_send = request->block;
                    target_node = entry->req_node_in_transient;
                    sim->cache_to_cache_transfers++;
                    debug_printf("\t\t\tUpgrading Proc in S: ");
                    send_Request(target_node, tag_to_send, ACK);
                    entry->O_F_id = UINT64_MAX;
                    entry->state = M;
                } else {
                    tag_to_send = request->block;
                    target_node = entry->req_node_in_transient;
                    sim->cache_to_cache_transfers++;
                    debug_printf("\t\t\tCache to cache transfer via directory: ");
                    send_Request(target_node, tag_to_send, DATA);
                    entry->presence[target_node] = true;
                    entry->active_sharers++;
                    entry->O_F_id = UINT64_MAX;
                    entry->state = M;
                }
                dequeue();
            } else if (entry->state == O && request->msg == GETM) {
                debug_printf("\t\t\tHave node %lu in O, invalidating non-requesters in S: \n", entry->O_F_id);
                entry->inv_ack_waiting = 0;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        if (i != request->srcID && i != entry->O_F_id) {
                            // invalidate the others
                            tag_to_send = request->block;
                            target_node = i;
                            entry->inv_ack_waiting++;
                            debug_printf("\t\t\tSending Invalidate to %lu: ", i);
                            send_Request(i, tag_to_send, REQ_INVALID);
                        } else {
                            // once all invalidates have been ack'd only the orig requester can have presence
                            debug_printf("\t\t\tShould not invalidate O proc or the requester\n");
                        }
                    }
                }
                if (entry->inv_ack_waiting == 0) {
                    // if in O, we know there are at least 2 procs with presence.
                    // if no invalidations were sent then requester must have been in S
                    // cannot be in O if only 1 proc has presence
                    // means there is 1 in O, one in S, and the one in S sent the getM:
                    debug_printf("\t\t\tNo non-requester in S and requester was not in O:");
                    tag_to_send = request->block;
                    debug_printf("\t\t\tRecalling from O proc %lu: ", entry->O_F_id);
                    send_Request(entry->O_F_id, tag_to_send, RECALL_GOTO_I);
                }
                entry->req_node_in_transient = request->srcID;
                entry->state = OM;
                // }
                dequeue();
            } else if (entry->state == O && request->msg == GETS) {
                // if O id is getM requester
                // - only requester has the data and we upgrade it
                // - recall O proc and send to requester.
                if (entry->O_F_id == request->srcID) {
                    fatal_error("Saw getS from proc already in O\n");
                } else {
                    // Recall O proc and send to requester:
                    tag_to_send = request->block;
                    debug_printf("\t\t\tRecalling from O proc %lu: ", entry->O_F_id);
                    send_Request(entry->O_F_id, tag_to_send, RECALL_GOTO_S);
                    entry->req_node_in_transient = request->srcID;
                    entry->state = OO;
                }
                dequeue();
            } else if (entry->state == O && request->msg == GETX) {
                // wanted to go EM but ended up going to O somewhere else first due to message ordering
                debug_printf("\t\t\tHave node %lu in O, invalidating non-requesters in S: \n", entry->O_F_id);
                entry->inv_ack_waiting = 0;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        if (i != request->srcID && i != entry->O_F_id) {
                            // invalidate the others since requester must be the F
                            tag_to_send = request->block;
                            target_node = i;
                            entry->inv_ack_waiting++;
                            debug_printf("\t\t\tSending Invalidate to %lu: ", i);
                            send_Request(i, tag_to_send, REQ_INVALID);
                        }
                    }
                }
                entry->state = OM;
                entry->req_node_in_transient = request->srcID;
                dequeue();
            } else if ((entry->state == OM) && (request->msg == INVACK)) {
                if (entry->inv_ack_waiting >= 1) {
                    entry->presence[request->srcID] = false;
                    entry->inv_ack_waiting--;
                    entry->active_sharers--;
                    debug_printf("\t\t\tMarking node %lu as invalid, %d acks remaining\n", request->srcID,
                                 entry->inv_ack_waiting);
                    
                } else {
                    fatal_error("\t\t\tReceived too many invacks\n");
                }
                if (entry->inv_ack_waiting == 0) {
                    if (entry->active_sharers == 1 && entry->req_node_in_transient == entry->O_F_id) {
                        debug_printf("\t\t\tOriginal requester was already in O, upgrade: ");
                        tag_to_send = request->block;
                        target_node = entry->req_node_in_transient;
                        send_Request(target_node, tag_to_send, ACK);
                        entry->state = M;
                    } else {
                        tag_to_send = request->block;
                        debug_printf("\t\t\tRecalling from O proc %lu: ", entry->O_F_id);
                        send_Request(entry->O_F_id, tag_to_send, RECALL_GOTO_I);
                    }
                    
                }
                dequeue();
            } else if ((entry->state == OM) && (request->msg == DATA)) {
                entry->presence[entry->O_F_id] = false;
                entry->active_sharers--;
                sim->memory_writes++;
                if (entry->presence[entry->req_node_in_transient]) {
                    // send upgrade since the node already has the data
                    tag_to_send = request->block;
                    target_node = entry->req_node_in_transient;
                    // no transfer necessary since requester has presence
                    debug_printf("\t\t\tUpgrading Proc in S: ");
                    send_Request(target_node, tag_to_send, ACK);
                    entry->O_F_id = UINT64_MAX;
                    entry->state = M;
                } else {
                    tag_to_send = request->block;
                    target_node = entry->req_node_in_transient;
                    sim->cache_to_cache_transfers++;
                    debug_printf("\t\t\tCache to cache transfer via directory: ");
                    send_Request(target_node, tag_to_send, DATA);
                    entry->presence[target_node] = true;
                    entry->active_sharers++;
                    entry->O_F_id = UINT64_MAX;
                    entry->state = M;
                }
                dequeue();
            } else if (entry->state == OO && request->msg == DATA) {
                // if O id is getM requester
                // - only requester has the data and we upgrade it
                // - recall O proc and send to requester.
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);
                entry->presence[target_node] = true;
                entry->active_sharers++;
                entry->state = O;
                dequeue();
            } else if ((entry->state == MM || entry->state == MO || entry->state == EM || entry->state == EF ||
                        entry->state == FM || entry->state == FF || entry->state == OM || entry->state == OO) &&
                       (request->msg == GETM || request->msg == GETS || request->msg == GETX)) {
                // cycle the queue
                debug_printf("\t\t\tCycling queue\n");
                cycle_queue();
            } else {
                print_presence(entry);
                fatal_error("\t\t\tInvalid state in directory for this request\n");
            }

            print_presence(entry);
        }
    }

    if (request_in_progress && Global_Clock >= response_time) {
        // Requisite time for Memory Access has passed:
        sim->memory_reads++;
        debug_printf("\t\t\tsending data from memory: ");
        if (send_E) {
            send_Request(target_node, tag_to_send, DATA_E);
            send_E = false;
        } else
            send_Request(target_node, tag_to_send, DATA);

        request_in_progress = false;
    }
}
