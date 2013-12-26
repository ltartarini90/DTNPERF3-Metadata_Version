/*
 * dtnperf_client.h
 *
 *  Created on: 10/lug/2012
 *      Author: michele
 */

#ifndef DTNPERF_CLIENT_H_
#define DTNPERF_CLIENT_H_

#include "../dtnperf_types.h"
#include <stdio.h>

/******************************** XXX/Tartarini *******************************/
/* 
 * extension/metadata block information
 */
typedef struct extension_block_info
{
    boolean_t metadata;
    u_int64_t metadata_type;
    al_bp_extension_block_t block;
} extension_block_info_t;

/* 
 * metadata functions
 */
boolean_t check_metadata(extension_block_info_t* ext_block);
void set_metadata(extension_block_info_t* ext_block, u_int64_t metadata_type);
al_bp_extension_block_t get_extension_block(extension_block_info_t* ext_block);
void set_block_buf(extension_block_info_t* ext_block, char * buf, u32_t len);
/******************************************************************************/

void run_dtnperf_client(dtnperf_global_options_t * global_options);
void print_client_usage(char* progname);
void parse_client_options(int argc, char ** argv, dtnperf_global_options_t * perf_g_opt);
void check_options(dtnperf_global_options_t * global_options);
void print_final_report(FILE * f);
void client_handler(int sig);
void client_clean_exit(int status);

//pthread functions
void * send_bundles(void * opt);
void * congestion_control(void * opt);
void * start_dedicated_monitor(void * params);
void * wait_for_sigint(void * arg);
#endif /* DTNPERF_CLIENT_H_ */
