#include <stdio.h>
#include "bundle_tools.h"
#include "definitions.h"
#include <al_bp_api.h>
#include <arpa/inet.h>


// static variables for stream operations
static char * buffer = NULL;
static u32_t buffer_len = 0;


/* ----------------------------------------------
 * bundles_needed
 * ---------------------------------------------- */
long bundles_needed (long data, long pl)
{
    long res = 0;
    ldiv_t r;

    r = ldiv(data, pl);
    res = r.quot;
    if (r.rem > 0)
        res += 1;

    return res;
} // end bundles_needed


/* ----------------------------
 * print_eid
 * ---------------------------- */
void print_eid(char * label, al_bp_endpoint_id_t * eid)
{
    printf("%s [%s]\n", label, eid->uri);
} // end print_eid




void init_info(send_information_t *send_info, int window)
{
    int i;

    for (i = 0; i < window; i++)
    {
        send_info[i].bundle_id.creation_ts.secs = 0;
        send_info[i].bundle_id.creation_ts.seqno = 0;
    }
} // end init_info



long add_info(send_information_t* send_info, al_bp_bundle_id_t bundle_id, struct timeval p_start, int window)
{
    int i;

    static u_int id = 0;
    static int last_inserted = -1;
    for (i = (last_inserted + 1); i < window ; i++)
    {
        if ((send_info[i].bundle_id.creation_ts.secs == 0) && (send_info[i].bundle_id.creation_ts.seqno == 0))
        {
            send_info[i].bundle_id.creation_ts.secs = bundle_id.creation_ts.secs;
            send_info[i].bundle_id.creation_ts.seqno = bundle_id.creation_ts.seqno;
            send_info[i].send_time.tv_sec = p_start.tv_sec;
            send_info[i].send_time.tv_usec = p_start.tv_usec;
            send_info[i].relative_id = id;
            last_inserted = i;
            return id++;
        }
    }
    for (i = 0; i <= last_inserted ; i++)
    {
        if ((send_info[i].bundle_id.creation_ts.secs == 0) && (send_info[i].bundle_id.creation_ts.seqno == 0))
        {
            send_info[i].bundle_id.creation_ts.secs = bundle_id.creation_ts.secs;
            send_info[i].bundle_id.creation_ts.seqno = bundle_id.creation_ts.seqno;
            send_info[i].send_time.tv_sec = p_start.tv_sec;
            send_info[i].send_time.tv_usec = p_start.tv_usec;
            send_info[i].relative_id = id;
            last_inserted = i;
            return id++;
        }
    }
    return -1;
} // end add_info


int is_in_info(send_information_t* send_info, al_bp_timestamp_t bundle_timestamp, int window)
{
    int i;

    static int last_collected = -1;
    for (i = (last_collected + 1); i < window; i++)
    {
        if ((send_info[i].bundle_id.creation_ts.secs == bundle_timestamp.secs) && (send_info[i].bundle_id.creation_ts.seqno == bundle_timestamp.seqno))
    	{
            last_collected = i;
            return i;
        }
    }
    for (i = 0; i <= last_collected; i++)
    {
    	if ((send_info[i].bundle_id.creation_ts.secs == bundle_timestamp.secs) && (send_info[i].bundle_id.creation_ts.seqno == bundle_timestamp.seqno))
    	{
            last_collected = i;
            return i;
        }

    }
    return -1;
} // end is_in_info

int count_info(send_information_t* send_info, int window)
{
	int i, count = 0;
	for (i = 0; i < window; i++)
	{
		if (send_info[i].bundle_id.creation_ts.secs != 0)
		{
			count++;
		}
	}
	return count;
}

void remove_from_info(send_information_t* send_info, int position)
{
    send_info[position].bundle_id.creation_ts.secs = 0;
    send_info[position].bundle_id.creation_ts.seqno = 0;
} // end remove_from_info


void set_bp_options(al_bp_bundle_object_t *bundle, dtnperf_connection_options_t *opt)
{
	al_bp_bundle_delivery_opts_t dopts = BP_DOPTS_NONE;

	// Bundle expiration
	al_bp_bundle_set_expiration(bundle, opt->expiration);

	// Bundle priority
	al_bp_bundle_set_priority(bundle, opt->priority);

	// Delivery receipt option
	if (opt->delivery_receipts)
		dopts |= BP_DOPTS_DELIVERY_RCPT;

	// Forward receipt option
	if (opt->forwarding_receipts)
		dopts |= BP_DOPTS_FORWARD_RCPT;

	// Custody transfer
	if (opt->custody_transfer)
		dopts |= BP_DOPTS_CUSTODY;


	// Custody receipts
	if (opt->custody_receipts)
		dopts |= BP_DOPTS_CUSTODY_RCPT;

	// Receive receipt
	if (opt->receive_receipts)
		dopts |= BP_DOPTS_RECEIVE_RCPT;

	//Disable bundle fragmentation

	if (opt->disable_fragmentation)
		dopts |= BP_DOPTS_DO_NOT_FRAGMENT;

	//Set options
	al_bp_bundle_set_delivery_opts(bundle, dopts);

} // end set_bp_options

int open_payload_stream_read(al_bp_bundle_object_t bundle, FILE ** f)
{
	al_bp_bundle_payload_location_t pl_location;
	char * buffer;
	u32_t buffer_len;

	al_bp_bundle_get_payload_location(bundle, &pl_location);

	if (pl_location == BP_PAYLOAD_MEM)
	{
		al_bp_bundle_get_payload_mem(bundle, &buffer, &buffer_len);
		*f = fmemopen(buffer, buffer_len, "rb");
		if (*f == NULL)
			return -1;
	}
	else
	{
		al_bp_bundle_get_payload_file(bundle, &buffer, &buffer_len);
		*f = fopen(buffer, "rb");
		if (*f == NULL)
		{
			perror("open");
			return -1;
		}
	}
	return 0;
}

int close_payload_stream_read(FILE * f)
{
	return fclose(f);
}

int open_payload_stream_write(al_bp_bundle_object_t bundle, FILE ** f)
{
	al_bp_bundle_payload_location_t pl_location;

	al_bp_bundle_get_payload_location(bundle, &pl_location);

	if (pl_location == BP_PAYLOAD_MEM)
	{
		al_bp_bundle_get_payload_mem(bundle, &buffer, &buffer_len);
		*f= open_memstream(&buffer, (size_t *) &buffer_len);
		if (*f == NULL)
			return -1;
	}
	else
	{
		al_bp_bundle_get_payload_file(bundle, &buffer, &buffer_len);
		*f = fopen(buffer, "wb");
		if (*f == NULL)
			return -1;
	}
	return 0;
}

int close_payload_stream_write(al_bp_bundle_object_t * bundle, FILE *f)
{
	al_bp_bundle_payload_location_t pl_location;
	al_bp_bundle_get_payload_location(*bundle, &pl_location);

	fclose(f);
	if (pl_location == BP_PAYLOAD_MEM)
	{
		al_bp_bundle_set_payload_mem(bundle, buffer, buffer_len);
	}
	else
	{
		al_bp_bundle_set_payload_file(bundle, buffer, buffer_len);
	}
	return 0;
}

al_bp_error_t prepare_payload_header_and_ack_options(dtnperf_options_t *opt, FILE * f)
{
	if (f == NULL)
		return BP_ENULLPNTR;

	HEADER_TYPE header;
	BUNDLE_OPT_TYPE options;

	// header
	switch(opt->op_mode)
	{
	case 'T':
		header = TIME_HEADER;
		break;
	case 'D':
		header = DATA_HEADER;
		break;
	case 'F':
		header = FILE_HEADER;
		break;
	default:
		return BP_EINVAL;
	}

	// options
	options = 0;
	// ack to client
	if (opt->bundle_ack_options.ack_to_client)
		options |= BO_ACK_CLIENT_YES;
	else
		options |= BO_ACK_CLIENT_NO;
	//ack to monitor
	if (opt->bundle_ack_options.ack_to_mon == ATM_NORMAL)
		options |= BO_ACK_MON_NORMAL;
	else if (opt->bundle_ack_options.ack_to_mon == ATM_FORCE_YES)
		options |= BO_ACK_MON_FORCE_YES;
	else if (opt->bundle_ack_options.ack_to_mon == ATM_FORCE_NO)
		options |= BO_ACK_MON_FORCE_NO;
	// bundle ack expiration time
	if (opt->bundle_ack_options.set_ack_expiration)
		options |= BO_SET_EXPIRATION;
	// bundle ack priority
	if (opt->bundle_ack_options.set_ack_priority)
	{
		options |= BO_SET_PRIORITY;
		switch (opt->bundle_ack_options.priority.priority)
		{
		case BP_PRIORITY_BULK:
			options |= BO_PRIORITY_BULK;
			break;
		case BP_PRIORITY_NORMAL:
			options |= BO_PRIORITY_NORMAL;
			break;
		case BP_PRIORITY_EXPEDITED:
			options |= BO_PRIORITY_EXPEDITED;
			break;
		case BP_PRIORITY_RESERVED:
			options |= BO_PRIORITY_RESERVED;
			break;
		default:
			break;
		}
	}

	// write in payload
	fwrite(&header, HEADER_SIZE, 1, f);
	fwrite(&options, BUNDLE_OPT_SIZE, 1, f);

	return BP_SUCCESS;
}

int get_bundle_header_and_options(al_bp_bundle_object_t * bundle, HEADER_TYPE * header, dtnperf_bundle_ack_options_t * options)
{
	if (bundle == NULL)
		return -1;
	BUNDLE_OPT_TYPE opt;
	FILE * pl_stream = NULL;
	open_payload_stream_read(*bundle, &pl_stream);

	if (header != NULL)
	{
		// read header
		fread(header, HEADER_SIZE, 1, pl_stream);
	}
	else
	{
		// skip header
		fseek(pl_stream, HEADER_SIZE, SEEK_SET);
	}

	if (options != NULL)
	{
		// initiate options
		options->ack_to_client = FALSE;
		options->ack_to_mon = ATM_NORMAL;
		options->set_ack_expiration = FALSE;
		options->set_ack_priority = FALSE;

		// read options
		fread(&opt, BUNDLE_OPT_SIZE, 1, pl_stream);

		// ack to client
		if ((opt & BO_ACK_CLIENT_MASK) == BO_ACK_CLIENT_YES)
			options->ack_to_client = TRUE;
		else if ((opt & BO_ACK_CLIENT_MASK) == BO_ACK_CLIENT_NO)
			options->ack_to_client = FALSE;

		// ack to mon
		if ((opt & BO_ACK_MON_MASK) == BO_ACK_MON_NORMAL)
			options->ack_to_mon = ATM_NORMAL;
		if ((opt & BO_ACK_MON_MASK) == BO_ACK_MON_FORCE_YES)
			options->ack_to_mon = ATM_FORCE_YES;
		else if ((opt & BO_ACK_MON_MASK) == BO_ACK_MON_FORCE_NO)
			options->ack_to_mon = ATM_FORCE_NO;

		// expiration
		if (opt & BO_SET_EXPIRATION)
			options->set_ack_expiration = TRUE;

		// priority
		if (opt & BO_SET_PRIORITY)
		{
			options->set_ack_priority = TRUE;
			if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_BULK)
				options->priority.priority = BP_PRIORITY_BULK;
			else if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_NORMAL)
				options->priority.priority = BP_PRIORITY_NORMAL;
			else if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_EXPEDITED)
				options->priority.priority = BP_PRIORITY_EXPEDITED;
			else if ((opt & BO_PRIORITY_MASK) == BO_PRIORITY_RESERVED)
				options->priority.priority = BP_PRIORITY_RESERVED;
		}
	}
	return 0;
}

al_bp_error_t prepare_generic_payload(dtnperf_options_t *opt, FILE * f)
{
	if (f == NULL)
		return BP_ENULLPNTR;

	char * pattern = PL_PATTERN;
	long remaining;
	int i;
	al_bp_error_t result;

	// prepare header and congestion control
	result = prepare_payload_header_and_ack_options(opt, f);

	// remaining = bundle_payload - HEADER_SIZE - congestion control char
	remaining = opt->bundle_payload - HEADER_SIZE - 1;

	// fill remainig payload with a pattern
	for (i = remaining; i > strlen(pattern); i -= strlen(pattern))
	{
		fwrite(pattern, strlen(pattern), 1, f);
	}
	fwrite(pattern, remaining % strlen(pattern), 1, f);

	return result;
}

al_bp_error_t prepare_force_stop_bundle(al_bp_bundle_object_t * start, al_bp_endpoint_id_t monitor,
		al_bp_timeval_t expiration, al_bp_bundle_priority_t priority)
{
	FILE * start_stream;
	HEADER_TYPE start_header = FORCE_STOP_HEADER;
	al_bp_endpoint_id_t none;
	al_bp_bundle_delivery_opts_t opts = BP_DOPTS_NONE;
	al_bp_bundle_set_payload_location(start, BP_PAYLOAD_MEM);
	open_payload_stream_write(*start, &start_stream);
	fwrite(&start_header, HEADER_SIZE, 1, start_stream);
	close_payload_stream_write(start, start_stream);
	al_bp_bundle_set_dest(start, monitor);
	al_bp_get_none_endpoint(&none);
	al_bp_bundle_set_replyto(start, none);
	al_bp_bundle_set_delivery_opts(start, opts);
	al_bp_bundle_set_expiration(start, expiration);
	al_bp_bundle_set_priority(start, priority);
	return BP_SUCCESS;
}

al_bp_error_t prepare_stop_bundle(al_bp_bundle_object_t * stop, al_bp_endpoint_id_t monitor,
		al_bp_timeval_t expiration, al_bp_bundle_priority_t priority, int sent_bundles)
{
	FILE * stop_stream;
	HEADER_TYPE stop_header = STOP_HEADER;
	al_bp_endpoint_id_t none;
	uint32_t buf;
	al_bp_bundle_delivery_opts_t opts = BP_DOPTS_NONE;
	al_bp_bundle_set_payload_location(stop, BP_PAYLOAD_MEM);
	open_payload_stream_write(*stop, &stop_stream);
	fwrite(&stop_header, HEADER_SIZE, 1, stop_stream);
	buf = htonl(sent_bundles);
	fwrite(&buf, sizeof(buf), 1, stop_stream);
	close_payload_stream_write(stop, stop_stream);
	al_bp_bundle_set_dest(stop, monitor);
	al_bp_get_none_endpoint(&none);
	al_bp_bundle_set_replyto(stop, none);
	al_bp_bundle_set_delivery_opts(stop, opts);
	al_bp_bundle_set_expiration(stop, expiration);
	al_bp_bundle_set_priority(stop, priority);
	return BP_SUCCESS;
}

al_bp_error_t get_info_from_stop(al_bp_bundle_object_t * stop, int * sent_bundles)
{
	FILE * stop_stream;
	uint32_t buf;
	open_payload_stream_read(*stop, &stop_stream);

	// skip header
	fseek(stop_stream, HEADER_SIZE, SEEK_SET);

	// read sent bundles num
	fread(&buf, sizeof(buf), 1, stop_stream);

	* sent_bundles = (int) ntohl(buf);

	close_payload_stream_read(stop_stream);
	return BP_SUCCESS;
}
/**
 *
 */
al_bp_error_t prepare_server_ack_payload(dtnperf_server_ack_payload_t ack, char ** payload, size_t * payload_size)
{
	FILE * buf_stream;
	char * buf;
	size_t buf_size;
	HEADER_TYPE header = DSA_HEADER;
	uint16_t eid_len;
	uint32_t timestamp_secs;
	uint32_t timestamp_seqno;
	buf_stream = open_memstream(& buf, &buf_size);
	fwrite(&header, 1, HEADER_SIZE, buf_stream);
	eid_len = strlen(ack.bundle_source.uri);
	fwrite(&eid_len, sizeof(eid_len), 1, buf_stream);
	fwrite(&(ack.bundle_source.uri), 1, eid_len, buf_stream);
	timestamp_secs = (uint32_t) ack.bundle_creation_ts.secs;
	timestamp_seqno = (uint32_t) ack.bundle_creation_ts.seqno;
	fwrite(&timestamp_secs, 1, sizeof(uint32_t), buf_stream);
	fwrite(&timestamp_seqno, 1, sizeof(uint32_t), buf_stream);
	fclose(buf_stream);
	*payload = (char*)malloc(buf_size);
	memcpy(*payload, buf, buf_size);
	*payload_size = buf_size;
	free(buf);
	return BP_SUCCESS;
}

al_bp_error_t get_info_from_ack(al_bp_bundle_object_t * ack, al_bp_endpoint_id_t * reported_eid, al_bp_timestamp_t * reported_timestamp)
{
	al_bp_error_t error;
	HEADER_TYPE header;
	FILE * pl_stream;
	uint16_t eid_len;
	uint32_t timestamp_secs, timestamp_seqno;
	open_payload_stream_read(*ack, &pl_stream);
	fread(&header, HEADER_SIZE, 1, pl_stream);
	if (header == DSA_HEADER)
	{
		fread(&eid_len, sizeof(eid_len), 1, pl_stream);
		if (reported_eid != NULL)
		{
			fread(reported_eid->uri, eid_len, 1, pl_stream);
			reported_eid->uri[eid_len] = '\0';

		}
		else
			fseek(pl_stream, eid_len, SEEK_CUR);

		if (reported_timestamp != NULL)
		{
			fread(&timestamp_secs, sizeof(uint32_t), 1, pl_stream);
			fread(&timestamp_seqno, sizeof(uint32_t), 1, pl_stream);
			reported_timestamp->secs = (u32_t) timestamp_secs;
			reported_timestamp->seqno = (u32_t) timestamp_seqno;
		}
		error = BP_SUCCESS;
	}
	else
		error = BP_ERRBASE;

	close_payload_stream_read(pl_stream);
	return error;
}

u32_t get_current_dtn_time()
{
	u32_t result;
	time_t dtn_epoch = (time_t) DTN_EPOCH;
	time_t current = time(NULL);
	result = (u32_t) difftime(current, dtn_epoch);
	return result;
}
