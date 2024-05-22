/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "settings.h"
#include "sim.h"

extern char *optarg;
extern int optind, optopt;

settings_t settings;
Simulator *sim;

char *protocol = NULL;
char *trace_dir = NULL;
int nproc = 0;
char *cputype = NULL;
char *nettype = NULL;
int link_width = 0;
int mem_latency = 0;

/** Fatal Error.  */
void fatal_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fflush(stdout);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    /** Enable debugging by asserting zero.  */
    assert(0 && "Fatal Error");
    if (sim) delete sim;
    init_cleanup();
    exit(-1);
}

/** debug_printing  */
void debug_printf(const char *fmt, ...) {
#ifdef DEBUG
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
#endif
(void) fmt;
}

void usage(void) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\t-p <protocol> (choices: MSI, MESI, MOSI, MOESIF)\n"); // no MOESI, MI implementation provided but unused
    fprintf(stderr, "\t-t <trace directory>\n");
    fprintf(stderr, "\t-n <number of processors> (choices: (4,8,12,16))\n");
    fprintf(stderr, "\t-c <cpu type> (choices: FICI)\n"); // FICO/FOCO Disabled (no existing implementation)
    fprintf(stderr, "\t-i <interconnect_topology> (choices: RING, XBAR)\n"); // XBAR support is experimental
    fprintf(stderr, "\t-l <link latency> (choices: 1)\n"); // only 1 cycle for now
    fprintf(stderr, "\t-m <memory/dram latency> (choices: 100)\n"); // 100 is fine for now
    fprintf(stderr, "\t-h prints this useful message\n");
}

void init_cleanup() {
    if (protocol) free(protocol);
    if (trace_dir) free(trace_dir);
    if (cputype) free(cputype);
    if (nettype) free(nettype);
}

void printconf() {
    fprintf(stdout, "Selected Configuration:\n");
    fprintf(stdout, "\tProtocol: %s\n", proto_str[settings.protocol]);
    fprintf(stdout, "\tTrace Directory: %s\n", settings.trace_dir);
    fprintf(stdout, "\tNum Procs: %lu\n", settings.num_procs);
    fprintf(stdout, "\tCPU TYPE: %s\n", cputype_str[settings.cputype]);
    fprintf(stdout, "\tNetwork Topology: %s\n", nettype_str[settings.nettype]);
    fprintf(stdout, "\tLink Width: %lu bytes\n", (1ul << settings.link_width));
    fprintf(stdout, "\tMemory Latency: %lu\n", settings.mem_latency);
    fprintf(stdout, "\tBlock Size: %lu\n", settings.block_size);
}

int main(int argc, char *argv[]) {
    // DEFAULT CONFIG:
    protocol = strdup("MSI");
    nproc = 4;
    cputype = strdup("FICI");
    nettype = strdup("RING");
    link_width = 3;
    mem_latency = 100;

    int o;
    while ((o = getopt(argc, argv, "HhP:p:T:t:N:n:C:c:I:i:L:l:M:m:")) != -1) {
        switch (o) {
            case 'h':
            case 'H':
                usage();
                exit(0);
                break;

            case 'p':
            case 'P':
                free(protocol);
                protocol = strdup(optarg);
                break;

            case 't':
            case 'T':
                trace_dir = strdup(optarg);
                break;

            case 'n':
            case 'N':
                nproc = atoi(optarg);
                break;

            case 'c':
            case 'C':
                free(cputype);
                cputype = strdup(optarg);
                break;

            case 'i':
            case 'I':
                free(nettype);
                nettype = strdup(optarg);
                break;

            case 'l':
            case 'L':
                link_width = atoi(optarg);
                break;

            case 'm':
            case 'M':
                mem_latency = atoi(optarg);
                break;

            default:
                fprintf(stderr, "Invalid command line arguments - %c", o);
                usage();
                return 1;
        }
    }

    if (protocol == NULL) {
        fprintf(stderr, "Error: no protocol specified.\n");
        init_cleanup();
        usage();
        return 1;
    }
    if (trace_dir == NULL) {
        fprintf(stderr, "Error: no trace directory specified.\n");
        init_cleanup();
        usage();
        return 1;
    }
    if (!((nproc == 4) || (nproc == 8) || (nproc == 12) || (nproc == 16))) {
        fprintf(stderr, "Error: invalid number of processors specified: %d.\n", nproc);
        init_cleanup();
        usage();
        return 1;
    }
    if (cputype == NULL) {
        fprintf(stderr, "Error: no cpu type specified.\n");
        init_cleanup();
        usage();
        return 1;
    }
    if (nettype == NULL) {
        fprintf(stderr, "Error: no interconnect topology specified.\n");
        init_cleanup();
        usage();
        return 1;
    }
    if (link_width == 0) {
        fprintf(stderr, "Error: no link latency specified.\n");
        init_cleanup();
        usage();
        return 1;
    }
    if (mem_latency != 100) {
        fprintf(stderr, "Error: no memory latency specified.\n");
        init_cleanup();
        usage();
        return 1;
    }

    if (!strcmp(protocol, "MI")) {
        settings.protocol = MI_PRO;
    } else if (!strcmp(protocol,"MSI")) {
        settings.protocol = MSI_PRO;
    } else if (!strcmp(protocol, "MESI")) {
        settings.protocol = MESI_PRO;
    } else if (!strcmp(protocol, "MOSI")) {
        settings.protocol = MOSI_PRO;
    } else if (!strcmp(protocol, "MOESI")) {
        settings.protocol = MOESI_PRO;
        fprintf(stderr, "Unsupported protocol MOESI\n");
        init_cleanup();
        exit(-1);
    } else if (!strcmp(protocol, "MESIF")) {
        settings.protocol = MESIF_PRO;
    } else if (!strcmp(protocol,"MOESIF")) {
        settings.protocol = MOESIF_PRO;
    } else {
        fprintf(stderr, "Error: invalid protocol specified.\n");
        init_cleanup();
        exit(-1);
    }

    settings.trace_dir = trace_dir;
    settings.num_procs = (nodeID_t)nproc;

    if (!strcmp(cputype, "FICI")) {
        settings.cputype = FICI_CPU;
    } else if (!strcmp(cputype, "FICO")) {
        settings.cputype = FICO_CPU;
        fprintf(stderr, "Unsupported cpu FICO\n");
        exit(-1);
    } else if (!strcmp(cputype, "FOCO")) {
        settings.cputype = FOCO_CPU;
        fprintf(stderr, "Unsupported cpu FOCO\n");
        exit(-1);
    } else {
        fprintf(stderr, "Error: invalid cpu type specified.\n");
        exit(-1);
    }

    if (!strcmp(nettype, "RING")) {
        settings.nettype = RING_TOP;
    } else if (!strcmp(nettype, "XBAR")) {
        printf("Warning: XBAR support is experimental\n");
        settings.nettype = XBAR_TOP;
    } else {
        fprintf(stderr, "Error: invalid interconnect topology specified.\n");
        exit(-1);
    }

    settings.link_width = link_width;
    settings.mem_latency = mem_latency;
    settings.block_size = 6;
    settings.header_flits = 1 << (4 - link_width);
    settings.payload_flits = 1 << (settings.block_size - settings.link_width);


    printconf();

    sim = new Simulator();
    sim->run();
    sim->dump_stats();
    delete sim;
    init_cleanup();
}
