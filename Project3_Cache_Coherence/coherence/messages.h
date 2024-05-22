#ifndef MESSAGES_H
#define MESSAGES_H

typedef enum {
    NOP = 0,

    LOAD,
    STORE,

    GETS,
    GETM,

    DATA,
    DATA_WB, // doesn't appear to be used rn

    REQ_INVALID,

    INVACK,
    ACK,

    DATA_E,

    GETX,

    DATA_F,

    RECALL_GOTO_I,
    RECALL_GOTO_S,

    REQ_MESSAGE_NUM  // Use this to make a Stat Array of message types
} message_t;

extern const char* message_t_str[REQ_MESSAGE_NUM];

#endif  // MESSAGES_H
