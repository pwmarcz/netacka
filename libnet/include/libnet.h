/*----------------------------------------------------------------
 * libnet.h - interface to libnet library
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-2001
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_libnet_h
#define libnet_included_file_libnet_h

#include <stdio.h>


#define LIBNET_VERSION_MAJOR 0
#define LIBNET_VERSION_MINOR 10
#define LIBNET_VERSION_PATCH 11
#define LIBNET_VERSION_STRING "0.10.11"


#ifdef __cplusplus
extern "C" {
#endif


/* Type to hold lists of drivers */
typedef int *NET_DRIVERLIST;

/* List containing all drivers */
extern NET_DRIVERLIST net_drivers_all;


/* Various types used to describe Libnet entities */
typedef struct NET_CHANNEL        NET_CHANNEL;
typedef struct NET_CONN           NET_CONN;
typedef struct NET_DRIVER         NET_DRIVER;
typedef struct NET_DRIVERNAME     NET_DRIVERNAME;
typedef struct NET_CLASS          NET_CLASS;
typedef struct NET_PREPADDR_DATA  NET_PREPADDR_DATA;


/* struct to hold information about a driver */
struct NET_DRIVERNAME {
  int num;                    /* reference number */
  char *name;                 /* driver name */
};

/* struct to hold information about a class of driver */
struct NET_CLASS {
  char *name;                 /* title of class */
  char *addrhelp;             /* help text for address format */
  NET_DRIVERLIST drivers;     /* list of drivers in this class */
};

/* List of known network classes */
extern NET_CLASS net_classes[];

/* Longest possible address size */
#define NET_MAX_ADDRESS_LENGTH  128

/* Largest allowable packet size */
#define NET_MAX_PACKET_SIZE     512

/* Autodetection results */
#define NET_DETECT_YES          2
#define NET_DETECT_MAYBE        1
#define NET_DETECT_NO           0

/* Driver selection constants */
#define NET_DRIVER_NONET         1       /* No networking */
#define NET_DRIVER_LOCAL         2       /* Local host */
#define NET_DRIVER_WSOCK_DOS     3       /* Winsock in DOS (v1.x only) */
#define NET_DRIVER_WSOCK_WIN     4       /* Winsock in Windows (any version) */
#define NET_DRIVER_SOCKETS       5       /* Berkeley sockets (Unix) */
#define NET_DRIVER_IPX_DOS       6       /* IPX in DOS */
#define NET_DRIVER_IPX_WIN       7       /* IPX in Windows */
#define NET_DRIVER_IPX_LINUX     8       /* IPX in Linux */
#define NET_DRIVER_SERIAL_DOS    9       /* Serial ports in DOS */
#define NET_DRIVER_SERIAL_WIN   10       /* Serial ports in Windows */
#define NET_DRIVER_SERIAL_LINUX 11       /* Serial ports in Linux */
#define NET_DRIVER_SERIAL_BEOS  12       /* Serial ports in BeOS */
#define NET_DRIVER_BESOCKS      13       /* BeOS Sockets*/

#define NET_DRIVER_USER        16       /* base for user driver numbers */
#define NET_DRIVER_MAX         32       /* maximum driver number + 1 */

/* Network class constants */
#define NET_CLASS_NONE          0
#define NET_CLASS_INET          1
#define NET_CLASS_IPX           2
#define NET_CLASS_SERIAL        3
#define NET_CLASS_LOCAL         4

#define NET_CLASS_USER          8
#define NET_CLASS_MAX          16

/* Conn status values */
#define NET_CONN_IDLE           1
#define NET_CONN_LISTENING      2
#define NET_CONN_CONNECTING     3
#define NET_CONN_CONNECTED      4


/* Core functions */
int net_init (void);
int net_register_driver (int num, NET_DRIVER *driver);
int net_loadconfig (const char *filename);
NET_DRIVERNAME *net_getdrivernames (NET_DRIVERLIST which);
NET_DRIVERLIST net_detectdrivers (NET_DRIVERLIST which);
int net_detectdriver (int which);
NET_DRIVERLIST net_initdrivers (NET_DRIVERLIST which);
int net_initdriver (int which);
int net_shutdown (void);


/* Driver list functions */
NET_DRIVERLIST net_driverlist_create (void);
void net_driverlist_destroy (NET_DRIVERLIST list);
int net_driverlist_clear (NET_DRIVERLIST list);
int net_driverlist_add (NET_DRIVERLIST list, int driver);
int net_driverlist_remove (NET_DRIVERLIST list, int driver);
int net_driverlist_add_list (NET_DRIVERLIST list1, NET_DRIVERLIST list2);
int net_driverlist_remove_list (NET_DRIVERLIST list1, NET_DRIVERLIST list2);
int net_driverlist_test (NET_DRIVERLIST list, int driver);
int net_driverlist_foreach (NET_DRIVERLIST list, int (*func)(int driver, void *dat), void *dat);
int net_driverlist_count (NET_DRIVERLIST list);


/* Address functions */
NET_PREPADDR_DATA *net_prepareaddress (int type, const char *addr, char *dest);
int net_poll_prepareaddress (NET_PREPADDR_DATA *data);


/* Channel functions */
NET_CHANNEL *net_openchannel (int type, const char *addr);
int net_closechannel (NET_CHANNEL *channel);
int net_fixupaddress_channel (NET_CHANNEL *chan, const char *addr, char *dest);

int net_assigntarget (NET_CHANNEL *channel, const char *target);
char *net_getlocaladdress (NET_CHANNEL *channel);

int net_send (NET_CHANNEL *channel, const void *buffer, int size);
int net_receive (NET_CHANNEL *channel, void *buffer, int maxsize, char *from);
int net_query (NET_CHANNEL *channel);


/* Conn functions */
NET_CONN *net_openconn (int type, const char *addr);
int net_closeconn (NET_CONN *conn);
int net_fixupaddress_conn (NET_CONN *conn, const char *addr, char *dest);

int net_listen (NET_CONN *conn);
NET_CONN *net_poll_listen (NET_CONN *conn);
int net_connect (NET_CONN *conn, const char *addr);
int net_poll_connect (NET_CONN *conn);

int net_connect_wait_time (NET_CONN *conn, const char *addr, int time);
int net_connect_wait_cb (NET_CONN *conn, const char *addr, int (*cb)(void));
int net_connect_wait_cb_time (NET_CONN *conn, const char *addr, int (*cb)(void), int time);

int net_send_rdm (NET_CONN *conn, const void *buffer, int size);
int net_receive_rdm (NET_CONN *conn, void *buffer, int maxsize);
int net_query_rdm (NET_CONN *conn);
int net_ignore_rdm (NET_CONN *conn);
int net_conn_stats (NET_CONN *conn, int *in_q, int *out_q);
char *net_getpeer (NET_CONN *conn);


/* Driver functions */
int net_channel_driver (NET_CHANNEL *channel);
int net_conn_driver (NET_CONN *conn);
int net_driver_class (int driver);


/* Callback functions */
void net_set_mutex_funcs (
	void (*create)  (void **),
	void (*destroy) (void *),
	void (*lock)    (volatile void *),
	void (*unlock)  (volatile void *)
);
void net_set_timer_func (
	unsigned int (*timer) (void)
);


/* Waiting functions */
void *net_wait_all (int tv);


#ifdef __cplusplus
}
#endif


#endif

