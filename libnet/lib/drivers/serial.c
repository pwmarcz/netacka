/*----------------------------------------------------------------
 * serial.c - serial network driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"
#include "libnet.h"
#include "internal.h"


/* If we can use serial ports, do so.  */
#ifdef __USE_REAL_SERIAL__

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "config.h"
#include "serial.h"


/* Note: In the serial code we distinguish between physical serial 
 * "ports" and virtual "slots".  Normally the words "address" and 
 * "port" apply, but it's less confusing for me this way.  */


#define MAX_PORT	8


/* High level serial ports.  These are a layer above the platform
 * specific low level serial port routines and are shared between
 * channels.  */

struct hl_port {
    struct hl_port *next;
    struct hl_port *prev;
    int port_nr;
    struct port *port;
    struct queue recv;
};

static struct list_head hl_port_list;


/* Private channel data.  */

#define ALL	-1
 
struct channel_data {
    struct channel_data *next;
    struct channel_data *prev;
    int port_nr;		/* port we use */
    int local_slot;		/* slot we receive from */
    int remote_slot;		/* slot we send to */
    struct list_head packet_list;
};

static struct list_head channel_data_list;


/* Packets.  These are like letters sitting in a mailbox (the channel) 
 * waiting to be collected.  Slots are then the names of recipients on
 * the envelope.  */
struct packet {
    struct packet *next;
    struct packet *prev;
    int from_port_nr;
    int from_slot;
    int size;
    void *data;
};


/* Config variables.  */
static int autoports[MAX_PORT] = { 0, 1 };
static int default_slot = 0;
static int disable_driver = 0;


/* Find a slot unused by any channel and is not the default.  */
static int unused_slot (void)
{
    static int next_slot = 0;
    struct channel_data *data;

  next:

    next_slot = (next_slot + 1) & 0xffff;

    if (next_slot == default_slot)
	goto next;

    foreach (data, channel_data_list)
	if (next_slot == data->local_slot)
	    goto next;

    return next_slot;
}


/* Return the port and slot numbers in a binding string.  */

#define ERR	-0xff

static int my_atoi (const char *s)
{
    return (isdigit (*s)) ? atoi (s) : ERR;
}

static int decode_binding (const char *binding, int *port_nr, int *slot)
{
    if (!binding) {			/* NULL */
	*port_nr = ALL;
	*slot = unused_slot ();
    }
    else if (!*binding) {		/* "" */
	*port_nr = ALL;
	*slot = default_slot;
    }
    else {
	char *c = strchr (binding, ':');
	if (c) {
	    if (c != binding)		/* x:[y] */
		*port_nr = my_atoi (binding);
	    else			/* :[y] */
		*port_nr = ALL;
	    
	    if (c[1])			/* [x]:y */
		*slot = my_atoi (c+1);
	    else			/* [x]: */
		*slot = default_slot;
	}
	else {				/* x */
	    *port_nr = my_atoi (binding);
	    *slot = default_slot;
	}
    }

    return ((*port_nr == ERR) || (*slot == ERR)) ? -1 : 0;
}


/* (see top of serial_send for packet format)  */

#define PACKET_MARKER	0x94A3FE05l
#define PACKET_SIZE	(4+2+2+2+2+4)

/* Helpers for calculating the checksums.  */

#define DATA_RLS_INIT	0x6FD3

static int rls_checksum (int remote, int local, int size)
{
    int chk = DATA_RLS_INIT;
    chk ^= ((chk << 11) | (chk >> 5)) ^ remote; chk &= 0xFFFFl;
    chk ^= ((chk << 11) | (chk >> 5)) ^ local;  chk &= 0xFFFFl;
    chk ^= ((chk << 11) | (chk >> 5)) ^ size;   chk &= 0xFFFFl;
    return chk;
}

#define DATA_CHK_INIT	0x1F25B79Cl

static int data_checksum_ex (int chk, const unsigned char *data, int size)
{
    while (size--)
        chk ^= ((chk << 19) | (chk >> 13)) ^ *data++;
    return chk;
}

static int data_checksum_q (struct queue *q, int p, int size)
{
    int chk = DATA_CHK_INIT;
    int p_end = queue_wrap (p + size);

    if (p_end < p) {
        int br = QUEUE_SIZE - p;
	chk = data_checksum_ex (chk, &q->data[p], br);
        chk = data_checksum_ex (chk, q->data, p);
    }
    else {
	chk = data_checksum_ex (chk, &q->data[p], size);
    }

    return chk;
}

#define data_checksum_l(data, size) \
        data_checksum_ex(DATA_CHK_INIT, (data), (size))


/* Retrieve the next packet from the bytestream.  */
static struct packet *get_next_packet (struct hl_port *hl_port, int *to_slot)
{
    struct queue *q = &hl_port->recv;
    struct packet *packet;

    int qtail = q->tail;
    int qsize = queue_wrap (q->head - qtail);

    int marker;

    if (qsize < PACKET_SIZE) return 0;

    /* Read the first int from the queue */
    marker                = q->data[qtail]; qtail = queue_wrap (qtail + 1);
    marker <<= 8; marker |= q->data[qtail]; qtail = queue_wrap (qtail + 1);
    marker <<= 8; marker |= q->data[qtail]; qtail = queue_wrap (qtail + 1);
    marker <<= 8; marker |= q->data[qtail]; qtail = queue_wrap (qtail + 1);

    /* qtail now points to the first byte after the hypothetical
     * PACKET_MARKER.  */

    while (qsize >= PACKET_SIZE) {

        if (marker == PACKET_MARKER) {

 	    int p = qtail;

	    int remote;
	    int local;
	    int size;
	    int n;

            remote                = q->data[p]; p = queue_wrap (p + 1);
            remote <<= 8; remote |= q->data[p]; p = queue_wrap (p + 1);

            local                 = q->data[p]; p = queue_wrap (p + 1);
            local  <<= 8; local  |= q->data[p]; p = queue_wrap (p + 1);

            size                  = q->data[p]; p = queue_wrap (p + 1);
            size   <<= 8; size   |= q->data[p]; p = queue_wrap (p + 1);

            /* Read checksum for packet header.  */
            n                     = q->data[p]; p = queue_wrap (p + 1);
            n      <<= 8; n      |= q->data[p]; p = queue_wrap (p + 1);

            /* Is the checksum correct?  */
            if (n == rls_checksum (remote, local, size)) {

		/* If the packet isn't all there yet, then we return.
                 * The packet will be read later when it is complete.  */
		if (qsize < PACKET_SIZE + size) {
                    q->tail = queue_wrap (qtail - 4);
		    return 0;
                }

                /* Skip the header permanently. It is valid.  */
                qtail = p;

                /* Read data checksum.  */
		p = queue_wrap (p + size);

	        n           = q->data[p]; p = queue_wrap (p + 1);
	        n <<= 8; n |= q->data[p]; p = queue_wrap (p + 1);
	        n <<= 8; n |= q->data[p]; p = queue_wrap (p + 1);
	        n <<= 8; n |= q->data[p]; p = queue_wrap (p + 1);

                /* Is the checksum correct?  */
                if (n == data_checksum_q (q, qtail, size)) {

		    packet = malloc (sizeof *packet);

                    /* Out of memory? Tut tut. Discard the packet.  */

                    if (packet) {
		        packet->size = size;
		        packet->data = malloc (size);

		        /* Store header information.  */
                        *to_slot = remote;
		        packet->from_slot = local;
		        packet->from_port_nr = hl_port->port_nr;
                    }

		    /* p currently points to after the data checksum.
  		     * Set q->tail accordinly, and set p to point to
  		     * the end of the data themselves.  */
		    q->tail = p;
		    p = queue_wrap (p - 4);

		    /* Read packet data.  */
                    if (packet) {
                        if (p < qtail) {
                            int br = QUEUE_SIZE - qtail;
			    memcpy (packet->data, &q->data[qtail], br);
			    memcpy (packet->data + br, q->data, p);
                        } else {
                            memcpy (packet->data, &q->data[qtail], p - qtail);
                        }
                    }

                    return packet;
                }
            }
        }

        /* Skip unidentified data.  */
        marker <<= 8;
        marker &= 0xFFFFFFFFl; /* Portable to 64-bit systems, NARF  */
        marker |= q->data[qtail];
        qtail = queue_wrap (qtail + 1);
        qsize--;
    }

    /* No complete packet was present.  */
    return 0;
}


/* Free a packet.  */
static void free_packet (struct packet *p)
{
    free (p->data);
    free (p);
}


/* Check ports for new data.  If packets are available, fish them out
 * and distribute them to the appropriate channels.  */
static void poll_ports (void)
{
    struct hl_port *hl_port;
    struct packet *packet;
    int to_slot;

    foreach (hl_port, hl_port_list) {
	unsigned char buf[256];
	int size, i;

	/* Read byte stream from low level port.  */
	size = __libnet_internal__serial_read (hl_port->port, buf, sizeof buf);
	if (size <= 0) continue;

	/* Stuff byte stream into queue.  */
	for (i = 0; i < size; i++)
	    queue_put (hl_port->recv, buf[i]);

      next:

	/* Get the next packet if available.  */
	if ((packet = get_next_packet (hl_port, &to_slot))) {

	    struct channel_data *data;

	    /* Distribute it to channel.  */
	    foreach (data, channel_data_list)
		if (data->local_slot == to_slot) {
		    append_to_list (data->packet_list, packet);
		    goto next;
		}

	    /* No takers.  */
	    free_packet (packet);
	}
    }
}


/* Return non-zero if port is not already opened.  */
static int port_unopened (int port_nr)
{
    struct hl_port *hl_port;
    if ((port_nr == ALL) || (autoports[port_nr]))
	return 0;
    foreach (hl_port, hl_port_list)
	if (hl_port->port_nr == port_nr) return 0;
    return 1;
}


/* Open a port and add it to the list.  Return zero on success.  */
static int open_port (int port_nr)
{
    struct hl_port *hl_port;
    struct port *port;
    
    hl_port = malloc (sizeof *hl_port);
    if (!hl_port) return -1;

    port = __libnet_internal__serial_open (port_nr);
    if (!port) {
	free (hl_port);
	return -1;
    }

    hl_port->port_nr = port_nr;
    hl_port->port = port;
    hl_port->recv.head = hl_port->recv.tail = 0;
    add_to_list (hl_port_list, hl_port);

    return 0;
}


/* Close a port and free it.  */
static void free_port (struct hl_port *hl_port)
{
    __libnet_internal__serial_close (hl_port->port);
    free (hl_port);
}


/* serial_detect:
 *  This function returns one of the following:
 *  - NET_DETECT_YES    (if the network driver can definitely work)
 *  - NET_DETECT_MAYBE  (if the network driver might be able to work)
 *  - NET_DETECT_NO     (if the network driver definitely cannot work)
 */
static int serial_detect (void)
{
    return (disable_driver) ? NET_DETECT_NO : NET_DETECT_MAYBE;
}


/* serial_init:
 *  Initialises the driver.  Return zero if successful, non-zero if not.
 */
static int serial_init (void)
{
    int i;

    init_list (hl_port_list);
    init_list (channel_data_list);

    if (__libnet_internal__serial_init ())
	return -1;

    for (i = 0; i < MAX_PORT; i++)  
	if (autoports[i]) open_port (i);
    
    return 0;
}


/* serial_exit:
 *  Shuts down the driver, freeing allocated resources, etc.  All channels
 *  will already have been destroyed by the `destroy_channel' function, so 
 *  normally all this needs to do is tidy up anything `init' left untidy.
 *  Return zero if successful, non-zero otherwise.
 */
static int serial_exit (void)
{
    free_list (hl_port_list, free_port);
    return __libnet_internal__serial_exit ();
}


/* serial_init_channel:
 *  This function will be called when a channel is created, before any 
 *  data is sent.  It should at least set the channel's return address to
 *  something meaningful or at least to a valid string if the driver can't
 *  supply a return address.  It can allocate memory, pointed to by the
 *  `data' field of the channel struct.  Return zero if successful, non-zero
 *  otherwise.
 */
static int serial_init_channel (NET_CHANNEL *chan, const char *addr)
{
    int port_nr;
    int slot;
    struct channel_data *data;

    if (decode_binding (addr, &port_nr, &slot))
	return -1;
    
    if (((port_nr != ALL) && (port_nr < 0)) || (port_nr >= MAX_PORT))
	return -1;

    if ((port_unopened (port_nr)) && (open_port (port_nr) < 0))
	return -1;

    data = malloc (sizeof *data);
    if (!data) return -1;

    data->port_nr = port_nr;
    data->local_slot = slot;
    init_list (data->packet_list);
    chan->data = data;
    add_to_list (channel_data_list, data);

    if (data->port_nr == ALL)
	sprintf (chan->local_addr, ":%d", data->local_slot);
    else
	sprintf (chan->local_addr, "%d:%d", data->port_nr, data->local_slot);

    return 0;
}


/* serial_destroy_channel:
 *  Called when a channel is being closed.  This should tidy up anything 
 *  that init_channel or subsequent functions have left messy, for example 
 *  it should free any data block that has been allocated.  Return zero if 
 *  successful, non-zero otherwise.
 */
static int serial_destroy_channel (NET_CHANNEL *chan)
{
    struct channel_data *data = chan->data;

    del_from_list (data);
    free_list (data->packet_list, free_packet);
    free (data);

    /* If the channel was using a port listed in autoports and is
     * the last channel using that port, should we close the port?  */

    return 0;
}


/* serial_update_target:
 *  This will be called if the target_addr for the channel changes. 
 *  If necessary, the address could be converted into network format here.
 *  Return zero if any conversion was possible, but don't bother checking
 *  whether or not the target is reachable.
 */
static int serial_update_target (NET_CHANNEL *chan)
{
    struct channel_data *data = chan->data;
    int port_nr, slot;

    /* This should probably not allow NULL and "" through.  */
    if (decode_binding (chan->target_addr, &port_nr, &slot))
	return -1;
    
    if (((port_nr != ALL) && (port_nr < 0)) || (port_nr >= MAX_PORT))
	return -1;

    if (port_unopened (port_nr))
	if (open_port (port_nr) < 0) return -1;

    data->port_nr = port_nr;
    data->remote_slot = slot;
    return 0;
}


/* serial_send:
 *  Send the given data block down the channel.  `size' is in bytes.
 *  Return zero if successful, otherwise return non-zero.  Don't bother
 *  checking that the packet arrived at the destination though.
 */

static void serial_send_all(struct port *port,
			    const unsigned char *buf, int size)
{
    int sent;
    while (size) {
	sent = __libnet_internal__serial_send(port, buf, size);
	buf += sent;
	size -= sent;
    }
}

static inline void do_send (struct port *port, const void *header,
			    const void *buf, int size,
                            const void *checksum)
{
    serial_send_all (port, header, 12);
    serial_send_all (port, buf, size);
    serial_send_all (port, checksum, 4);
}

static int serial_send (NET_CHANNEL *chan, const void *buf, int size)
{
    /* Packets are sent in the following format:
     * - 4 PACKET_MARKER
     * - 2 remote_slot (as seen by sender)
     * - 2 local_slot (as seen by sender)
     * - 2 length of packet (n, in bytes, data only)
     * - 2 checksum for remote_slot, local_slot and length
     * - n packet data
     * - 4 checksum for packet data
     */
    struct channel_data *data = chan->data;
    struct hl_port *hl_port;

    unsigned char header[12];
    unsigned char checksum[4];

    int chk;

    header[0] = (unsigned char) (PACKET_MARKER >> 24);
    header[1] = (unsigned char) (PACKET_MARKER >> 16);
    header[2] = (unsigned char) (PACKET_MARKER >> 8 );
    header[3] = (unsigned char) (PACKET_MARKER      );
    header[4] = data->remote_slot >> 8;
    header[5] = data->remote_slot;
    header[6] = data->local_slot >> 8;
    header[7] = data->local_slot;
    header[8] = size >> 8;
    header[9] = size;

    chk = rls_checksum (data->remote_slot, data->local_slot, size);

    header[10] = chk >> 8;
    header[11] = chk;

    chk = data_checksum_l (buf, size);

    checksum[0] = chk >> 24;
    checksum[1] = chk >> 16;
    checksum[2] = chk >> 8;
    checksum[3] = chk;

    if (data->port_nr == ALL) {
	foreach (hl_port, hl_port_list)
	    do_send (hl_port->port, header, buf, size, checksum);
	return 0;
    }
    
    foreach (hl_port, hl_port_list)
        if (hl_port->port_nr == data->port_nr) {
	    do_send (hl_port->port, header, buf, size, checksum);
	    return 0;
	}

    return -1;
}


/* serial_recv:
 *  Receive a packet into the memory block given.  `size' is the maximum
 *  number of bytes to receive.  The sender's address will be put in `from'.
 *  Return the number of bytes received, or -1 if an error occured.  A zero
 *  return value means there was no data to receive.
 */
static int serial_recv (NET_CHANNEL *chan, void *buf, int size, char *from)
{
    struct channel_data *data = chan->data;
    struct packet *packet;

    poll_ports ();

    if (list_empty (data->packet_list))
	return 0;

    packet = data->packet_list.next;

    if (packet->size < size)
	size = packet->size;
    memcpy (buf, packet->data, size);

    if (from)
	sprintf (from, "%i:%i", packet->from_port_nr, packet->from_slot);

    del_from_list (packet);
    free_packet (packet);

    return size;
}


/* serial_query:
 *  Returns non-zero if there is data waiting to be read from the channel.
 */
static int serial_query (NET_CHANNEL *chan)
{
    poll_ports ();
    return !(list_empty (((struct channel_data *) chan->data)->packet_list));
}


/* serial_load_config:
 *  This will be called once when the library is initialised, inviting the
 *  driver to load configuration information from the passed text file.
 */

static void parse_autoports (char *value)
{
    int i;
    char *endptr;
        
    for (i = 0; i < MAX_PORT; i++)
	autoports[i] = 0;
    
    while (1) {
	i = strtol (value, &endptr, 10);
	if (endptr == value) break;
	if ((i >= 0) && (i < MAX_PORT))
	    autoports[i] = 1;
	value = endptr;
    }
}

static void serial_load_config (NET_DRIVER *drv, FILE *fp)
{
    char section[] = "serial port #", *p;
    char *option, *value;
    int i;

    /* [serial port #] */
    p = strchr (section, '#');
    for (i = 0; i < MAX_PORT; i++) {
	*p = '0' + i;
	if (__libnet_internal__seek_section (fp, section))
	    continue;
	while (!__libnet_internal__get_setting (fp, &option, &value))
	    __libnet_internal__serial_load_config (i, option, value);
    }

    /* [serial] */
    if (!__libnet_internal__seek_section (fp, "serial")) {
	while (!__libnet_internal__get_setting (fp, &option, &value)) {
	    if (!strcmp (option, "autoports"))
		parse_autoports (value);
	    else if (!strcmp (option, "slot"))
		default_slot = atoi (value);
	    else if (!strcmp (option, "disable"))
		disable_driver = (atoi (value) || (value[0] == 'y'));
	}
    }

    if (drv->load_conn_config)
	drv->load_conn_config (drv, fp, "serial");
}

#endif


/* NET_DRIVER structures.  */

static int dummy_detect (void) { return NET_DETECT_NO; }
static void dummy_load_config (NET_DRIVER *drv, FILE *fp) { }


#define MAKE_REAL_SERIAL_DRIVER(drv, name)		\
NET_DRIVER drv = {					\
    name,						\
    "Serial driver",					\
    NET_CLASS_SERIAL,					\
							\
    serial_detect,					\
    serial_init,					\
    serial_exit,					\
							\
    NULL, NULL,						\
							\
    serial_init_channel,				\
    serial_destroy_channel,				\
							\
    serial_update_target,				\
    serial_send,					\
    serial_recv,					\
    serial_query,					\
							\
    NULL, NULL,						\
    NULL, NULL, NULL, NULL,				\
    NULL, NULL, NULL, NULL, NULL,			\
							\
    serial_load_config,					\
    NULL, NULL						\
};


#define MAKE_DUMMY_SERIAL_DRIVER(drv, name)		\
NET_DRIVER drv = {					\
    name, "Dummy driver",				\
    NET_CLASS_SERIAL,					\
    dummy_detect,					\
    NULL, NULL,						\
    NULL, NULL,						\
    NULL, NULL,						\
    NULL, NULL, NULL, NULL,				\
    NULL, NULL, NULL, NULL, NULL, NULL,			\
    NULL, NULL, NULL, NULL, NULL,			\
    dummy_load_config,					\
    NULL, NULL						\
};


#ifdef __USE_REAL_SERIAL_DOS__
    MAKE_REAL_SERIAL_DRIVER (net_driver_serial_dos, "Serial ports in DOS");
#else
    MAKE_DUMMY_SERIAL_DRIVER (net_driver_serial_dos, "Serial ports in DOS");
#endif


#ifdef __USE_REAL_SERIAL_LINUX__
    MAKE_REAL_SERIAL_DRIVER (net_driver_serial_linux, "Serial ports in Linux");
#else
    MAKE_DUMMY_SERIAL_DRIVER (net_driver_serial_linux, "Serial ports in Linux");
#endif


#ifdef __USE_REAL_SERIAL_BEOS__
    MAKE_REAL_SERIAL_DRIVER (net_driver_serial_beos, "Serial ports in BeOS");
#else
    MAKE_DUMMY_SERIAL_DRIVER (net_driver_serial_beos, "Serial ports in BeOS");
#endif
