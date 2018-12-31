/*----------------------------------------------------------------
 * template.c - template network driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

/* Note: This file is out of date.  I'll update it soon... */


/* How to make a new driver:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1) Copy this file to one for your driver.
 * 2) Fill in the blanks -- replace the dummy function bodies with whatever
 *    your driver needs.  The comments tell you what each function should
 *    do.
 * 3) For testing, edit the makefile, adding your driver's filename 
 *    (without the extension) to the list starting `DRIVERS ='.  If your
 *    file has any special dependencies (other than libnet.h) add an entry
 *    for it further down in the makefile, under `drivers/wsock.o: ...'.
 * 4) Add a #define for your driver in include/libnet.h, using the next free 
 *    number (e.g. 1, 2, 3, 4, ...).  Add your number onto NET_DRIVER_ALL.

 (note: this needs revision, the remaining instructions are wrong)

 * 5) Add your driver's NET_DRIVER struct to the `extern NET_DRIVER' list.
 * 6) Add an entry to the net_driver_list[] in `core/core.c'.
 * 7) Test it.
 * 8) Send me your source code and any special details (e.g. makefile 
 *    dependencies).
 *
 * Any questions?  Ask me: george.foot@merton.ox.ac.uk
 */

#include <stdlib.h>
#include <string.h>

#include "libnet.h"
#include "internal.h"
#include "config.h"


/* desc:
 *  This is a string describing the driver -- it can be static like this,
 *  or it can be set dynamically by the `detect' function, which can add
 *  information on what it detected.
 */
static char template_desc[80] = "Template network driver";

/* detect:
 *  This function returns one of the following:
 *
 *  - NET_DETECT_YES    (if the network driver can definitely work)
 *  - NET_DETECT_MAYBE  (if the network driver might be able to work)
 *  - NET_DETECT_NO     (if the network driver definitely cannot work)
 *
 *  We also return NET_DETECT_NO if the driver is disabled.
 */
static int disable_driver = 0;
static int template_detect (void)
{
	if (disable_driver)
		return NET_DETECT_NO;
	else
		return NET_DETECT_YES;
}

/* init:
 *  Initialises the driver.  Return zero if successful, non-zero if not.
 */
static int template_init (void) { return 0; }

/* exit:
 *  Shuts down the driver, freeing allocated resources, etc.  All channels
 *  will already have been destroyed by the `destroy_channel' function, so 
 *  normally all this needs to do is tidy up anything `init' left untidy.
 *  Return zero if successful, non-zero otherwise.
 */
static int template_exit (void) { return 0; }


/* prepareaddress:
 *  Called to initiate preparation of an address for use.  This should convert
 *  the address in any way necessary such that the result can be used in
 *  other routines efficiently (i.e. without waiting for DNS lookups etc).
 *  Return zero if successful, non-zero on error.
 */
static int template_prepareaddress (NET_PREPADDR_DATA *d) { strncpy (d->dest, d->src, NET_MAX_ADDRESS_LENGTH); return 0; }

/* poll_prepareaddress:
 *  Called to continue preparation of an address, as above.  Return zero if
 *  still in progress, -1 on failure, or 1 on success.
 */
static int template_poll_prepareaddress (NET_PREPADDR_DATA *d) { return 1; }


/* init_channel:
 *  This function will be called when a channel is created, before any 
 *  data is sent.  It should at least set the channel's return address to
 *  something meaningful or at least to a valid string if the driver can't
 *  supply a return address.  It can allocate memory, pointed to by the
 *  `data' field of the channel struct.  Return zero if successful, non-zero
 *  otherwise.
 */
static int template_init_channel    (NET_CHANNEL *chan, const char *addr) { strcpy (chan->local_addr, ""); return 0; }

/* destroy_channel:
 *  Called when a channel is being closed.  This should tidy up anything 
 *  that init_channel or subsequent functions have left messy, for example 
 *  it should free any data block that has been allocated.  Return zero if 
 *  successful, non-zero otherwise.
 */
static int template_destroy_channel (NET_CHANNEL *chan) { return 0; }

/* update_target:
 *  This will be called if the target_addr for the channel changes. 
 *  If necessary, the address could be converted into network format here.
 *  Return zero if any conversion was possible, but don't bother checking
 *  whether or not the target is reachable.
 */
static int template_update_target   (NET_CHANNEL *chan) { return 0; }


/* send:
 *  Send the given data block down the channel.  `size' is in bytes.
 *  Return zero if successful, otherwise return non-zero.  Don't bother
 *  checking that the packet arrived at the destination though.
 */
static int template_send (NET_CHANNEL *chan, const void *buf, int size) { return 0; }

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
static int template_recv (NET_CHANNEL *chan, void *buf, int size, char *from) { return 0; }

/* query:
 *  Returns non-zero if there is data waiting to be read from the channel.
 */
static int template_query (NET_CHANNEL *chan) { return 0; }


/* Note on conn functions:  It's recommended that you leave all these 
 * entries NULL.  Libnet will provide its own conn system, implemented
 * on top of the channel system.
 */

/* init_conn:
 *  This function is called to initialise a conn.  The `addr' parameter
 *  controls the local association of the conn -- if it's NULL, the driver
 *  can use any association; if it's an empty string the driver should use
 *  the default association (from the config file, or hardcoded); otherwise
 *  the driver should interpret the string as appropriate to determine what
 *  binding to use.  Return zero on success, nonzero on failure.
 */
static int template_init_conn (NET_CONN *conn, const char *addr) { return 0; }

/* destroy_conn:
 *  This function is called before an initialised conn is destroyed, to
 *  allow the driver to tidy up.  Return zero on success, nonzero on failure.
 */
static int template_destroy_conn (NET_CONN *conn) { return 0; }

/* listen:
 *  Makes a conn start listening.  This function should not block.  Return
 *  zero on success, nonzero otherwise.
 */
static int template_listen (NET_CONN *conn) { return 0; }

/* poll_listen:
 *  Checks whether or not a listening conn has an incoming connection 
 *  attempt.  If it does, the connection is accepted and `newconn'
 *  is initialised to refer to the connection.  `conn' should keep
 *  listening.  Note that `newconn' already points at allocated memory
 *  before the call.  Return zero if there are no incoming connections,
 *  nonzero otherwise.
 */
static int template_poll_listen (NET_CONN *conn, NET_CONN *newconn) { return 0; }

/* connect:
 *  Starts a connection attempt to `addr'.  Return nonzero if an error
 *  occurs, zero if the connection attempt is begun.
 */
static int template_connect (NET_CONN *conn, const char *addr) { return 0; }

/* poll_connect:
 *  Checks whether or not a connecting channel has connected or been 
 *  refused.  Return zero if it is still trying to connect, negative
 *  if it has been refused, positive if it has succeeded.
 */
static int template_poll_connect (NET_CONN *conn) { return 0; }

/* send_rdm:
 *  Sends a reliably delivered message along a connected conn.  Return
 *  zero on success, nonzero on error.
 */
static int template_send_rdm (NET_CONN *conn, const void *buf, int size) { return 0; }

/* recv_rdm:
 *  Tries to receive one RDM from the conn.  Return the number of bytes
 *  received, or -1 on error.
 */
static int template_recv_rdm (NET_CONN *conn, void *buf, int max) { return -1; }

/* query_rdm:
 *  Tests for incoming data on the conn.  Return nonzero if data is 
 *  waiting, zero if not.
 */
static int template_query_rdm (NET_CONN *conn) { return 0; }

/* ignore_rdm:
 *  Ignores the next packet, returning zero if no packet was there, 
 *  1 otherwise.
 */
static int template_ignore_rdm (NET_CONN *conn) { return 0; }

/* conn_stats:
 *  Gets the number of packets waiting in each queue.  in_q and out_q 
 *  point to ints to store the info in; if one is NULL, don't store 
 *  anything there.  Return zero on success.
 */
static int template_conn_stats (NET_CONN *conn, int *in_q, int *out_q) { if (in_q) *in_q = 0; if (out_q) *out_q = 0; return 0; }


/* load_config:
 *  This will be called once when the library is initialised, inviting the
 *  driver to load configuration information from the passed text file.
 */
static void template_load_config (NET_DRIVER *drv, FILE *fp)
{
	char *option, *value;
	
	/* Find the section we're interested in (maybe more than one --
	 * read them one at a time) */
	if (__libnet_internal__seek_section (fp, "template")) return;
	
	/* Process the settings one by one */
	while (__libnet_internal__get_setting (fp, &option, &value) == 0) {
		/* This option is common to all drivers; if set, the `detect'
		 * routine always fails */
		if (!strcmp (option, "disable")) {
			disable_driver = (atoi (value) || (value[0] == 'y'));
		}
	}
	
	/* Chain to the loader for the conn routines, unless you wrote your
         * own conn routines -- in this file, we did. */
//	if (drv->load_conn_config) drv->load_conn_config (drv, fp, "template");
}



NET_DRIVER net_driver_template = {
	"No network",    /* name -- initialise this staticly */
	template_desc,
	NET_CLASS_NONE,

	template_detect,
	template_init,
	template_exit,

	template_prepareaddress,
	template_poll_prepareaddress,

	template_init_channel,
	template_destroy_channel,

	template_update_target,
	template_send,
	template_recv,
	template_query,

	template_init_conn,
	template_destroy_conn,

	template_listen,
	template_poll_listen,
	template_connect,
	template_poll_connect,

	template_send_rdm,
	template_recv_rdm,
	template_query_rdm,
	template_ignore_rdm,
	template_conn_stats,

	template_load_config,
	NULL, NULL
};

