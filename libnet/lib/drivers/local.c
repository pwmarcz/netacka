/*----------------------------------------------------------------
 * local.c - Local Host driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "libnet.h"
#include "internal.h"
#include "config.h"
#include "threads.h"
#include "platdefs.h"


#define MAX_PACKET		(16+1)
#define MAX_PACKET_SIZE		512


typedef struct packet {
    int from;
    int size;
    char data[MAX_PACKET_SIZE];
} packet_t;

typedef struct port {
    int num;
    int users;
    packet_t packets[MAX_PACKET];
    int head, tail;
    struct port *next;    
} port_t;

typedef struct channel_data {
    port_t *port;
    int target;		/* may get destroyed so we can't just point to it */
} channel_data_t;


static port_t *port_list;
MUTEX_DEFINE_STATIC(port_list);

static int disable_driver = 0;



/*
 * Port list helpers
 */

static port_t *find_port(int num)
{
    port_t *p = port_list;
    while (p) {
	if (p->num == num)
	    return p;
	p = p->next;
    }
    return NULL;
}


static int get_free_port(void)
{
    static int i = 1;
    while (find_port(i)) i++;
    return i;
}


static port_t *create_port(int num)
{
    port_t *p = find_port(num);

    if (!p) {
	    p = malloc(sizeof *p);
	    if (p) {
		p->num = num;
		p->users = 1;
		p->head = p->tail = 0;
		p->next = port_list;
		port_list = p;
	    }
    }
    else
	p->users++;

    return p;
}


static void destroy_port(port_t *match)
{
    port_t *p = port_list, *prev = NULL;
        
    while (p) {
	if (p == match) {
	    p->users--;
	    if (!p->users) {
		if (!prev)
		    port_list = p->next;
		else
		    prev->next = p->next;
	    
		free(p);
	    }
	    return;
	}
	
	prev = p;
	p = p->next;
    }
}



/* detect:
 *  This function returns one of the following:
 *
 *  - NET_DETECT_YES    (if the network driver can definitely work)
 *  - NET_DETECT_MAYBE  (if the network driver might be able to work)
 *  - NET_DETECT_NO     (if the network driver definitely cannot work)
 */
static int local_detect(void)
{
    return (!disable_driver) ? NET_DETECT_YES : NET_DETECT_NO;
}



/* init:
 *  Initialises the driver.  Return zero if successful, non-zero if not.
 */
static int local_init(void)
{
    port_list = NULL;
    MUTEX_CREATE(port_list);
    return 0;
}



/* exit:
 *  Shuts down the driver, freeing allocated resources, etc.  All channels
 *  will already have been destroyed by the `destroy_channel' function, so 
 *  normally all this needs to do is tidy up anything `init' left untidy.
 *  Return zero if successful, non-zero otherwise.
 */
static int local_exit(void)
{
    MUTEX_LOCK(port_list);
    while (port_list)
	destroy_port(port_list);
    MUTEX_UNLOCK(port_list);
    MUTEX_DESTROY(port_list);
    return 0;
}



static int parse_address(const char *addr)
{
    if (!addr)
        return get_free_port();	/* any */

    if (*addr == 0) 
	return 0;		/* default */

    /* else, [:]port is user specified */
    
    if (*addr == ':')	
	addr++;
    
    if (!isdigit((unsigned char)*addr))
	return -1;
    else
	return atoi(addr);
}



/* init_channel:
 *  This function will be called when a channel is created, before any 
 *  data is sent.  It should at least set the channel's return address to
 *  something meaningful or at least to a valid string if the driver can't
 *  supply a return address.  It can allocate memory, pointed to by the
 *  `data' field of the channel struct.  Return zero if successful, non-zero
 *  otherwise.
 */
static int local_init_channel(NET_CHANNEL *chan, const char *addr)
{
    channel_data_t *data;
    port_t *port;
    int num;

    MUTEX_LOCK(port_list);

    num = parse_address(addr);
    if (num < 0) {
	MUTEX_UNLOCK(port_list);
	return -1;
    }
    
    port = create_port(num);

    /* We don't need the lock any more because `create_port' incremented
     * the usage counter on the port, so it won't vanish, and we're not 
     * dereferencing the pointer any more. */
    MUTEX_UNLOCK(port_list);

    if (!port)
        return -1;
    
    sprintf(chan->local_addr, "%d", num);

    chan->data = data = malloc(sizeof *data);
    data->port = port;
    data->target = -1;
    
    return 0;
}



/* destroy_channel:
 *  Called when a channel is being closed.  This should tidy up anything 
 *  that init_channel or subsequent functions have left messy, for example 
 *  it should free any data block that has been allocated.  Return zero if 
 *  successful, non-zero otherwise.
 */
static int local_destroy_channel(NET_CHANNEL *chan)
{
    channel_data_t *data = chan->data;

    MUTEX_LOCK(port_list);
    destroy_port(data->port);
    MUTEX_UNLOCK (port_list);

    free(data);

    return 0;
}



/* update_target:
 *  This will be called if the target_addr for the channel changes. 
 *  If necessary, the address could be converted into network format here.
 *  Return zero if any conversion was possible, but don't bother checking
 *  whether or not the target is reachable.
 */
static int local_update_target(NET_CHANNEL *chan)
{
    char *addr;
    int target;

    addr = chan->target_addr;
    if (!addr) return -1;
    
    MUTEX_LOCK(port_list);
    target = parse_address(addr);
    MUTEX_UNLOCK(port_list);

    if (target < 0) return -1;
    ((channel_data_t *)chan->data)->target = target;

    return 0;
}



static inline int next_packet(int n)
{
    if (n+1 < MAX_PACKET)
	return n+1;
    else
	return 0;
}


/* send:
 *  Send the given data block down the channel.  `size' is in bytes.
 *  Return zero if successful, otherwise return non-zero.  Don't bother
 *  checking that the packet arrived at the destination though.
 */
static int local_send(NET_CHANNEL *chan, const void *buf, int size)
{
    channel_data_t *data;
    port_t *target;
    packet_t *packet;
    
    if (size > MAX_PACKET_SIZE)
	return -1;
    
    /* Strictly, we should be locking the port's queue, but this will do. */
    MUTEX_LOCK(port_list);

    data = chan->data;
    target = find_port(data->target);
    
    if (target && (next_packet(target->head) != target->tail)) {
	target->head = next_packet(target->head);
    
	packet = &target->packets[target->head];
	packet->from = data->port->num;
	packet->size = size;
	memcpy(packet->data, buf, size);
    }

    /* Errors and successes all end up here, because no delivery
     * checking occurs on channels. */

    MUTEX_UNLOCK(port_list);
    
    return 0;
}



/* recv:
 *  Receive a packet into the memory block given.  `size' is the maximum
 *  number of bytes to receive.  The sender's address will be put in `from'.
 *  Return the number of bytes received, or -1 if an error occured.  A zero 
 *  return value means there was no data to receive.
 *
 *  Behaviour if more that `size' bytes are waiting is undecided -- either
 *  return an error, or read `size' bytes and return `size'.  The remainder
 *  of the packet should probably be discarded, in either case.  For now,
 *  it's the user's fault for not making the buffer big enough.
 */
static int local_recv(NET_CHANNEL *chan, void *buf, int size, char *from)
{
    port_t *port;
    packet_t *packet;

    port = ((channel_data_t *)chan->data)->port;

    /* Strictly we should be locking the port's queue, but this will do.
     * We get the lock this early in case another channel is sharing the
     * port.  (I'm particularly concerned about overlapping `recv' calls
     * here.) */
    MUTEX_LOCK(port_list);

    if (port->tail == port->head) {
	MUTEX_UNLOCK(port_list);
	return -1;
    }

    port->tail = next_packet(port->tail);
    packet = &port->packets[port->tail];
        
    if (packet->size < size)
	size = packet->size;
    memcpy(buf, packet->data, size);

    if (from)
	sprintf(from, "%d", packet->from);
    
    MUTEX_UNLOCK(port_list);

    return size;
}



/* query:
 *  Returns non-zero if there is data waiting to be read from the channel.
 */
static int local_query(NET_CHANNEL *chan)
{
    port_t *port = ((channel_data_t *)chan->data)->port;
    /* Again, strictly we should lock the port queue, not the whole list. */
    MUTEX_LOCK(port_list);
    if (port->head != port->tail) {
	MUTEX_UNLOCK(port_list);
	return 1;
    } else {
	MUTEX_UNLOCK(port_list);
	return 0;
    }
}



/* load_config:
 *  This will be called once when the library is initialised, inviting the
 *  driver to load configuration information from the passed text file.
 */
static void local_load_config(NET_DRIVER *drv, FILE *fp)
{
    char *option, *value;

    if (__libnet_internal__seek_section(fp, "localhost"))
	return;

    while (__libnet_internal__get_setting(fp, &option, &value) == 0) 
	if (strcmp(option, "disable") == 0)
	    disable_driver = (atoi(value) || (value[0] == 'y'));
    
    if (drv->load_conn_config)
	drv->load_conn_config(drv, fp, "localhost");
}



NET_DRIVER net_driver_local = {
    "Local Host",
    "Local Host driver",
    NET_CLASS_LOCAL,

    local_detect,
    local_init,
    local_exit,

	NULL, NULL,

    local_init_channel,
    local_destroy_channel,

    local_update_target,
    local_send,
    local_recv,
    local_query,

    NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,

    local_load_config,
    NULL, NULL
};
