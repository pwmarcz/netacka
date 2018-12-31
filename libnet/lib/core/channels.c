/*----------------------------------------------------------------
 * channels.c - channel handling routines for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <stdlib.h>
#include <string.h>

#include "libnet.h"
#include "internal.h"
#include "channels.h"
#include "drivers.h"
#include "threads.h"


/* Maybe this could be static?  Hrm... */
struct __channel_list_t *__libnet_internal__openchannels;
MUTEX_DEFINE(__libnet_internal__openchannels);

/* __libnet_internal__channels_init:  (internal)
 *  Performs internal initialisation.
 */
void __libnet_internal__channels_init() {
	__libnet_internal__openchannels = (struct __channel_list_t *) malloc (sizeof(struct __channel_list_t));
	__libnet_internal__openchannels->next = NULL;
	__libnet_internal__openchannels->chan = NULL;
	MUTEX_CREATE(__libnet_internal__openchannels);
}


/* __libnet_internal__channels_exit:  (internal)
 *  Internal shutdown routine.
 */
void __libnet_internal__channels_exit() {
	while (__libnet_internal__openchannels->next) net_closechannel(__libnet_internal__openchannels->next->chan);
	free (__libnet_internal__openchannels);
	__libnet_internal__openchannels = NULL;
	MUTEX_DESTROY(__libnet_internal__openchannels);
}


/* create_chan:  (internal)
 *  A little helper function to create a new channel.
 */
static struct __channel_list_t *create_chan (void) {
	struct __channel_list_t *ptr;

	ptr = (struct __channel_list_t *) malloc (sizeof(struct __channel_list_t));
	if (!ptr) return NULL;

	ptr->chan = (NET_CHANNEL *) malloc (sizeof (NET_CHANNEL));
	if (!ptr->chan) {
		free(ptr);
		return NULL;
	}

	ptr->chan->type = 0;
	ptr->chan->driver = NULL;
	ptr->chan->target_addr[0] = '\0';
	ptr->chan->local_addr[0] = '\0';
	ptr->chan->data = NULL;

	/* Not thread-safe here */
	ptr->next = __libnet_internal__openchannels->next;
	__libnet_internal__openchannels->next = ptr;

	return ptr;
}


/* destroy_chan:  (internal)
 *  Destroys the given channel.
 */
static void destroy_chan (struct __channel_list_t *ptr)
{
	/* Not thread-safe */
	struct __channel_list_t *ptr2 = __libnet_internal__openchannels;
	while (ptr2 && (ptr2->next != ptr)) ptr2 = ptr2->next;
	if (ptr2) {
		ptr2->next = ptr->next;
		free (ptr->chan);
		free (ptr);
	}
}


/* net_openchannel:
 *  Opens a communications channel over the specified network
 *  type `type'. It returns a pointer to the NET_CHANNEL struct
 *  created, or NULL on error.  addr determines the local binding.
 */
NET_CHANNEL *net_openchannel (int type, const char *addr)
{
	struct __channel_list_t *ptr;
	
	MUTEX_LOCK(__libnet_internal__openchannels);

	if ((ptr = create_chan ())) {
		NET_CHANNEL *chan = ptr->chan;
		chan->type = type;
		chan->driver = __libnet_internal__get_driver (type);
	
		if (chan->driver && !chan->driver->init_channel (chan, addr)) {
			MUTEX_UNLOCK(__libnet_internal__openchannels);
			return chan;
		}
	
		destroy_chan (ptr);
	}

	MUTEX_UNLOCK(__libnet_internal__openchannels);

	return NULL;
}


/* net_closechannel:
 *  Closes a previously opened channel. Returns 0 on success.
 */
int net_closechannel (NET_CHANNEL *channel)
{
	struct __channel_list_t *ptr, *optr;

	MUTEX_LOCK(__libnet_internal__openchannels);

	for (ptr = __libnet_internal__openchannels; ptr->next && (ptr->next->chan != channel); ptr = ptr->next);
	if (!ptr->next) {
		MUTEX_UNLOCK(__libnet_internal__openchannels);
		return 1;
	}

	optr = ptr->next;
	ptr->next = optr->next;
	free (optr);

	MUTEX_UNLOCK(__libnet_internal__openchannels);

	channel->driver->destroy_channel (channel);
	free (channel);
	return 0;
}


/* net_assigntarget:
 *  Assigns a target (`target') to the given channel. The format
 *  of `target' depends upon the specific network type; e.g. for
 *  UDP, it could be a null-terminated string: "127.0.0.1:12345".
 *  Returns zero on success, non-zero on error. `Success' doesn't
 *  guarrantee that the target is reachable.
 */
int net_assigntarget (NET_CHANNEL *channel, const char *target)
{
	strcpy (channel->target_addr, target);
	return channel->driver->update_target (channel);
}


/* net_getlocaladdress:
 *  Returns the local address of the given channel -- this is
 *  probably the address other computers would use to send into
 *  this channel, but may not be for some network drivers.
 */
char *net_getlocaladdress (NET_CHANNEL *channel)
{
	return channel ? channel->local_addr : NULL;
}


/* net_send:
 *  Sends data down a channel, returning zero to indicate
 *  success or non-zero if an error occurs.
 */
int net_send (NET_CHANNEL *channel, const void *buffer, int size)
{
	return channel->driver->send (channel, buffer, size);
}


/* net_receive:
 *  Receives data from a channel, maximum `maxsize' bytes,
 *  storing the data in `buffer' and pointing `from' to a
 *  string containing the sender's address. Returns the number 
 *  of bytes received. 0 is a valid return type; there was no 
 *  data to read. -1 indicates that an error occured.
 */
int net_receive (NET_CHANNEL *channel, void *buffer, int maxsize, char *from)
{
	return channel->driver->recv (channel, buffer, maxsize, from);
}


/* net_query:
 *  Returns nonzero if data is ready for reading from the
 *  given channel.
 */
int net_query (NET_CHANNEL *channel)
{
	return channel->driver->query (channel);
}


/* net_channel_driver:
 *  Returns the ID of the driver this channel is using.
 */
int net_channel_driver (NET_CHANNEL *chan)
{
        return __libnet_internal__get_driver_by_ptr (chan->driver);
}


