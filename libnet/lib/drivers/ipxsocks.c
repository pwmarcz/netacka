/*----------------------------------------------------------------
 * ipxsocks.c - Sockets-based IPX driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If this platform supports this driver, use it. */
#if defined __USE_REAL_IPX_LINUX__ || defined __USE_REAL_IPX_WIN__

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __USE_REAL_IPX_LINUX__
#include <netipx/ipx.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define SOCKET int
#define INVALID_SOCKET ((SOCKET)-1)
#define SOCKET_ERROR (-1)
#define ioctlsocket ioctl
#define closesocket close
#endif

#ifdef __USE_REAL_IPX_WIN__
#include <winsock.h>
#include <wsipx.h>

#define sipx_network sa_netnum
#define sipx_node sa_nodenum
#define sipx_family sa_family
#define sipx_port sa_socket
#endif


/*---------------------------------------- IPX address types */
typedef unsigned char ipx_node[6];
typedef unsigned char ipx_net[4];
typedef struct {
	ipx_node node;
	ipx_net net;
} ipx_addr;


/*---------------------------------------- libnet interface */
#include "libnet.h"
#include "internal.h"
#include "config.h"

static SOCKET ipx_socket;
static unsigned char ipx_buffer[NET_MAX_PACKET_SIZE + 8];

/* General default settings */
static int default_socket = 24785;    /* default IPX socket number */

static int default_port = 1;
static int forceport = 0;             /* should we reuse non-vacant ports? */

static ipx_addr local_addr;

static int disable_driver = 0;

#define MAX_PACKETS  16   /* Max number of packets queueable on a port */


/*---------------------------------------- Data structures */

/* Internal data for channels */
struct channel_data_t {
	int port;
	char target_port[4];
	char source_port[4];
	struct sockaddr_ipx target_address;
	int address_length;
};

/* Packet */
struct packet_t {
	int size;
	ipx_addr from;
	int fromport;
	unsigned char data[NET_MAX_PACKET_SIZE];
};

/* Linked list of port packet queues */
struct port_queue_t {
	int port;
	struct port_queue_t *next;
	int usage;
	struct packet_t packet[MAX_PACKETS];
	int first, count;
};

static struct port_queue_t *port_queues;


/*---------------------------------------- Utility functions */

static inline void write_32 (unsigned char *p, unsigned int x)
{
	p[0] = x >> 24;
	p[1] = x >> 16;
	p[2] = x >> 8;
	p[3] = x;
}

static inline int read_32 (unsigned char *p)
{
	return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}

/*---------------------------------------- Port queue control */

/* Find the given port's queue struct, or return NULL */
static struct port_queue_t *find_port (int id)
{
	struct port_queue_t *port = port_queues;
	while (port && port->port != id) port = port->next;
	return port;
} 

/* Get the number of a free port (not the default port) */
static int find_free_port (void)
{
	static int next_port;
	while ((next_port == 0) || (next_port == default_port) || find_port (next_port)) next_port++;
	return next_port;
}

/* Create a queue for the given port, if it doesn't already exist;
 * if it does exist, return failure if `force' is zero. */
static int create_port (int id, int force)
{
	struct port_queue_t *port = find_port (id);
	if (port) {
		if (!force) return -1;
		port->usage++;
		return 0;
	}
	port = malloc (sizeof *port);
	port->port = id;
	port->usage = 1;
	port->first = port->count = 0;
	port->next = port_queues;
	port_queues = port;
	return 0;
}

/* Free up the given port.  We have a usage count in case more than one
 * channel is using the same port. */
static void free_port (int id)
{
	struct port_queue_t *port = find_port (id), **prev;
	if (!port) return;
	if (--port->usage) return;
	for (prev = &port_queues; *prev && *prev != port; prev = &(*prev)->next);
	if (*prev) *prev = port->next;
	free (port);
}

/* Poll the IPX socket and distribute the packets between the ports */
static void poll_ipx_socket (void)
{
	fd_set rfds;
	struct timeval tv;
	int size, port_id;
	struct port_queue_t *port;
	struct packet_t *pack;
	struct sockaddr_ipx from_addr;
	int addrsize;
	
	do {	
		FD_ZERO (&rfds);
		FD_SET (ipx_socket, &rfds);
		tv.tv_sec = tv.tv_usec = 0;
		
		if (!select (ipx_socket+1, &rfds, NULL, NULL, &tv)) break;

		addrsize = sizeof from_addr;		
		size = recvfrom (ipx_socket, ipx_buffer, sizeof ipx_buffer, 0, (struct sockaddr *)&from_addr, &addrsize);
		if (size < 0) break;
		
		port_id = read_32 (ipx_buffer);
		port = find_port (port_id);
		if (!port) continue;
		if (port->count == MAX_PACKETS) continue;

		pack = port->packet + ((port->first + port->count) % MAX_PACKETS);
		pack->fromport = read_32 (ipx_buffer + 4);
		memcpy (pack->from.net, &from_addr.sipx_network, 4);
		memcpy (pack->from.node, from_addr.sipx_node, 6);
		memcpy (pack->data, ipx_buffer + 8, size - 8);
		pack->size = size - 8;

		port->count++;
	} while (1);
}


/*---------------------------------------- Address utility functions */

static void write_address (char *s, ipx_addr *addr, int port)
{
	sprintf (s, "%02X%02X%02X%02X/%02X%02X%02X%02X%02X%02X:%d",
		addr->net[0], addr->net[1], addr->net[2], addr->net[3],
		addr->node[0], addr->node[1], addr->node[2],
		addr->node[3], addr->node[4], addr->node[5],
		port);
}

/* helper to read a pair of hex digits */
static int read_hex_pair (const char **s)
{
	static char *digits = "0123456789ABCDEF";
	char *p1, *p2;
	p1 = strchr (digits, toupper(**s));
	if (!p1 || !*p1) return -1;
	(*s)++;
	p2 = strchr (digits, toupper(**s));
	if (!p2 || !*p2) return -1;
	(*s)++;
	return ((p1 - digits) << 4) + (p2 - digits);
}

/* read the network portion of an address */
static int read_net_addr (const char *s, const char *end, ipx_addr *addr)
{
	int i,j;

	if (*s == '*' && end - s == 1) {
		memset (addr->net, 0xff, 4);
		return 0;
	}

	if (end - s != 8) return 1;

	for (i = 0; i < 4; i++) {
		j = read_hex_pair (&s);
		if (j == -1) return 1;
		addr->net[i] = j;
	}
	return 0;
}

/* read the node portion of an address */
static int read_node_addr (const char *s, const char *end, ipx_addr *addr)
{
	int i,j;
	
	if (*s == '*' && end - s == 1) {
		memset (addr->node, 0xff, 6);
		return 0;
	}
	
	if (end - s != 12) return 1;

	for (i = 0; i < 6; i++) {
		j = read_hex_pair (&s);
		if (j == -1) return 1;
		addr->node[i] = j;
	}
	return 0;
}

/* parse one of the following: (net = 8 hex digits, node = 12 hex, port = dec)
 *    net/node:port, net/node, node:port, node
 */
static int parse_address (const char *s, ipx_addr *addr, int *port)
{
	char *colon = strchr (s, ':');

	if (port && colon) *port = atoi (colon+1);
	if (!colon) colon = strchr (s, '\0');
		
	if (addr) {
		char *slash = strchr (s, '/');
		if (slash) {
			if (read_net_addr (s, slash, addr)) return 1;
			s = slash+1;
		} else {
			memset (addr->net, 0, 4);
		}
		if (read_node_addr (s, colon, addr)) return 1;
	}

	return 0;
}

/*---------------------------------------- General driver functions */

static int detect (void)
{
	if (disable_driver) return NET_DETECT_NO;
	{
#ifdef __USE_REAL_WSOCK_WIN__
		unsigned short wVersionRequested = MAKEWORD (1, 1);
		WSADATA wsaData;
		int err;

		err = WSAStartup (wVersionRequested, &wsaData);
		if (err) return NET_DETECT_NO;

		WSACleanup();
#endif
		return NET_DETECT_YES;
	}
}

static int init (void)
{
	int one;
	struct sockaddr_ipx addr;
	int addrlength;

#ifdef __USE_REAL_WSOCK_WIN__
	short wVersionRequested = MAKEWORD (1, 1);
	WSADATA wsaData;
	unsigned int err;

	err = WSAStartup (wVersionRequested, &wsaData);
	if (err) return 1;
#endif

	ipx_socket = socket (AF_IPX, SOCK_DGRAM, 0);
	if (ipx_socket == INVALID_SOCKET) return 1;

	one = 1;
	if (ioctlsocket (ipx_socket, FIONBIO, (u_long *)&one)) {
		closesocket (ipx_socket);
		return 2;
	}

	addr.sipx_family = AF_IPX;
	memcpy (&addr.sipx_network, local_addr.net, 4);
	memcpy (addr.sipx_node, local_addr.node, 6);
	addr.sipx_port = default_socket;
	if (bind (ipx_socket, (struct sockaddr *)&addr, sizeof addr)) {
		closesocket (ipx_socket);
		return 3;
	}

	addrlength = sizeof addr;
	getsockname (ipx_socket, (struct sockaddr *)&addr, &addrlength);
	memcpy (local_addr.net, &addr.sipx_network, 4);
	memcpy (local_addr.node, addr.sipx_node, 6);

	return 0;
}

static int drv_exit (void)
{
	closesocket (ipx_socket);
#ifdef __USE_REAL_WSOCK_WIN__
	WSACleanup();
#endif
	return 0;
}


/*---------------------------------------- Channel functions */

static int init_channel (NET_CHANNEL *chan, const char *addr)
{
	struct channel_data_t *data;
	int f = forceport, port = 0;
	ipx_addr address = { { 0 }, { 0 } };

	if (addr) {
		port = default_port;
		switch (*addr) {
			case '+': f = 1; addr++; break;
			case '-': f = 0; addr++; break;
		}
		if (*addr) parse_address (addr, &address, &port);
	}

	data = (struct channel_data_t *) malloc (sizeof (struct channel_data_t));
	if (!data) return 1;
	chan->data = data;
	
	data->port = port ? port : find_free_port();
	if (create_port (data->port, f)) {
		free (data);
		return 2;
	}
	
	write_32 (data->source_port, data->port);

	data->target_address.sipx_family = AF_IPX;
	data->target_address.sipx_port = default_socket;
	data->address_length = sizeof data->target_address;
	
	write_address (chan->local_addr, &local_addr, data->port);
	return 0;
}

static int update_target (NET_CHANNEL *chan)
{
	struct channel_data_t *data = chan->data;
	int port;
	ipx_addr addr;
	
	if (parse_address (chan->target_addr, &addr, &port)) return 1;
	if (!port) port = default_port;

	data->target_address.sipx_family = AF_IPX;
	memcpy (&data->target_address.sipx_network, &addr.net, 4);
	memcpy (&data->target_address.sipx_node, &addr.node, 6);

	write_32 (data->target_port, port);

	return 0;
}

static int destroy_channel (NET_CHANNEL *chan)
{
	struct channel_data_t *data = chan->data;
	free_port (data->port);
	free (data);
	return 0;
}

static int drv_send (NET_CHANNEL *chan, const void *buf, int size)
{
	struct channel_data_t *data = chan->data;
	int x;

	if (size > NET_MAX_PACKET_SIZE) return 1;

	/* Unfortunately we need to copy these packets, to prepend the port 
	 * numbers. :( */
	memcpy (ipx_buffer, data->target_port, 4);
	memcpy (ipx_buffer + 4, data->source_port, 4);
	memcpy (ipx_buffer + 8, buf, size);

	x = sendto (ipx_socket, ipx_buffer, size+8, 0, (struct sockaddr *)&data->target_address, data->address_length);
	return (x == SOCKET_ERROR);
}

static int drv_recv (NET_CHANNEL *chan, void *buf, int size, char *from)
{
	struct channel_data_t *data = chan->data;
	struct port_queue_t *port = find_port (data->port);
	struct packet_t *pack;
	
	poll_ipx_socket();
	
	if (port->count) {
		pack = &port->packet[port->first];
		if (size > pack->size) size = pack->size;
		memcpy (buf, pack->data, size);
		if (from) write_address (from, &pack->from, pack->fromport);
		port->first = (port->first + 1) % MAX_PACKETS;
		port->count--;
		return size;
	}

	return -1;
}

static int query (NET_CHANNEL *chan)
{
	struct channel_data_t *data = chan->data;
	struct port_queue_t *port = find_port (data->port);
	
	poll_ipx_socket();
	
	return !!port->count;
}


static void load_config (NET_DRIVER *drv, FILE *fp)
{
	char *option, *value;
	
	if (!__libnet_internal__seek_section (fp, "ipx")) {
	
		while (!__libnet_internal__get_setting (fp, &option, &value)) {
		
			if (!strcmp (option, "port")) {
     
				int x = atoi (value);
				if (x) default_port = x;

			} else if (!strcmp (option, "socket")) {

				int x = atoi (value);
				if (x) default_socket = x;

			} else if (!strcmp (option, "forceport")) {

				forceport = atoi (value);
        
			} else if (!strcmp (option, "address")) {
        
				parse_address (value, &local_addr, NULL);
        
			} else if (!strcmp (option, "disable")) {
				disable_driver = (atoi (value) || (value[0] == 'y'));
			}
  
		}
	}

#ifdef __USE_REAL_IPX_LINUX__
	if (!__libnet_internal__seek_section (fp, "ipx_linux")) {
		while (!__libnet_internal__get_setting (fp, &option, &value)) {
			if (!strcmp (option, "disable")) {
				disable_driver = (atoi (value) || (value[0] == 'y'));
			}
		}
	}
#endif
#ifdef __USE_REAL_IPX_WIN__
	if (!__libnet_internal__seek_section (fp, "ipx_win")) {
		while (!__libnet_internal__get_setting (fp, &option, &value)) {
			if (!strcmp (option, "disable")) {
				disable_driver = (atoi (value) || (value[0] == 'y'));
			}
		}
	}
#endif

	if (drv->load_conn_config) drv->load_conn_config (drv, fp, "ipx");
}


#endif  /* __USE_REAL_IPX_LINUX__ || __USE_REAL_IPX_WIN__ */


/* Prepare for dummy drivers */

#include <stdio.h>
#include <stdlib.h>

#include "libnet.h"
#include "internal.h"

static int dummy_detect (void) { return NET_DETECT_NO; }
static void dummy_load_config (NET_DRIVER *drv, FILE *fp) { }

#define MAKE_REAL_IPX_DRIVER(drv,name) \
	NET_DRIVER drv = { \
		name, \
		"IPX driver", \
		NET_CLASS_IPX, \
		\
		detect, \
		init, \
		drv_exit, \
		\
		NULL, NULL, \
		\
		init_channel, \
		destroy_channel, \
		\
		update_target, \
		drv_send, \
		drv_recv, \
		query, \
		\
		NULL, NULL, \
		NULL, NULL, NULL, NULL, \
		NULL, NULL, NULL, NULL, NULL, \
		\
		load_config, \
		NULL, NULL \
	}

#define MAKE_DUMMY_IPX_DRIVER(drv,name) \
	NET_DRIVER drv = { \
		name, "IPX driver", \
		NET_CLASS_IPX, \
		dummy_detect, \
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
		NULL, NULL, NULL, NULL, NULL, NULL, \
		NULL, NULL, NULL, NULL, NULL, \
		dummy_load_config, \
		NULL, NULL \
	}



#ifdef __USE_REAL_IPX_LINUX__
	MAKE_REAL_IPX_DRIVER  (net_driver_ipx_linux, "IPX in Linux");
#else
	MAKE_DUMMY_IPX_DRIVER (net_driver_ipx_linux, "IPX in Linux");
#endif


#ifdef __USE_REAL_IPX_WIN__
	MAKE_REAL_IPX_DRIVER  (net_driver_ipx_win, "IPX in Windows");
#else
	MAKE_DUMMY_IPX_DRIVER (net_driver_ipx_win, "IPX in Windows");
#endif

