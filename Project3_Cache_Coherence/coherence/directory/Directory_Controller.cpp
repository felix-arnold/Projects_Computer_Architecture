#include "Directory_Controller.h"
#include <iostream>

const char *dir_state_t_str[NUM_DIR_STATE] = {
    "M",   // Modified
    "O",   // Owner
    "S",   // Shared
    "I",   // Invalid
    "F",   // Forwarder
    "FS",  // Transient state from F->S
    "FM",  // Transient state from F->M
    "SM",  // Transient state from S->M
    "MO",  // Transient state from M->O
    "SS",  // Transient state from S->S
    "MS",  // Transient state from M->S
    "MM",  // Transient state from M->M
    "OM",  // Transient state from O->M
    "E",   // Exclusive
    "ES",  // Transient state from E->S
    "EM",  // Transient state from E->M
    "OO",  // Transient state from O->O
    "EF",  // Transient state from E->F
    "FF"   // Transient state from F->F
};

DirectoryController::DirectoryController(nodeID_t id) {
    this->id = id;
    request_in_progress = false;
    response_time = 0;
    mem_latency = settings.mem_latency;
    send_E = false; // ADD THIS
}

DirectoryController::~DirectoryController() {
    for (auto entry : directory) {
        delete entry.second;
    }
}

void DirectoryController::tick() {
    debug_printf("Tick for Directory\n");
    switch (settings.protocol) {
        case MI_PRO:
            MI_tick();
            break;
        case MSI_PRO:
            MSI_tick();
            break;
        case MESI_PRO:
            MESI_tick();
            break;
        case MOSI_PRO:
            MOSI_tick();
            break;
        case MOESIF_PRO:
            MOESIF_tick();
            break;
        default:
            fatal_error("Unsupported Protocol\n");
    }
}

void DirectoryController::tock() { request_queue.splice(request_queue.end(), request_next); }
// Returns directory entry for the requested cache block
dir_entry_t *DirectoryController::get_entry(blockID_t blockID) {
    auto it = directory.find(blockID);
    dir_entry_t *entry;
    if (it == directory.end()) {
        entry = new dir_entry_t();
        directory.insert(std::pair<blockID_t, dir_entry_t *>(blockID, entry));
    } else {
        entry = it->second;
    }
    return entry;
}

// Returns head of request queue without popping from the queue
Request *DirectoryController::poll_queue() {
    if (!request_queue.empty()) {
        return request_queue.front();
    } else {
        return NULL;
    }
}

// Pops the front of the request queue
void DirectoryController::dequeue() {
    if (!request_queue.empty()) {
        Request *front = request_queue.front();
        request_queue.pop_front();
        delete (front);
    }
}

// Pops the front of the request queue and pushes it to the back
void DirectoryController::cycle_queue() {
    if (!request_queue.empty()) {
        Request *head = request_queue.front();
        request_queue.pop_front();
        request_next.push_back(head);
    }
}

// Creates a new request and pushes it onto the node's network out queue
void DirectoryController::send_Request(nodeID_t dest, blockID_t tag, message_t msg) {
    Request *req = new Request(id, dest, tag, msg);
    if (!req) {
        fatal_error("Allocation for Request Failed\n");
    }
    req->print_state();
    my_node->ntwk_out_next.push_back(req);
}

// Prints the presence bits for the input directory entry for debugging
void DirectoryController::print_presence(dir_entry_t *entry) {
    debug_printf("\t\t\tState: %s, Presence bits: [", dir_state_t_str[entry->state]);
    for (nodeID_t i = 0; i < settings.num_procs - 1; i++) {
        debug_printf("%d, ", entry->presence[i] ? 1 : 0);
    }
    debug_printf("%d] Active: %d\n", entry->presence[settings.num_procs - 1] ? 1 : 0, entry->active_sharers);

    uint64_t presence = 0;
    for (nodeID_t i = 0; i < settings.num_procs; i++) {
        presence += entry->presence[i] ? 1 : 0;
    }
    if (presence != entry->active_sharers) {
        debug_printf("\t\t\tEntry active sharers does not match presence vector: %d\n", presence);
    }
}
