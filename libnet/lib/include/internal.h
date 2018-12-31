/*----------------------------------------------------------------
 * internal.h - internal things for libnet library
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */


#ifndef libnet_included_file_internal_h
#define libnet_included_file_internal_h

#include "libnet.h"
#include <stdio.h>


struct NET_CHANNEL {
  int type;                                  /* network type */
  struct NET_DRIVER *driver;                 /* network driver */
  char target_addr[NET_MAX_ADDRESS_LENGTH];  /* target address */
  char local_addr[NET_MAX_ADDRESS_LENGTH];   /* local address */
  void *data;                                /* data block for driver's use */
};

struct NET_CONN {
  int type;                                /* network type */
  struct NET_DRIVER *driver;               /* network driver */
  int status;                              /* status (idle, connected, etc) */
  char peer_addr[NET_MAX_ADDRESS_LENGTH];  /* peer address */
  void *data;                              /* data block for driver's use */
};


struct NET_DRIVER {
  char *name;                                                        /* driver name */
  char *desc;                                                        /* description string */
  int class;                                                         /* driver class */
  
  int (*detect)(void);                                               /* auto-detect function (0 = absent) */
  int (*init)(void);                                                 /* initialise (0 = okay) */
  int (*exit)(void);                                                 /* undo the above initialisation */


  int (*prepareaddress)(NET_PREPADDR_DATA *data);                    /* prepare address for use (e.g. DNS lookup) */
  int (*poll_prepareaddress)(NET_PREPADDR_DATA *data);               /* poll status of preparation */
  
  
  int (*init_channel) (NET_CHANNEL *chan, const char *addr);               /* perform low-level initialisation on a channel */
  int (*destroy_channel) (NET_CHANNEL *chan);                        /* undo the above initialisation */
  
  int (*update_target) (NET_CHANNEL *chan);                          /* update private data for change of target address */
  int (*send) (NET_CHANNEL *chan, const void *buf, int size);              /* send data */
  int (*recv) (NET_CHANNEL *chan, void *buf, int max, char *from);   /* receive data */
  int (*query) (NET_CHANNEL *chan);                                  /* query for incoming data */
  
  
  int (*init_conn) (NET_CONN *conn, const char *addr);                      /* performs low-level initialisation of a connection */
  int (*destroy_conn) (NET_CONN *conn);                               /* undo any initialisation */
  
  int (*listen) (NET_CONN *conn);                                     /* start listening for connections */
  int (*poll_listen) (NET_CONN *conn, NET_CONN *newconn);             /* poll for connections (when listening) */
  int (*connect) (NET_CONN *conn, const char *addr);                        /* start attempting to connect */
  int (*poll_connect) (NET_CONN *conn);                               /* check whether connected (when attempting) */
  
  int (*send_rdm) (NET_CONN *conn, const void *buf, int size);              /* send reliably delivered message */
  int (*recv_rdm) (NET_CONN *conn, void *buf, int max);               /* receive RDM */
  int (*query_rdm) (NET_CONN *conn);                                  /* query for incoming RDMs */
  int (*ignore_rdm) (NET_CONN *conn);                                 /* ignores the next RDM */
  int (*conn_stats) (NET_CONN *conn, int *in_q, int *out_q);          /* gets queue lenghts */
  
  void (*load_config) (NET_DRIVER *drv, FILE *fp);                    /* load configuration data */
  void (*load_conn_config) (NET_DRIVER *drv, FILE *fp, const char *section); /* hook for wrapper conn routines */
  struct conn_config *conn_config;                                      /* data for wrapper conn routines */
};

typedef NET_DRIVERLIST list_t;    /* I'm lazy */


/* Used internally by address conversion routines */
struct NET_PREPADDR_DATA {
	NET_DRIVER *driver;
	char src[NET_MAX_ADDRESS_LENGTH];
	char *dest;
	void *driver_data;
};


#endif

