/*----------------------------------------------------------------
 *   wait.c -- functions to wait for activity
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <libnet.h>
#include "timer.h"
#include "channels.h"
#include "conns.h"

/* Generic function -- this works on any platform, but keeps the CPU busy */
/* Based on Gillius's net_wait_channels function */
static void *net_wait_all_generic (int tv)
{
	unsigned int start_time = __libnet_timer_func();
	do {
		{
			struct __channel_list_t *currchan = __libnet_internal__openchannels->next;
			while (currchan) {
				if (net_query (currchan->chan)) return currchan->chan;
				currchan = currchan->next;
			}
		}
		{
			struct __conn_list_t *currconn = __libnet_internal__openconns->next;
			while (currconn) {
				if (net_query_rdm (currconn->conn)) return currconn->conn;
				currconn = currconn->next;
			}
		}
	} while (tv < 0 || (__libnet_timer_func() - start_time) < (unsigned)tv);
	return NULL;
}

void *net_wait_all (int tv)
{
#if defined TARGET_MSVC
//... platform-specific code
	return net_wait_all_generic (tv);
#else
	return net_wait_all_generic (tv);
#endif
}

