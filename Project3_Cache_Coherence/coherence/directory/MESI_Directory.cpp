#include "Directory_Controller.h"
#include "sim.h"

void DirectoryController::MESI_tick() {
    Request *request;
    dir_entry_t *entry;

    if (!request_in_progress) {
        if (((request = poll_queue()) != NULL)) {
            entry = get_entry(request->block);

            debug_printf("\t\tProcessing req from node %lu for block 0x%lx, state: %s, msg: %s\n", request->srcID,
                         request->block, dir_state_t_str[entry->state], message_t_str[request->msg]);

            // Directories for all protocols follow a similar pattern where the state machine is implemented as a series
            // of if-then-else statements. The current state of the block (entry->state) in addition to the request
            // message type (request->msg) are examined to check the current state, and take actions based on the
            // message type including setting the next state of the entry and take any network actions as required.

            // The block is in Invalid state and a store request on the block is encountered
            // When the block is in Invalid state in the directory, all requests are served by main memory and the
            // directory stalls until the response from main memory comes back.
            if (entry->state == I && request->msg == GETM) {
                // Set the request_in_progress flag since the directory stalls until the memory response comes back
                request_in_progress = true;

                // Set the block address and target node from the request
                tag_to_send = request->block;
                target_node = request->srcID;

                // Memory response will come back at current clock time + memory access latency
                response_time = Global_Clock + mem_latency;

                // Entry will be in state M once the data comes back from main memory
                entry->state = M;

                // Requesting processor's presence bit is set and number of active sharers incremented
                entry->presence[request->srcID] = true;
                entry->active_sharers++;

                debug_printf("\t\t\tRequesting data from memory\n");

                // Remove request from the head of the request queue
                dequeue();
            } else if (entry->state == I && request->msg == GETS) {
                request_in_progress = true;
                tag_to_send = request->block;
                target_node = request->srcID;
                response_time = Global_Clock + mem_latency;

                // In protocols with the E state, when a block is in Invalid state and a Load (read) request comes from
                // an agent, the block state transitions to the E state. The send_E flag is used to ensure that the
                // request from the directory to the agent is sent with message type DATA_E
                entry->state = E;
                send_E = true;

                entry->presence[request->srcID] = true;
                entry->active_sharers++;
                debug_printf("\t\t\tRequesting data from memory\n");
                dequeue();
            } else if (entry->state == M && request->msg == GETM) {
                // The block is in M state, and an agent issues a store (GETM) request. The directory will recall data
                // from the agent that currently has the block in the dirty state before sending it to the requesting
                // agent.

                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        debug_printf("\t\t\tNode %d will provide data for $-$ transfer\n", i);
                    }
                }

                // Send a data recall to the target agent in order to get the current dirty block from the agent. Tell
                // the agent to go to I state since the request is a write to the block. Then move the current state
                // from M to MM since the block is moving from M to M but the agent in the M state is changing and needs
                // to be sent the recalled data
                debug_printf("\t\t\tRecall_goto_I: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_I);
                entry->state = MM;
                entry->req_node_in_transient = request->srcID;

                dequeue();
            } else if (entry->state == M && request->msg == GETS) {
                // The block is in the M state, and an agent issues a read (GETS). The directory will recall the data
                // from the agent that currently has the block in the dirty state before sending it to the requesting
                // agent.

                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        debug_printf("\t\t\tNode %d will provide data for $-$ transfer\n", i);
                    }
                }

                // Send a data recall to the target agent in order to get the current dirty block from the agent. Tell
                // the agent to go to S state since the request is a read. Then move the current state
                // from M to MS since the block is moving from M to S but needs to wait on the data from the target
                // agent before providing it to the requesting agent.
                debug_printf("\t\t\tRecall_goto_S: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_S);
                entry->state = MS;
                entry->req_node_in_transient = request->srcID;

                dequeue();
            } else if (entry->state == M && request->msg == GETX) {
                // The block is in the M state, and an agent that was once in E issues a write (GETX). The directory 
                // will recall the data from the agent that currently has the block in the dirty state before sending 
                // it to the requesting agent.
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
                // Current state is MM and the message is DATA. This means that the data can be sent to the agent that
                // requested the data and the block state can move to M.

                tag_to_send = request->block;

                // The target node is the one that had made the request when the block was in the MM state.
                target_node = entry->req_node_in_transient;

                // now that the original in M has gone to the I state remove it's presence bit
                entry->presence[request->srcID] = false;
                entry->active_sharers--;

                // Directory serves as $-$ transfer agent: Increment stats!
                sim->cache_to_cache_transfers++;

                // Memory is updated since the original agent left the M state
                sim->memory_writes++;

                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);

                // Set the block's state to M and set the presence bit for the processor that now has the block in M.
                entry->state = M;
                entry->presence[target_node] = true;
                entry->active_sharers++;

                dequeue();
            } else if (entry->state == MS && request->msg == DATA) {
                // Current state is MS and the message is DATA. This means that the data can be sent to the agent that
                // requested the data and the block state can move to S.
                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;

                // do not need to modify presence for node that was in M since it is now in S
                // Directory serves as $-$ transfer agent:
                sim->cache_to_cache_transfers++;
                // Memory is updated since the original agent left the M state
                sim->memory_writes++;

                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);

                // Set the state of the block to S since read only copies in agent caches
                entry->state = S;
                entry->presence[target_node] = true;
                entry->active_sharers++;

                dequeue();
            } else if (entry->state == E && request->msg == GETM) {
                // Current state is E and the message is GETM. The directory needs to recall the data and
                // send it to the requesting node

                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        debug_printf("\t\t\tNode %d will provide data for $-$ transfer\n", i);
                    }
                }
                
                // Tell the agent that has the block in the E state to send the data to the directory and transition to
                // the Invalid state.
                debug_printf("\t\t\tRecall_goto_I: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_I);

                // Move to transient state EM and wait for the agent in E to send data to the directory
                entry->state = EM;
                entry->req_node_in_transient = request->srcID;

                dequeue();
            } else if (entry->state == E && request->msg == GETS) {
                // Current state is E and the message is GETS. The directory needs to recall the data and
                // send it to the requesting node
                tag_to_send = request->block;
                target_node = UINT64_MAX;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        target_node = i;
                        debug_printf("\t\t\tNode %d will provide data for $-$ transfer\n", i);
                    }
                }

                // Move to transient state ES and wait for the agent in E to send data to the directory
                debug_printf("\t\t\tRecall_goto_S: ");
                send_Request(target_node, tag_to_send, RECALL_GOTO_S);
                entry->state = ES;
                entry->req_node_in_transient = request->srcID;
                
                dequeue();
            } else if (entry->state == E && request->msg == GETX) {
                // The block is in the E state and the processor that has the block wants to write to the block. The
                // processor sends a GETX to the directory informing it that it wants to do a silent upgrade from E to M

                tag_to_send = request->block;
                target_node = request->srcID;
                sim->silent_upgrades++;

                // The directory sends and ACK message and moves the block to the M state
                debug_printf("\t\t\tAllowing upgrade from E to M: ");
                send_Request(target_node, tag_to_send, ACK);
                entry->state = M;

                dequeue();
            } else if (entry->state == ES && request->msg == DATA) {
                // Transient state for moving block from E to S via a one time $-$ transfer

                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;

                // do not need to modify presence for node that was in E since it is now in S
                // Directory serves as $-$ transfer agent:
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);

                entry->state = S;
                entry->presence[target_node] = true;
                entry->active_sharers++;

                dequeue();
            } else if (entry->state == EM && request->msg == DATA) {
                // Transient state for moving block from E to M via a one time $-$ transfer

                tag_to_send = request->block;
                target_node = entry->req_node_in_transient;

                // Presense bit modified since the agent that was in E now moves to the I state
                entry->presence[request->srcID] = false;
                entry->active_sharers--;

                // Directory serves as $-$ transfer agent:
                sim->cache_to_cache_transfers++;
                debug_printf("\t\t\tCache to cache transfer via directory: ");
                send_Request(target_node, tag_to_send, DATA);

                // Block moved to the M state
                entry->state = M;
                entry->presence[target_node] = true;
                entry->active_sharers++;

                dequeue();
            } else if (entry->state == S && request->msg == GETM) {
                // Current state is S and the message is GETM. This means 1 of 2 events can happen:
                // - many (including the requester) have the data and we invalidate others then upgrade requester via ACK
                // - many (excluding requester) have the data and we invalidate others then go to mem to get the data
                // note that in MESI, when in S, at least 2 nodes must be in S

                // Send invalidation messages to nodes that have the data and are not the requester
                entry->inv_ack_waiting = 0;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        if (i != request->srcID) {
                            // invalidate the others
                            tag_to_send = request->block;
                            target_node = i;
                            entry->inv_ack_waiting++;
                            debug_printf("\t\t\tSending Invalidate to %lu: ", i);
                            send_Request(i, tag_to_send, REQ_INVALID);
                        } else {
                            // Nothing to do here for the src. It has to wait for all the Invalid ACK's to come back.
                            // Once they all come back only the requester has the block in its cache and can write to
                            // it.
                        }
                    }
                }
                entry->state = SM;
                entry->req_node_in_transient = request->srcID;

                dequeue();
            } else if (entry->state == S && request->msg == GETS) {
                // S is not a $-$ transfer agent so the block data has to come from main memory

                request_in_progress = true;
                tag_to_send = request->block;
                target_node = request->srcID;
                response_time = Global_Clock + mem_latency;
                entry->state = S;
                entry->presence[request->srcID] = true;
                entry->active_sharers++;

                debug_printf("\t\t\tRequesting data from memory\n");
                dequeue();
            } else if (entry->state == S && request->msg == GETX) {
                // The block is in S state in the directory and there is a request for upgrade (i.e. block is being
                // written by the agent) from an agent that was in E. The directory has to check if any other agents 
                // have the block and sent invalidates to sharers if any.

                entry->inv_ack_waiting = 0;
                for (nodeID_t i = 0; i < settings.num_procs; i++) {
                    if (entry->presence[i]) {
                        if (i != request->srcID) {
                            // invalidate the others
                            tag_to_send = request->block;
                            target_node = i;
                            entry->inv_ack_waiting++;
                            debug_printf("\t\t\tSending Invalidate to %lu: ", i);
                            send_Request(i, tag_to_send, REQ_INVALID);
                        } else {
                            // once all invalidates have been ack'd only the orig requester can have presence
                        }
                    }
                }
                entry->state = SM;
                entry->req_node_in_transient = request->srcID;

                dequeue();
            } else if ((entry->state == SM) && (request->msg == INVACK)) {
                // Transitioning from S to M, and waiting on invalid ACKs from all agent's who were sent the invalidate
                // message. Wait until all invalid acks have been received.

                // Decrement entry inv_ack_waiting until we get to the last invalidate:
                if (entry->inv_ack_waiting >= 1) {
                    entry->presence[request->srcID] = false;
                    entry->inv_ack_waiting--;
                    entry->active_sharers--;
                    debug_printf("\t\t\tMarking node %lu as invalid, %d acks remaining\n", request->srcID,
                                 entry->inv_ack_waiting);

                } else {
                    fatal_error("\t\t\tReceived too many invacks\n");
                }

                // Once all invalid acks have been received, One of 2 cases can occur:
                // - only the requester has the data and we upgrade it via ACK
                // - the requester does not have the data and we must go to memory
                if (entry->inv_ack_waiting == 0) {
                    if (entry->presence[entry->req_node_in_transient]) {
                        tag_to_send = request->block;
                        target_node = entry->req_node_in_transient;
                        debug_printf("\t\t\tFinal invack received, upgrading requester to M: ");
                        send_Request(target_node, tag_to_send, ACK);
                        entry->state = M;

                    } else {
                        request_in_progress = true;
                        tag_to_send = request->block;
                        target_node = entry->req_node_in_transient;
                        response_time = Global_Clock + mem_latency;
                        entry->presence[entry->req_node_in_transient] = true;
                        entry->active_sharers++;
                        debug_printf("\t\t\tFinal invack received, fetching data from memory\n");
                        entry->state = M;
                        
                    }
                }
                dequeue();
            } else if ((entry->state == SM || entry->state == MM || entry->state == MS || entry->state == EM ||
                        entry->state == ES) &&
                       (request->msg == GETM || request->msg == GETS || request->msg == GETX)) {
                debug_printf("\t\t\tCycling queue\n");

                // When in a transition state if the request type is not DATA or INVACK, just push the request to the
                // back of the queue (cycling)
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
