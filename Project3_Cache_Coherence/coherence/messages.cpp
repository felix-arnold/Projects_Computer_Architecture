#include "messages.h"

const char* message_t_str[REQ_MESSAGE_NUM] = {

    "NOP",

    "LOAD",          "STORE",

    "GETS",          "GETM",

    "DATA",          "DATA_WB",

    "REQ_INVALID",

    "INVACK",        "ACK",

    "DATA_E",

    "GETX",

    "DATA_F",

    "RECALL_GOTO_I", "RECALL_GOTO_S"};
