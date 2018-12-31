/*----------------------------------------------------------------
 * nonet.c - dummy network driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <string.h>

#include "libnet.h"
#include "internal.h"

static char nonet_desc[80] = "No networking";

static int nonet_detect(void) { return NET_DETECT_YES; }
static int nonet_init(void)   { return 0; }
static int nonet_exit(void)   { return 0; }

static int nonet_init_channel    (NET_CHANNEL *chan, const char *addr) { strcpy (chan->local_addr, "(n/a)"); return 0; }
static int nonet_destroy_channel (NET_CHANNEL *chan) { return 0; }
static int nonet_update_target   (NET_CHANNEL *chan) { return 0; }

static int nonet_send (NET_CHANNEL *chan, const void *buf, int size) { return 0; }
static int nonet_recv (NET_CHANNEL *chan, void *buf, int size, char *x) { return 0; }

static int nonet_query (NET_CHANNEL *chan) { return 0; }

static int nonet_init_conn (NET_CONN *conn, const char *addr) { return 0; }
static int nonet_destroy_conn (NET_CONN *conn) { return 0; }
static int nonet_listen (NET_CONN *conn) { return -1; }
static int nonet_poll_listen (NET_CONN *conn, NET_CONN *newconn) { return 0; }
static int nonet_connect (NET_CONN *conn, const char *addr) { return -1; }
static int nonet_poll_connect (NET_CONN *conn) { return -1; }

static int nonet_send_rdm (NET_CONN *conn, const void *buf, int size) { return -1; }
static int nonet_recv_rdm (NET_CONN *conn, void *buf, int max) { return -1; }
static int nonet_query_rdm (NET_CONN *conn) { return 0; }
static int nonet_ignore_rdm (NET_CONN *conn) { return 0; }
static int nonet_conn_stats (NET_CONN *conn, int *in_q, int *out_q) { if (in_q) *in_q = 0; if (out_q) *out_q = 0; return 0; }

static void nonet_load_config (NET_DRIVER *drv, FILE *fp) { }
static void nonet_load_conn_config (NET_DRIVER *drv, FILE *fp, const char *section) { }


NET_DRIVER net_driver_nonet = {
	"No network",    /* name */
	nonet_desc,
	NET_CLASS_NONE,

	nonet_detect,
	nonet_init,
	nonet_exit,

	NULL, NULL,

	nonet_init_channel,
	nonet_destroy_channel,

	nonet_update_target,
	nonet_send,
	nonet_recv,
	nonet_query,

	nonet_init_conn,
	nonet_destroy_conn,
	nonet_listen,
	nonet_poll_listen,
	nonet_connect,
	nonet_poll_connect,

	nonet_send_rdm,
	nonet_recv_rdm,
	nonet_query_rdm,
	nonet_ignore_rdm,
	nonet_conn_stats,

	nonet_load_config,
	nonet_load_conn_config,
	NULL
};

