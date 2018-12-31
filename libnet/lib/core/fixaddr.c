/*----------------------------------------------------------------
 * fixaddr.c -- routines to fix up addresses
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#include <string.h>
#include "libnet.h"
#include "internal.h"


static int fixup_address (const char *target_addr, const char *remote_chan_addr, char *dest)
{
	const char *p, *colon;
	int i = 0;

	p = target_addr;

	/* copy the address part the target address (sans port) */
	colon = strrchr (p, ':');
	if (colon) {
		i = colon - p + 1;	/* include the colon */
		memcpy (dest, target_addr, i);
	} else {
		strcpy (dest, target_addr);
		i = strlen (dest);
		dest[i++] = ':';
	}

	/* add the port number of the remote channel */
	colon = strrchr (remote_chan_addr, ':');
	p = (colon ? (colon+1) : remote_chan_addr);

	strcpy (dest + i, p);

	return 0;
}


/* net_fixupaddress_conn:
 *  Given `conn' and an address on the remote machine, whose actual address
 *  part may not work from the local machine, this function fills in `dest'
 *  with an address which should work.
 */
int net_fixupaddress_conn (NET_CONN *conn, const char *remote_chan_addr, char *dest)
{
	const char *p;

	if (!(conn && remote_chan_addr && dest))
		return -1;

	p = net_getpeer (conn);
	if (!p) return -1;

	return fixup_address (p, remote_chan_addr, dest);
}


/* net_fixupaddress_channel:
 *  Given `chan' and an address on the remote machine, whose actual address
 *  part may not work from the local machine, this function fills in `dest'
 *  with an address which should work.
 */
int net_fixupaddress_channel (NET_CHANNEL *chan, const char *remote_chan_addr, char *dest)
{
	const char *p;

	if (!(chan && remote_chan_addr && dest))
		return -1;

	p = chan->target_addr;
	return fixup_address (p, remote_chan_addr, dest);
}
