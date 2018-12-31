/*----------------------------------------------------------------
 * conns.c - conn handling routines for libnet
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
#include "conns.h"
#include "drivers.h"
#include "threads.h"
#include "timer.h"


/* Maybe this could be static?  Hrm... */
struct __conn_list_t *__libnet_internal__openconns;
MUTEX_DEFINE(__libnet_internal__openconns);


/* __libnet_internal__conns_init:  (internal)
 *  Performs internal initialisation.
 */
void __libnet_internal__conns_init() {
	__libnet_internal__openconns = (struct __conn_list_t *) malloc (sizeof (struct __conn_list_t));
	__libnet_internal__openconns->next = NULL;
	__libnet_internal__openconns->conn = NULL;
	MUTEX_CREATE(__libnet_internal__openconns);
}


/* __libnet_internal__conns_exit:  (internal)
 *  Internal shutdown routine.
 */
void __libnet_internal__conns_exit() {
	while (__libnet_internal__openconns->next) net_closeconn(__libnet_internal__openconns->next->conn);
	free (__libnet_internal__openconns);
	__libnet_internal__openconns = NULL;
	MUTEX_DESTROY(__libnet_internal__openconns);
}


/* create_conn:  (internal)
 *  Inserts a new conn into the chain.
 */
static struct __conn_list_t *create_conn (void) {
	struct __conn_list_t *ptr;

	ptr = (struct __conn_list_t *) malloc (sizeof (struct __conn_list_t));
	if (!ptr) return NULL;

	ptr->conn = (NET_CONN *) malloc (sizeof (NET_CONN));
	if (!ptr->conn) {
		free(ptr);
		return NULL;
	}

	ptr->conn->type = 0;
	ptr->conn->driver = NULL;
	ptr->conn->status = NET_CONN_IDLE;
	ptr->conn->data = NULL;

	/* Not thread-safe here */
	ptr->next = __libnet_internal__openconns->next;
	__libnet_internal__openconns->next = ptr;

	return ptr;
}


/* destroy_conn:  (internal)
 *  Destroys the given conn.
 */
static void destroy_conn (struct __conn_list_t *ptr)
{
	/* Not thread-safe here */
	struct __conn_list_t *ptr2 = __libnet_internal__openconns;
	while (ptr2 && (ptr2->next != ptr)) ptr2 = ptr2->next;
	if (ptr2) {
		ptr2->next = ptr->next;
		free (ptr->conn);
		free (ptr);
	}
}


/* net_openconn:
 *  Opens a conn over the specified network type `type'.  `addr' defines
 *  whether or not the conn should be use a specific local association. 
 *  The function returns a pointer to the NET_CONN struct created, or 
 *  NULL on error.
 */
NET_CONN *net_openconn (int type, const char *addr)
{
	struct __conn_list_t *ptr;

	MUTEX_LOCK(__libnet_internal__openconns);

	if ((ptr = create_conn())) {
		NET_CONN *conn = ptr->conn;
		conn->type = type;
		conn->driver = __libnet_internal__get_driver (type);

		if (conn->driver && !conn->driver->init_conn (conn, addr)) {
			MUTEX_UNLOCK(__libnet_internal__openconns);
			return conn;
		}

		destroy_conn (ptr);
	}

	MUTEX_UNLOCK(__libnet_internal__openconns);

	return NULL;
}


/* net_closeconn:
 *  Closes a previously opened conn. Returns zero on success.
 */
int net_closeconn (NET_CONN *conn)
{
	struct __conn_list_t *ptr;

	MUTEX_LOCK(__libnet_internal__openconns);

	ptr = __libnet_internal__openconns->next;
	while (ptr && (ptr->conn != conn)) ptr = ptr->next;
	if (!ptr) {
		MUTEX_UNLOCK(__libnet_internal__openconns);
		return -1;
	}

	conn->driver->destroy_conn (conn);

	destroy_conn (ptr);

	MUTEX_UNLOCK(__libnet_internal__openconns);

	return 0;
}


/* net_send_rdm:
 *  Sends data down a conn, returning zero to indicate
 *  success or non-zero if an error occurs.
 */
int net_send_rdm (NET_CONN *conn, const void *buffer, int size)
{
	return conn->driver->send_rdm (conn, buffer, size);
}


/* net_receive_rdm:
 *  Receives data from a conn, maximum `maxsize' bytes,
 *  storing the data in `buffer'.  Returns the number of 
 *  bytes received.  0 is a valid return type; there was 
 *  no data to read.  -1 indicates that an error occured.
 */
int net_receive_rdm (NET_CONN *conn, void *buffer, int maxsize)
{
	return conn->driver->recv_rdm (conn, buffer, maxsize);
}


/* net_query_rdm:
 *  Returns nonzero if data is ready for reading from the
 *  given conn.
 */
int net_query_rdm (NET_CONN *conn)
{
	return conn->driver->query_rdm (conn);
}


/* net_ignore_rdm:
 *  Ignores the next queued incoming packet, if any.  Returns nonzero
 *  if it does ignore anything, zero if nothing was queued.
 */
int net_ignore_rdm (NET_CONN *conn)
{
	return conn->driver->ignore_rdm (conn);
}


/* net_conn_stats:
 *  Gets the number of packets in each queue.  You can pass NULL
 *  if you're not interested in one of the queues.
 */
int net_conn_stats (NET_CONN *conn, int *in_q, int *out_q)
{
	return conn->driver->conn_stats (conn, in_q, out_q);
}


/* net_listen:
 *  Makes a conn start listening.  Only works on an idle conn.
 *  Returns zero on success, nonzero otherwise.
 */
int net_listen (NET_CONN *conn)
{
	if (conn->status != NET_CONN_IDLE) return -1;
	if (conn->driver->listen (conn)) {
		return -1;
	} else {
		conn->status = NET_CONN_LISTENING;
		return 0;
	}
}


/* net_poll_listen:
 *  Polls a listening conn for incoming connections.  If 
 *  there are any, this function accepts the first queued and
 *  returns a net NET_CONN * which the user can use to talk 
 *  to the connecting computer.  Otherwise NULL is returned.
 */
NET_CONN *net_poll_listen (NET_CONN *conn)
{
	static NET_CONN newconn;
	if (conn->status != NET_CONN_LISTENING) return NULL;
	newconn.type = conn->type;
	newconn.driver = conn->driver;
	if (conn->driver->poll_listen (conn, &newconn)) {
		struct __conn_list_t *ptr;
		newconn.status = NET_CONN_CONNECTED;
		MUTEX_LOCK(__libnet_internal__openconns);
		ptr = create_conn ();
		if (!ptr) {
			MUTEX_UNLOCK(__libnet_internal__openconns);
			conn->driver->destroy_conn (&newconn);
			return NULL;
		} else {
			memcpy (ptr->conn, &newconn, sizeof (NET_CONN));
			conn = ptr->conn;
			MUTEX_UNLOCK(__libnet_internal__openconns);
			return conn;
		}
	} else return NULL;
}


/* net_connect:
 *  Initiates a connection attempt to target `addr'.  Returns 
 *  zero if successful in initiating; nonzero otherwise.  If
 *  the return value is zero, the app should keep calling
 *  net_poll_connect until a connection is established or 
 *  refused, or until the app gets bored.
 */
int net_connect (NET_CONN *conn, const char *addr)
{
	if (conn->status != NET_CONN_IDLE) return -1;
	if (conn->driver->connect (conn, addr)) {
		return -1;
	} else {
		conn->status = NET_CONN_CONNECTING;
		return 0;
	}
}


/* net_poll_connect:
 *  Polls a connecting conn, returning zero if the connection 
 *  is still in progress, nonzero if the connection process 
 *  has ended.  A nonzero return value is either positive 
 *  (connection established) or negative (connection not 
 *  established).
 */
int net_poll_connect (NET_CONN *conn)
{
	int x;
	switch (conn->status) {
		case NET_CONN_CONNECTING:
			break;
		case NET_CONN_CONNECTED:
			return 1;
		default:
			return -1;
	}
	x = conn->driver->poll_connect (conn);
	if (x == 0) return 0;
	if (x < 0) {
		conn->status = NET_CONN_IDLE;
		return -1;
	} else {
		conn->status = NET_CONN_CONNECTED;
		return 1;
	}
}



/* safe_sleep:
 *  This routine acts roughly like `sleep', waiting a certain 
 *  number of seconds.  It's used because in Linux at least, 
 *  normal `sleep' uses SIGALRM in Linux, which we'd rather 
 *  not touch (Allegro likes it to be left alone).
 */
static void safe_sleep (int seconds)
{
	unsigned x = __libnet_timer_func();
	int y = 0;
	while (y < seconds * 1000) y = (unsigned)(__libnet_timer_func() - x);
}



/* net_connect_wait_*:
 *  These functions use net_connect and net_poll_connect to 
 *  establish a connection.  They wait until the connection 
 *  process is complete or some other event occurs (which 
 *  depends upon which function is used).  They return zero 
 *  if the connection is established, negative if there is 
 *  an error (e.g. connection refused) and positive if the 
 *  event occurs.
 */

/* net_connect_wait_time:
 *  See `net_connect_wait_*'.  The event in this case is that 
 *  `time' seconds have passed.
 */
int net_connect_wait_time (NET_CONN *conn, const char *addr, int time)
{
	int y;
	if (net_connect (conn, addr)) return -1;
	do {
		time--;
		y = net_poll_connect (conn);
		safe_sleep (1);
	} while ((time >= 0) && (y == 0));
	if (y < 0) return -1;
	if (y > 0) return 0;
	return 1;
}

/* net_connect_wait_cb:
 *  See `net_connect_wait_*'.  This version calls the given 
 *  callback while waiting, and aborts if the callback returns 
 *  nonzero.
 */
int net_connect_wait_cb (NET_CONN *conn, const char *addr, int (*cb)(void))
{
	int x = 0, y;
	if (net_connect (conn, addr)) return -1;
	do {
		if (cb) x = cb();
		y = net_poll_connect (conn);
		safe_sleep (1);
	} while ((x == 0) && (y == 0));
	if (y < 0) return -1;
	if (y > 0) return 0;
	return 1;
}

/* net_connect_wait_cb_time:
 *  See `net_connect_wait_*'.  This is a mixture of 
 *  `net_connect_wait_cb' and `net_connect_wait_time'; it
 *  calls the callback while waiting, and aborts if either
 *  the callback returns nonzero or the time limit expires.
 *
 *  If the callback is time consuming, the time limit will 
 *  be inaccurate.
 */
int net_connect_wait_cb_time (NET_CONN *conn, const char *addr, int (*cb)(void), int time)
{
	int x = 0, y;
	if (net_connect (conn, addr)) return -1;
	do {
		if (cb) x = cb();
		time--;
		y = net_poll_connect (conn);
		safe_sleep (1);
	} while ((x == 0) && (time > 0) && (y == 0));
	if (y < 0) return -1;
	if (y > 0) return 0;
	return 1;
}

/* net_getpeer:
 *  Returns the address of the peer, or NULL on error.  Pass a connected conn.
 */
char *net_getpeer (NET_CONN *conn)
{
	if (conn->status != NET_CONN_CONNECTED) return NULL;
	return conn->peer_addr;
}

/* net_conn_driver:
 *  Returns the ID of the driver this conn is using.
 */
int net_conn_driver (NET_CONN *conn)
{
	return __libnet_internal__get_driver_by_ptr (conn->driver);
}

