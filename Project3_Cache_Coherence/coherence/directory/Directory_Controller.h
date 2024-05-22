#ifndef DirectoryController_H
#define DirectoryController_H

#include <cstring>
#include <list>
#include <unordered_map>

#include "node.h"
#include "request.h"
#include "settings.h"
#include "types.h"

class Node;

typedef enum {
    M = 0,  // Modified
    O,      // Owner
    S,      // Shared
    I,      // Invalid
    F,      // Forwarder
    FS,     // Transient state from F->S (not real?)
    FM,     // Transient state from F->M
    SM,     // Transient state from S->M
    MO,     // Transient state from M->O
    SS,     // Transient state from S->S
    MS,     // Transient state from M->S
    MM,     // Transient state from M->M
    OM,     // Transient state from O->M
    E,      // Exclusive
    ES,     // Transient state from E->S
    EM,     // Transient state from E->M
    OO,     // Transient state from O->O
    EF,     // Transient state from E->F
    FF,     // Transient state from F->F
    NUM_DIR_STATE
} dir_state_t;

extern const char *dir_state_t_str[NUM_DIR_STATE];

typedef struct directory_entry_struct {
    blockID_t blockID;  // what is this for?
    bool dirty;
    bool presence[MAX_SHARERS];

    // Needed because transient states exist
    dir_state_t state;

    // The current number of sharers (same as the number of 1s in presence)
    uint32_t active_sharers;

    // How we know who to respond to/initial requester of block
    nodeID_t req_node_in_transient;

    // Need to know who is in O/F
    nodeID_t O_F_id;

    // Number of acks expected after invalidates were sent
    int32_t inv_ack_waiting;

    // If a req just needs src to be upgraded, we can just tell the src to upgrade
    bool getM_upgrade;

    directory_entry_struct()
        : blockID(0ul),
          dirty(false),
          state(I),
          active_sharers(0),
          req_node_in_transient(0),
          O_F_id(0),
          inv_ack_waiting(0),
          getM_upgrade(false) {
        memset(presence, 0, MAX_SHARERS);
    }
} dir_entry_t;

class DirectoryController {
   public:
    DirectoryController(nodeID_t id);
    ~DirectoryController();

    nodeID_t id;

    int64_t mem_latency;

    Node *my_node;

    std::list<Request *> request_next;
    // tick: set by other nodes
    // tock: move to queue

    std::list<Request *> request_queue;
    // tick: consume and progress state
    // tock: set from next

    std::unordered_map<blockID_t, dir_entry_t *> directory;

    bool send_E = false;
    bool send_F = false;
    blockID_t tag_to_send;
    nodeID_t target_node;
    bool request_in_progress;
    timestamp_t response_time;

    void tick();
    void tock();

    void MI_tick();
    void MSI_tick();
    void MESI_tick();
    void MOSI_tick();
    void MESIF_tick();
    void MOESIF_tick();

    dir_entry_t *get_entry(blockID_t blockID);

    Request *poll_queue();
    void dequeue();
    void cycle_queue();
    void print_presence(dir_entry_t *entry);
    void send_Request(nodeID_t destination, blockID_t tag, message_t msg);
};

#endif  // DirectoryController_H
