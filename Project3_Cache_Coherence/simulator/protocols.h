/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef enum {
    CACHE_PRO = 0,
    MI_PRO,
    MSI_PRO,
    MESI_PRO,
    MOESI_PRO,
    MOSI_PRO,
    MESIF_PRO,
    MOESIF_PRO
} protocol_t;

extern const char *proto_str[8];

#endif  // PROTOCOLS_H
