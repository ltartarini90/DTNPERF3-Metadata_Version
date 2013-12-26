/*
 * dtnperf_router.h
 *
 *  Created on: 16/01/2012
 *      Author: luca
 */

#ifndef DTNPERF_ROUTER_H_
#define DTNPERF_ROUTER_H_

#include "../dtnperf_types.h"
#include <stddef.h>

#define BUFSIZE 16
#define SORT_CMD "sortbundle"
#define ORDER_SUB_CMD "order"
#define INIT_TABLE_SUB_CMD "init_table"
#define PRIORITY_TABLE_SUB_CMD "priority_table"

#define DAEMON_COMMAND_SCRIPT "/usr/bin/automatizedtnd.sh"

typedef enum sort_subcommand {
	SORTBUNDLE_ORDER = 0,
        SORTBUNDLE_INIT_TABLE = 1,
        SORTBUNDLE_PRIORITY_TABLE = 2,
} sortbundle_command_t;

void run_dtnperf_router(dtnperf_global_options_t * global_options );

// file expiration timer thread
void * fet(void * opt);

void print_router_usage(char* progname);
void parse_router_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt);
al_bp_error_t prepare_server_ack_payload(dtnperf_server_ack_payload_t ack, 
        char ** payload, size_t * payload_size);

// Ctrl+C handler
void router_handler(int signo);

void router_clean_exit(int status);

static void print_data(char* buffer, u32_t length);

void copy_command(char* dest, char* buffer, u32_t length);

void parse_router_metadata_blocks(al_bp_bundle_object_t* bundle_object, 
        size_t bundle_payload_len, boolean_t debug);

#endif /* DTNPERF_ROUTER_H_ */
