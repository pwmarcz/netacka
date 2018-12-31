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
#include "drivers.h"


/* net_prepareaddress:
 *  Prepares `addr' for fast use, storing a result in `dest'.  This ensures
 *  that calls to addressing functions, passed `dest', will not block (e.g.
 *  doing a DNS lookup).
 *
 *  This function will not complete immediately.  It returns a pointer for
 *  internal use, or NULL if the conversion was impossible.  The pointer
 *  should be passed back to net_poll_prepareaddress frequently, until that
 *  function succeeds or fails.
 */
NET_PREPADDR_DATA *net_prepareaddress (int type, const char *addr, char *dest)
{
	NET_DRIVER *driver = __libnet_internal__get_driver (type);
	struct NET_PREPADDR_DATA *data;

	if (!driver) return NULL;

	data = malloc (sizeof *data);
	if (!data) return NULL;

	data->driver = driver;
	strncpy (data->src, addr, NET_MAX_ADDRESS_LENGTH);
	data->dest = dest;
	
	if (driver->prepareaddress) {
		if (driver->prepareaddress (data)) {
			free (data);
			return NULL;
		}
	}

	return data;
}


/* net_poll_prepareaddress:
 *  Polls a request to prepare an address.  Takes the pointer returned 
 *  by net_prepareaddress, and returns 0 if the address is not yet ready,
 *  1 if it is ready, and -1 if the conversion failed.
 */
int net_poll_prepareaddress (NET_PREPADDR_DATA *data)
{
	int result;

	if (!data) return -1;

	if (data->driver->poll_prepareaddress) {
		result = data->driver->poll_prepareaddress (data);
	} else {
		/* special case for drivers which don't use this */
		strncpy (data->dest, data->src, NET_MAX_ADDRESS_LENGTH);
		result = 1;
	}

	if (result) free (data);

	return result;
}

