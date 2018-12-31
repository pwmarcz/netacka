/*----------------------------------------------------------------
 *   connhelp.c -- helper conn functions for drivers to use
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

/* These functions encapsulate the conn system on top of the channel
 * system.  Drivers can support the channels themselves, and set the
 * conn function entries in their struct to NULL.  Libnet will then
 * automatically redirect them to point at this driver, which chains
 * back to the real driver using the normal channel functions.
 */


#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platdefs.h"
#include "libnet.h"
#include "internal.h"
#include "connhelp.h"
#include "config.h"
#include "timer.h"

/*---------------------------------------- Settings for conn queues */

struct conn_config {

	/* Queue sizes -- these must be powers of two */
	int outqueue_size, inqueue_size;

	/* Time between resends of unacknowledged outgoing packets (ms) */
	int resend_rate;
};

/* Defaults */
static struct conn_config conn_config_default = { 16, 16, 500 };

/* I'm feeling lazy, so I won't change these properly all through the code */
#define CONN_CONFIG (conn->driver->conn_config?conn->driver->conn_config:&conn_config_default)
#define MAX_OUTGOING_PACKETS   (CONN_CONFIG->outqueue_size)
#define MAX_INCOMING_PACKETS   (CONN_CONFIG->inqueue_size)
#define RESEND_RATE            (CONN_CONFIG->resend_rate)


/*---------------------------------------- Data structures */

/* The type used for index numbers.  This must be capable of storing 32-bit
 * unsigned integers. */
typedef unsigned int index_t;

/* Outgoing packet */
struct out_packet_t {
	index_t index;
	int ack;
	int last_send_time;
	int size;
	void *data;
};

/* Incoming packet */
struct in_packet_t {
	int size;
	void *data;
};

/* Outgoing packet queue */
struct out_t {
	struct out_packet_t *packets;
	index_t base_index;
	index_t next_index;
};

/* Incoming packet queue */
struct in_t {
	struct in_packet_t *packets;
	index_t base_index;
};

/* Connection list */
struct conns_list {
	struct conns_list *next;
	NET_CHANNEL *chan;        /* channel we created for this client */
	char *addr;               /* address of connecting client */
	int client_conn_id;       /* client's quasi-unique ref for this conn */
	struct conn_data_t *ref;  /* cross-reference to conn_data */
	int last_access_time;     /* time when we were last used */
};

/* Internal data */
struct conn_data_t {
	NET_CHANNEL *chan;
	char connect_string[12];
	struct conns_list *conns;
	struct out_t out;
	struct in_t in;
	struct conn_data_t *referer; /* pointer to listening conn refering to us */
	int connect_timestamp;
};


/*---------------------------------------- Driver conn functions */


/* create_queues:
 *  This sets up the packet queues for a conn.
 */
static int create_queues (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;
	int i;
	
	data->in.packets = malloc (MAX_INCOMING_PACKETS * sizeof *data->in.packets);
	if (!data->in.packets) return 1;

	for (i = 0; i < MAX_INCOMING_PACKETS; i++)
		data->in.packets[i].data = NULL;
	data->in.base_index = 1;

	data->out.packets = malloc (MAX_OUTGOING_PACKETS * sizeof *data->out.packets);
	if (!data->out.packets) {
		free (data->in.packets);
		return 1;
	}

	for (i = 0; i < MAX_OUTGOING_PACKETS; i++)
		data->out.packets[i].data = NULL;
	data->out.next_index = data->out.base_index = 1;
	
	return 0;
}

static void destroy_queues (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;
	int i;

	for (i = 0; i < MAX_INCOMING_PACKETS; i++)
		if (data->in.packets[i].data)
			free (data->in.packets[i].data);
	for (i = 0; i < MAX_OUTGOING_PACKETS; i++)
		if (data->out.packets[i].data)
			free (data->out.packets[i].data);
	free (data->in.packets);
	free (data->out.packets);
}


/* init_conn:
 *  This just chains to the channel initialiser, basically.
 */
static int init_conn (NET_CONN *conn, const char *addr)
{
	struct conn_data_t *data;
	
	conn->peer_addr[0] = '\0';	
	conn->data = data = malloc (sizeof *data);
	if (!data) return 1;
	
	if (create_queues (conn)) {
		free (data);
		return 2;
	}
	
	data->conns = NULL;
	data->referer = NULL;
	
	data->chan = net_openchannel (conn->type, addr);
	if (!data->chan) {
		destroy_queues (conn);
		free (data);
		return 3;
	}

	return 0; /* success */
}


static void destroy_conns_list (struct conns_list *conns)
{
	struct conns_list *c;
	while (conns) {
		c = conns->next;
		if (conns->ref)
			conns->ref->referer = NULL;

		free (conns->addr);
		free (conns);
		conns = c;
	}
}


static int destroy_conn (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;

	if (data->referer)
	{
		/* remove us from refering list in listener connection */
		struct conns_list *prev = NULL, *list = data->referer->conns;
		while (list)
		{
			if (list->chan == data->chan)
			{
				struct conns_list *old = list;

				if (prev)
					prev->next = list->next;
				else
					data->referer->conns = list->next;

				list = list->next;

				free(old->addr);
				free(old);
			} else
			{
				prev = list;
				list = list->next;
			}
		}
	}

	net_closechannel (data->chan);
	destroy_queues (conn);
	destroy_conns_list (data->conns);

	free (data);
	return 0;
}


/* listen:
 *  Here we need to set up the stub for the list of connectors.  We need
 *  this list because there's a (high) chance that we'll get multiple
 *  copies of the same connection request.  The list notes which
 *  addresses have requested connections, and which channel was created
 *  for each, so that we can return the same channel rather than opening
 *  another one.
 */
static int listen (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;
	
	data->conns = malloc (sizeof *data->conns);
	if (!data->conns) return 1;
	
	data->conns->next = NULL;
	data->conns->addr = NULL;
	data->conns->chan = NULL;
	data->conns->client_conn_id = -1;
	data->conns->ref = NULL;
	
	return 0;
}


/* get_channel:
 *  Scans the list of connections for one matching this connector, and 
 *  fills it in if found, returning positive.  Otherwise, adds a new entry
 *  to the list, fills it in, and returns negative.  On error, returns zero.
 */
static int get_channel (struct conns_list *conns, const char *addr, int conn_id, int type, const char *bind, NET_CHANNEL **chan, struct conn_data_t *condat)
{
#if 0
	/* Before we do anything else, time out entries which have been here 
	 * too long */
	struct conns_list *ptr = conns;
	while (ptr->next) {
		if ((unsigned)(__libnet_timer_func() - ptr->last_access_time) > 10000) {
			struct conns_list *ptr2 = ptr->next;
			ptr->next = ptr2->next;
			free (ptr2->addr);
			free (ptr2);
		}
	}
#endif

	while (conns->next) {
		conns = conns->next;
		if ((conn_id == conns->client_conn_id) && !strcmp (addr, conns->addr)) {
			*chan = conns->chan;
			conns->last_access_time = __libnet_timer_func();
			return 1;
		}
	}
	conns->next = malloc (sizeof *conns->next);
	if (conns->next) {
		conns->next->next = NULL;
		conns->next->addr = strdup (addr);
		conns->next->client_conn_id = conn_id;
		conns->next->ref = condat;
		conns->next->last_access_time = __libnet_timer_func();
		if (conns->next->addr) {
			conns->next->chan = net_openchannel (type, bind);
			if (conns->next->chan) {
				*chan = conns->next->chan;
				return -1;
			}

			free (conns->next->addr);
		}
		free (conns->next);
		conns->next = NULL;
	}
	return 0;
}


/* poll_listen:
 *  Here we check for an incoming connection, and if there is one we
 *  fill in `newconn' with our data pointer for it and the addresses,
 *  and return nonzero.  Otherwise return 0.
 */
static int poll_listen (NET_CONN *conn, NET_CONN *newconn)
{
	struct conn_data_t *data;
	char buffer[12], buffer2[8+NET_MAX_ADDRESS_LENGTH] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	char addr[NET_MAX_ADDRESS_LENGTH];
	int x;

	int count = 32; /* maximum number of packets to process */
	while (net_query (((struct conn_data_t *)conn->data)->chan) && count-- > 0) {
		if ((net_receive (((struct conn_data_t *)conn->data)->chan, buffer, 12, addr) == 12) && !memcmp (buffer, "connect", 8)) {
			newconn->data = data = malloc (sizeof *data);
			if (!data) continue;

			if (create_queues (newconn)) {
				free (data);
				continue;
			}

			data->conns = NULL;
			
			x = get_channel (
				((struct conn_data_t *)conn->data)->conns, addr,
				(buffer[8] << 24) + (buffer[9] << 16) +
				(buffer[10] << 8) + buffer[11],
				conn->type, NULL, &data->chan, data
			);
			
			if (x) {
				data->referer = conn->data;

				/* tell new channel where to send in future */
				net_assigntarget (data->chan, addr);

				/* send reply now with address of new channel, through
				 * listening conn so it can get through NATs */
				net_assigntarget (((struct conn_data_t *)conn->data)->chan, addr);
				strcpy (buffer2+8, net_getlocaladdress (data->chan));
				net_send (((struct conn_data_t *)conn->data)->chan, buffer2, 8+NET_MAX_ADDRESS_LENGTH);
			}
			
			if (x >= 0) {
				destroy_queues (newconn);
				free (data);
				continue;
			}

			strcpy (newconn->peer_addr, addr);

			return 1;
		}
	}

	return 0;
}


/* connect:
 *  Set the target address and send the first connection request.  This
 *  might not get through of course; later we can just repeat the send
 *  statement because the target address is already set.  No need to store
 *  it anywhere.
 */
static int connect (NET_CONN *conn, const char *target)
{
	int id;
	static int next_id = 0;
	struct conn_data_t *data = conn->data;

	/* The id is a compound of the current time (with 1 second 
	 * granularity), and an increasing counter.  The time is needed
	 * because the counter resets when you restart the program, and 
	 * the counter is needed because of the granularity of the time. */

	/* Strictly, this line needs a mutex, but there's nowhere around 
	 * here we can call `MUTEX_CREATE'. */
	id = (next_id++ << 16) + (time(NULL) & 0xffff);

	strcpy (data->connect_string, "connect");
	data->connect_string[8] = (id >> 24) & 0xff;
	data->connect_string[9] = (id >> 16) & 0xff;
	data->connect_string[10] = (id >> 8) & 0xff;
	data->connect_string[11] = id & 0xff;
	
	if (net_assigntarget (data->chan, target)) return 1;
	if (net_send (data->chan, data->connect_string, 12)) return 2;
	data->connect_timestamp = __libnet_timer_func();
	return 0;
}


/* poll_connect:
 *  This function does two things.  Firstly it checks for a response from 
 *  the server.  If there's no response, it then resends the connection 
 *  request.
 *
 *  The possible problem here is that the server's response might just be
 *  delayed.  The best way I can see around this problem is for the server
 *  to keep an eye on the return addresses of the connection attempts, and
 *  not open a fresh channel each time a duplicate of an old packet arrives.
 *  This actually kills two birds with one stone, since if either the 
 *  client's request packet or the server's response packet are dropped, the
 *  client will eventually resend, causing the server to send an identical 
 *  response.
 *
 *  Later note:  In fact we needed to introduce an almost-unique identifier
 *  to pass as well, since the channel's address may be reused later on.
 */
static int poll_connect (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;
	char buffer[8+NET_MAX_ADDRESS_LENGTH];
	char addr[NET_MAX_ADDRESS_LENGTH];
	
	if ((net_receive (data->chan, buffer, 8+NET_MAX_ADDRESS_LENGTH, addr) == 8+NET_MAX_ADDRESS_LENGTH) && (!memcmp (buffer, "\0\0\0\0\0\0\0", 8))) {
		net_fixupaddress_channel(data->chan, &buffer[8], addr);
		net_assigntarget (data->chan, addr);
		strcpy (conn->peer_addr, addr);
		return 1;
	}
	/* No response */
	{
		unsigned long clock_value = __libnet_timer_func();
		if ((unsigned)(clock_value - data->connect_timestamp) > RESEND_RATE) {
			net_send (data->chan, data->connect_string, 8);
			data->connect_timestamp = clock_value;
		}
	}
	return 0;
}


/* poll:
 *  This function handles all the true I/O for the RDMs.
 */
static void poll (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;
	
	/* First check whether anything in the outgoing queue needs sending */
	{
		struct out_packet_t *outp;
		int clock_value = __libnet_timer_func(), i;
		for (i = 0; i < MAX_OUTGOING_PACKETS; i++) {
			outp = data->out.packets + i;
			if ((outp->data) && (!outp->ack) && ((unsigned)(clock_value - outp->last_send_time) > RESEND_RATE)) {
				net_send (data->chan, outp->data, outp->size + 4);
				outp->last_send_time = clock_value;
			}
		}
	}
	
	/* Then receive any incoming data */
	{
		int count = 10;  /* max num of packets [we're not meant to block] */
		unsigned char receive_buffer[16384];
		
		while (count--) {
			int x;
			unsigned id;
			x = net_receive (data->chan, receive_buffer, sizeof receive_buffer, 0);
			if ((x == 0) || (x == -1)) break; /* quit if no more data or error */
			if (x < 4) continue;    /* badly formed packet -- no ID */
			id = 0;
			id = (id << 8) + receive_buffer[3];
			id = (id << 8) + receive_buffer[2];
			id = (id << 8) + receive_buffer[1];
			id = (id << 8) + receive_buffer[0];
			if (id == 0) {    /* it's an acknowledgement */
				if (x != 8) continue;   /* badly formed */
				id = (id << 8) + receive_buffer[7];
				id = (id << 8) + receive_buffer[6];
				id = (id << 8) + receive_buffer[5];
				id = (id << 8) + receive_buffer[4];
				if (id >= data->out.base_index)
					data->out.packets[id % MAX_OUTGOING_PACKETS].ack = 1;
				if (id == data->out.base_index) {
					int i, j = id % MAX_OUTGOING_PACKETS;
					for (i = 0; i < MAX_OUTGOING_PACKETS; i++) {
						if (!data->out.packets[j].ack || !data->out.packets[j].data) break;
						data->out.base_index++;
						free (data->out.packets[j].data);
						data->out.packets[j].data = NULL;
						j = (j + 1) % MAX_OUTGOING_PACKETS;
					}
				}
				continue;
			}
			
			if (x == 4) continue;          /* zero length */
			
			if (id >= data->in.base_index + MAX_INCOMING_PACKETS)
				continue;

			if (id >= data->in.base_index) {
				struct in_packet_t *ptr = data->in.packets + id % MAX_INCOMING_PACKETS;
				if (!ptr->data) {   /* haven't got this one yet */
					ptr->data = malloc (x - 4);
					if (!ptr->data) continue;
					memcpy (ptr->data, receive_buffer + 4, x - 4);
					ptr->size = x - 4;
				}
			}
			/* Acknowledge */
			{
				char buf[8] = { 0 };
				memcpy (buf + 4, receive_buffer, 4);
				net_send (data->chan, buf, 8);
			}
		}
	}
}

/* send_rdm:
 *  Sends an RDM down a conn.  Returns 0 on success, anything else on
 *  failure.  Could fail if the queue is full.
 */
static int send_rdm (NET_CONN *conn, const void *buf, int size)
{
	struct conn_data_t *data = conn->data;
	struct out_packet_t *outp;

	if ((size <= 0) || (data->out.next_index >= data->out.base_index + MAX_OUTGOING_PACKETS - 1)) {
		poll(conn); /* poll so that eventually the send queue will decrease */
		return -1;
	}
	
	outp = data->out.packets + (data->out.next_index % MAX_OUTGOING_PACKETS);
	outp->data = malloc (size + 4);
	if (!outp->data) return -1;
	outp->index = data->out.next_index++;
	outp->ack = 0;
	outp->last_send_time = 0;
	outp->size = size;
	
	((char *)outp->data)[0] =  outp->index        & 0xff;
	((char *)outp->data)[1] = (outp->index >> 8)  & 0xff;
	((char *)outp->data)[2] = (outp->index >> 16) & 0xff;
	((char *)outp->data)[3] = (outp->index >> 24) & 0xff;
	memcpy (((char *)outp->data) + 4, buf, size);
	
	poll (conn); /* dispatch the packet immediately */
	return 0;
}

/* recv_rdm:
 *  Tries to receive one RDM from the conn.  Return the number of bytes
 *  received, or negative on error.  -1 = buffer too small.
 */
static int recv_rdm (NET_CONN *conn, void *buf, int max)
{
	struct conn_data_t *data = conn->data;
	struct in_packet_t *packet;

	poll (conn);

	packet = data->in.packets + (data->in.base_index % MAX_INCOMING_PACKETS);

	if (!packet->data) return 0;
	if (packet->size > max) {
		memcpy (buf, packet->data, max);
		return -1;
	}
	memcpy (buf, packet->data, packet->size);
	free (packet->data);
	packet->data = NULL;
	data->in.base_index++;
	return packet->size;
}

/* ignore_rdm:
 *  Causes the next incoming packet (if any) to be dropped.  For use
 *  when, for whatever reason, a huge packet drifts into the queue
 *  and you aren't expecting to have to deal with it.  Returns 1 if
 *  a packet was ignored, zero otherwise.
 */
static int ignore_rdm (NET_CONN *conn)
{
        struct conn_data_t *data = conn->data;
        struct in_packet_t *packet;

        poll (conn);

        packet = data->in.packets + (data->in.base_index % MAX_INCOMING_PACKETS);

        if (!packet->data) return 0;
	free (packet->data);
        packet->data = NULL;
        data->in.base_index++;
        return 1;
}

/* query_rdm:
 *  Tests for incoming data on the conn.  Returns the amount of data in the
 *  first queued packet, or zero if none are queued.
 */
static int query_rdm (NET_CONN *conn)
{
	struct conn_data_t *data = conn->data;
	poll (conn);
	if (data->in.packets[data->in.base_index % MAX_INCOMING_PACKETS].data)
		return data->in.packets[data->in.base_index % MAX_INCOMING_PACKETS].size;
	else
		return 0;
}

/* conn_stats:
 *  Fills in the number of packets in the in and/or out queues.  Pass
 *  NULL if you don't care about one of them.
 */
static int conn_stats (NET_CONN *conn, int *in_q, int *out_q)
{
	struct conn_data_t *data = conn->data;
	poll (conn);
	
	if (in_q) {
		int i;
		*in_q = 0;
		for (i = 0; i < MAX_INCOMING_PACKETS; i++)
			if (data->in.packets[i].data) (*in_q)++;
	}
	if (out_q) *out_q = data->out.next_index - data->out.base_index;

	return 0;
}

/* load_conn_config:
 *  Loads conn config information from the given section.
 */
static void load_conn_config (NET_DRIVER *drv, FILE *fp, const char *section)
{
	char *option, *value;
	
	if (!drv->conn_config) {
		drv->conn_config = malloc (sizeof *drv->conn_config);
		if (!drv->conn_config) return;
		memcpy (drv->conn_config, &conn_config_default, sizeof conn_config_default);
	}

	if (__libnet_internal__seek_section (fp, section)) return;
	
	while (__libnet_internal__get_setting (fp, &option, &value) == 0) {
		if (!strcmp (option, "conn_outqueue_size")) {
			unsigned int x = atoi (value), y = 1;
			while (y && (y < x)) y <<= 1;
			if (!y) y = conn_config_default.outqueue_size;
			drv->conn_config->outqueue_size = y;
		} else if (!strcmp (option, "conn_inqueue_size")) {
			unsigned int x = atoi (value), y = 1;
			while (y && (y < x)) y <<= 1;
			if (!y) y = conn_config_default.inqueue_size;
			drv->conn_config->inqueue_size = y;
		} else if (!strcmp (option, "conn_resend_rate")) {
			unsigned int x = atoi (value);
			if (!x) x = conn_config_default.resend_rate;
			drv->conn_config->resend_rate = x;
		}
	}
}


NET_DRIVER __libnet_internal__wrapper_driver = {
	"Wrapper driver",
	NULL,                    /* desc */
	NET_CLASS_NONE,
	
	NULL, NULL, NULL,        /* detect, init, exit */
	
	NULL, NULL,              /* prepareaddress, poll_prepareaddress */
	
	NULL, NULL,              /* init_channel, destroy_channel */

	NULL, NULL, NULL, NULL,  /* update_target, send, recv, query */
	
	init_conn, destroy_conn,
	listen, poll_listen, connect, poll_connect,
	
	send_rdm,
	recv_rdm,
	query_rdm,
	ignore_rdm,
	conn_stats,

	NULL,                    /* load_config */
	load_conn_config,
	NULL
};

