/*
 * dtnperf_main.c
 *
 *  Created on: 09/mar/2012
 *      Author: michele
 */

#include "includes.h"
#include "utils.h"
#include "dtnperf_types.h"
#include "definitions.h"
#include "dtnperf_modes/dtnperf_client.h"
#include "dtnperf_modes/dtnperf_server.h"
#include "dtnperf_modes/dtnperf_monitor.h"
#include "dtnperf_modes/dtnperf_router.h"

/* ---------------------------
 * Global variables and options
 * --------------------------- */
dtnperf_global_options_t global_options;



/* -------------------------------
 *       function interfaces
 * ------------------------------- */

void parse_options(int argc, char** argv, dtnperf_global_options_t * global_opt);
void print_usage(char* progname);
void init_dtnperf_global_options(dtnperf_global_options_t *, dtnperf_options_t *, dtnperf_connection_options_t *);
void init_dtnperf_options(dtnperf_options_t *);
void init_dtnperf_connection_options(dtnperf_connection_options_t*);


/* -------------------------------
 *              MAIN
 * ------------------------------- */
int main(int argc, char ** argv)
{
	// variable declarations
	dtnperf_global_options_t global_options;
	dtnperf_options_t perf_opt;
	dtnperf_connection_options_t conn_opt;
	monitor_parameters_t mon_params;
	int fd, pid;

	// init options
	init_dtnperf_global_options(&global_options, &perf_opt, &conn_opt);

	// parse command line options
	parse_options(argc, argv, &global_options);

	switch (global_options.mode)
	{
	case DTNPERF_SERVER:
		if (perf_opt.daemon)
		{
			if ((fd = open(perf_opt.server_output_file,O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
			{
				pid = fork();
				if (pid == 0)
				{
					close(1);
					close(2);
					dup(fd);
					dup(fd);
					run_dtnperf_server(&global_options);
					close(1);
					close(2);
					close(fd);
				}
				else
				{
					close(fd);
					printf("Started dtnperf server in daemon mode.\n");
					printf("Pid = %d\n", pid);
					printf("To terminate the daemon use:\n");
					printf("\t%s %s --stop\n", argv[0], SERVER_STRING);
					printf("See log at %s\n", perf_opt.server_output_file);
				}
			}
			else
			{
				printf("ERROR: failed to open output file %s: %s\n", perf_opt.server_output_file, strerror(errno));
				exit(1);
			}
		}
		else
		{
			run_dtnperf_server(&global_options);
		}
		break;

	case DTNPERF_CLIENT_MONITOR:
	case DTNPERF_CLIENT:
		run_dtnperf_client(&global_options);
		break;

	case DTNPERF_MONITOR:
		mon_params.client_id = 0;
		mon_params.dedicated_monitor = FALSE;
		mon_params.perf_g_opt = &global_options;
		if (perf_opt.daemon)
				{
					if ((fd = open(perf_opt.monitor_output_file,O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
					{
						pid = fork();
						if (pid == 0)
						{
							close(1);
							close(2);
							dup(fd);
							dup(fd);
							run_dtnperf_monitor(&mon_params);
							close(1);
							close(2);
							close(fd);
						}
						else
						{
							close(fd);
							printf("Started dtnperf monitor in daemon mode.\n");
							printf("Pid = %d\n", pid);
							printf("To terminate the daemon use:\n");
							printf("\t%s %s --stop\n", argv[0], MONITOR_STRING);
							printf("See log at %s\n", perf_opt.monitor_output_file);
						}
					}
					else
					{
						printf("ERROR: failed to open output file %s: %s\n", perf_opt.monitor_output_file, strerror(errno));
						exit(1);
					}
				}
				else
				{
					run_dtnperf_monitor(&mon_params);
				}
		break;
		/******************************************************************************/
                case DTNPERF_ROUTER:
		if (perf_opt.daemon)
		{
			if ((fd = open(perf_opt.router_output_file,O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0)
			{
				pid = fork();
				if (pid == 0)
				{
					close(1);
					close(2);
					dup(fd);
					dup(fd);
					run_dtnperf_router(&global_options);
					close(1);
					close(2);
					close(fd);
				}
				else
				{
					close(fd);
					printf("Started dtnperf router in daemon mode.\n");
					printf("Pid = %d\n", pid);
					printf("To terminate the daemon use:\n");
					printf("\t%s %s --stop\n", argv[0], ROUTER_STRING);
					printf("See log at %s\n", perf_opt.router_output_file);
				}
			}
			else
			{
				printf("ERROR: failed to open output file %s: %s\n", perf_opt.router_output_file, strerror(errno));
				exit(1);
			}
		}
		else
		{
			run_dtnperf_router(&global_options);
		}
		break;
	/******************************************************************************/
	default:
		fprintf(stderr,"error in switching dtnperf mode");
		exit(-1);
	}

	exit(0);
}

void print_usage(char* progname){
	fprintf(stderr, "\n");
	fprintf(stderr, "DTNperf version %s\n", DTNPERF_VERSION);
	fprintf(stderr, "SYNTAX: %s <operative mode> [options]\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "operative modes:\n");
	fprintf(stderr, " %s\n", SERVER_STRING);
	fprintf(stderr, " %s\n", CLIENT_STRING);
	fprintf(stderr, " %s\n", MONITOR_STRING);
	/******************************************************************************/
        fprintf(stderr, " %s\n", ROUTER_STRING);
	/******************************************************************************/
	fprintf(stderr, "\n");
	fprintf(stderr, "For more options see\n %s <operative mode> --help\n", progname);
	fprintf(stderr, " %s --help\tPrint this screen.\n", progname);
	fprintf(stderr, " %s --Version\tShow version information.\n", progname);
	fprintf(stderr, "\n");
	exit(1);
}

void parse_options(int argc, char**argv, dtnperf_global_options_t * global_opt)
{
	int i;
	dtnperf_mode_t perf_mode = 0;


	// find dtnperf mode (server, client or monitor)
	if (argc < 2)
	{
		print_usage(argv[0]);
		exit(1);
	}
	if (strcmp(argv[1], SERVER_STRING) == 0)
	{
		perf_mode = DTNPERF_SERVER;
	}
	else if (strcmp(argv[1], CLIENT_STRING) == 0)
	{
		perf_mode = DTNPERF_CLIENT;
	}
	else if (strcmp(argv[1], MONITOR_STRING) == 0)
	{
		perf_mode = DTNPERF_MONITOR;
	}
	/******************************************************************************/
        else if (strcmp(argv[1], ROUTER_STRING) == 0)
	{
		perf_mode = DTNPERF_ROUTER;
	}
	/******************************************************************************/
	else if (strcmp(argv[1], "--help") == 0) // general help option
	{
		print_usage(argv[0]);
		exit(0);
	}
	else if (strcmp(argv[1], "--version") == 0) // print program version
	{
		printf("DTNperf version %s\n", DTNPERF_VERSION);
		exit(0);
	}
	else
	{
		fprintf(stderr, "dtnperf mode must be specified as first argument\n");
		print_usage(argv[0]);
		exit(1);
	}

	//scroll argv array
	for(i = 2; i < argc; i++)
	{
		argv[i-1] = argv[i];
	}
	argc = argc -1;

	switch(perf_mode)
	{
	case DTNPERF_CLIENT:
		parse_client_options(argc, argv, global_opt);
		if (global_opt->perf_opt->mon_eid[0] == '\0')
			perf_mode = DTNPERF_CLIENT_MONITOR;
		break;
	case DTNPERF_SERVER:
		parse_server_options(argc, argv, global_opt);
		break;
	case DTNPERF_MONITOR:
		parse_monitor_options(argc, argv, global_opt);
		break;
	/******************************************************************************/
        case DTNPERF_ROUTER:
                parse_router_options(argc, argv, global_opt);
                break;
	/******************************************************************************/
	default:
		fprintf(stderr, "error in parsing options\n");
		print_usage(argv[0]);
		exit(1);
	}

	// insert perf_mode in global options
	global_opt->mode = perf_mode;

} // end parse_options


void init_dtnperf_global_options(dtnperf_global_options_t *opt, dtnperf_options_t * perf_opt, dtnperf_connection_options_t * conn_opt)
{
	opt->perf_opt = perf_opt;
	opt->conn_opt = conn_opt;
	init_dtnperf_options(opt->perf_opt);
	init_dtnperf_connection_options(opt->conn_opt);
	opt->mode = 0;
}

void init_dtnperf_options(dtnperf_options_t *opt)
{
	opt->verbose = FALSE;
	opt->debug = FALSE;
	opt->debug_level = 0;
	opt->use_ip = FALSE;
	opt->ip_addr = "127.0.0.1";
	opt->ip_port = 5010;
	opt->daemon = FALSE;
	opt->server_output_file = SERVER_OUTPUT_FILE;
	opt->monitor_output_file = MONITOR_OUTPUT_FILE;
	/******************************************************************************/
        opt->router_output_file = ROUTER_OUTPUT_FILE;
	/******************************************************************************/
	memset(opt->dest_eid, 0, AL_BP_MAX_ENDPOINT_ID);
	memset(opt->mon_eid, 0, AL_BP_MAX_ENDPOINT_ID);
	opt->op_mode = 'D';
	opt->data_qty = 0;
	opt->D_arg = NULL;
	opt->F_arg = NULL;
	opt->p_arg = NULL;
	opt->use_file = 1;
	opt->data_unit = 'M';
	opt->transmission_time = 0;
	opt->congestion_ctrl = 'w';
	opt->window = 1;
	opt->rate_arg = NULL;
	opt->rate = 0;
	opt->rate_unit = 'b';
	opt->wait_before_exit = 0;
	opt->bundle_payload = DEFAULT_PAYLOAD;
	opt->payload_type = BP_PAYLOAD_MEM;
	opt->dest_dir = BUNDLE_DIR_DEFAULT;
	opt->file_dir = FILE_DIR_DEFAULT;
	opt->create_log = FALSE;
	opt->log_filename = LOG_FILENAME;
	opt->acks_to_mon = FALSE;
	opt->no_acks = FALSE;
	opt->logs_dir = LOGS_DIR_DEFAULT;
	opt->bundle_ack_options.ack_to_client = TRUE;
	opt->bundle_ack_options.ack_to_mon = ATM_NORMAL;;
	opt->bundle_ack_options.set_ack_expiration = FALSE;
	opt->bundle_ack_options.set_ack_priority = FALSE;
        /**********************************************************************/
        opt->num_blocks = 0;
        opt->metadata_type = -1;
        /**********************************************************************/
}



void init_dtnperf_connection_options(dtnperf_connection_options_t* opt)
{
	opt->expiration = 3600;				// expiration time (sec) [3600]
	opt->delivery_receipts = TRUE;		// request delivery receipts [1]
	opt->forwarding_receipts = FALSE;   // request per hop departure [0]
	opt->custody_transfer = FALSE;   	// request custody transfer [0]
	opt->custody_receipts = FALSE;   	// request per custodian receipts [0]
	opt->receive_receipts = FALSE;   	// request per hop arrival receipt [0]
	opt->wait_for_report = TRUE;   		// wait for bundle status reports [1]
	opt->disable_fragmentation = FALSE; //disable bundle fragmentation[0]
	opt->priority.priority = BP_PRIORITY_NORMAL; // bundle priority [BP_PRIORITY_NORMAL]
}
