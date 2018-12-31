/*----------------------------------------------------------------
 * internet.c - generic internet sockets driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If this platform supports some sort of sockets with this driver, use it. */
#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_WSOCK_WIN__ || defined __USE_REAL_BESOCKS__


/*---------------------------------------- Common includes */
#include <stdlib.h>
#include <string.h>


/*---------------------------------------- Platform-specific definitions */
#include "inetdefs.h"


/*---------------------------------------- libnet interface */
#include "libnet.h"
#include "internal.h"
#include "config.h"

/* General default settings */
static int default_port = 24785;    /* default port */
static int forceport = 0;           /* should we reuse non-vacant ports? */
static int ip_address = 0;          /* IP address */


/*---------------------------------------- Data structures */

/* Internal data for channels */
struct channel_data_t {
  SOCKET sock;
  struct sockaddr_in target_address;
  int address_length;
};


/*---------------------------------------- General driver functions */

static int disable_driver = 0;
static int detect (void)
{
	if (disable_driver) return NET_DETECT_NO;
	{
#ifdef __USE_REAL_WSOCK_WIN__
		unsigned short wVersionRequested = MAKEWORD (1, 1);
		WSADATA wsaData;
		int err;

		err = WSAStartup (wVersionRequested, &wsaData);
		if (err) return NET_DETECT_NO;

		WSACleanup();
#endif
		return NET_DETECT_YES;
	}
}

static int init (void)
{
#ifdef __USE_REAL_WSOCK_WIN__
	short wVersionRequested = MAKEWORD (1, 1);
	WSADATA wsaData;
	unsigned int err;

	err = WSAStartup (wVersionRequested, &wsaData);
	if (err) return 1;
#endif
	return 0;
}

static int drv_exit (void)
{
#ifdef __USE_REAL_WSOCK_WIN__
	WSACleanup();
#endif
	return 0;
}


/*---------------------------------------- Address utility functions */

#include "inetaddr.h"


/*---------------------------------------- Channel functions */

static int do_init_channel (NET_CHANNEL *chan, int addr, int port, int forceport)
{
  struct channel_data_t *data;
  int addrlength;
  struct sockaddr_in sock_addr;
  int a;
  
  data = (struct channel_data_t *) malloc (sizeof (struct channel_data_t));
  if (!data) return 1;
  chan->data = data;
  
  data->sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (data->sock==INVALID_SOCKET) { free(data); return 2; }
  a=1;
  
#ifdef __USE_REAL_BESOCKS__
  a = -1; // the BeOS documentation says fill the data with "non-zeros", but (u_long)1
          // would be 3 zeros and only one non-zero (?)
  if (setsockopt (data->sock, SOL_SOCKET, SO_NONBLOCK, (u_long *)&a, sizeof(u_long))) {
#else  
  if (ioctlsocket (data->sock, FIONBIO, (u_long *)&a)) {
#endif
  
    closesocket(data->sock);
    free(data);
    return 3;
  }
  
  /* If forceport is set, tell socket not to fail its bind operation if port 
   * is already in use */
  setsockopt (data->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&forceport, sizeof (forceport));
  
  sock_addr.sin_family      = AF_INET;
  sock_addr.sin_port        = htons((unsigned short)port);
  sock_addr.sin_addr.s_addr = htonl(addr);
  
  addrlength = sizeof (sock_addr);
  
  if (bind (data->sock, (struct sockaddr *)&sock_addr, addrlength)) {
    closesocket(data->sock);
    free(data);
    return 4;
  }
  
  getsockname (data->sock, (struct sockaddr *)&sock_addr, &addrlength);
  write_address (chan->local_addr, &sock_addr);
  
  return 0;
}

static int init_channel (NET_CHANNEL *chan, const char *addr)
{
  int a, p, f = forceport;
  if (!addr) return do_init_channel (chan, 0, 0, 0);
  if (*addr == '+') {
    f = 1;
    addr++;
  } else if (*addr == '-') {
    f = 0;
    addr++;
  }
  if (!*addr) return do_init_channel (chan, 0, default_port, f);
  parse_address (addr, &a, &p);
  if (!p) p = default_port;
  return do_init_channel (chan, a, p, f);
}

static int update_target (NET_CHANNEL *chan)
{
  struct channel_data_t *data = chan->data;
  int addr, port;
  
  if (parse_address (chan->target_addr,&addr,&port)) return 1;
#ifndef __USE_REAL_BESOCKS__
  if (addr == INADDR_BROADCAST) {
    char one = 1;
    if (setsockopt (data->sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof one)) return 1;
  }
#endif
  if (port == 0) port = default_port;
  data->target_address.sin_family = AF_INET;
  data->target_address.sin_port = htons((unsigned short)port);
  data->target_address.sin_addr.s_addr = htonl(addr);
  
  data->address_length = sizeof (data->target_address);
  
  return 0;
}

static int destroy_channel (NET_CHANNEL *chan)
{
  closesocket (((struct channel_data_t *)chan->data)->sock);
  free (chan->data);
  return 0;
}

static int drv_send (NET_CHANNEL *chan, const void *buf, int size)
{
  struct channel_data_t *data = chan->data;
  int x = sendto (data->sock, (char *)buf, size, 0, (struct sockaddr *)&data->target_address, data->address_length);
  return (x == SOCKET_ERROR);
}

static int drv_recv (NET_CHANNEL *chan, void *buf, int size, char *from)
{
  struct channel_data_t *data = chan->data;
  struct sockaddr_in from_addr;
  int size_read, addrsize = sizeof (from_addr);
  
  size_read = recvfrom (data->sock, (char *)buf, size, 0, (struct sockaddr *) &from_addr, &addrsize);
  if (size_read >= 0) {
    if (from) write_address (from, &from_addr);
    return size_read;
  } else {
    if (
#ifdef __USE_REAL_WSOCK_WIN__
      (WSAGetLastError() != WSAEWOULDBLOCK)
#else
      (errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EINTR)
#endif
    )
      return -1;
    else
      return 0;
  }
}

static int query (NET_CHANNEL *chan)
{
  int sock = ((struct channel_data_t *)chan->data)->sock;
  fd_set rfds;
  struct timeval tv;
  
  FD_ZERO (&rfds);
  FD_SET (sock, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  
  return !!select (sock+1, &rfds, NULL, NULL, &tv);
}


static void load_config (NET_DRIVER *drv, FILE *fp)
{
  char *option, *value;
  
  if (!__libnet_internal__seek_section (fp, "internet")) {
  
    while (!__libnet_internal__get_setting (fp, &option, &value)) {
      
      if (!strcmp (option, "port")) {
        
        int x = atoi (value);
        if (x) default_port = x;
        
      } else if (!strcmp (option, "forceport")) {
        
        forceport = atoi (value);
        
      } else if (!strcmp (option, "ip")) {
        
        char *chp = value;
        int addr = 0, i;
        
        for (i = 0; i < 4; i++) {
	  while ((*chp) && (*chp != '.')) chp++;
	  if (*chp) *chp++ = 0;
	  addr = (addr << 8) + atoi (value);
	  value = chp;
        }
        
        ip_address = addr;
        
      } else if (!strcmp (option, "disable")) {
        disable_driver = (atoi (value) || (value[0] == 'y'));
      }
  
    }
  }

#ifdef __USE_REAL_SOCKS__  
  if (!__libnet_internal__seek_section (fp, "sockets")) {
    while (!__libnet_internal__get_setting (fp, &option, &value)) {
      if (!strcmp (option, "disable")) {
        disable_driver = (atoi (value) || (value[0] == 'y'));
      }
    }
  }
#endif
#ifdef __USE_REAL_WSOCK_WIN__
  if (!__libnet_internal__seek_section (fp, "winsock_win")) {
    while (!__libnet_internal__get_setting (fp, &option, &value)) {
      if (!strcmp (option, "disable")) {
        disable_driver = (atoi (value) || (value[0] == 'y'));
      }
    }
  }
#endif
#ifdef __USE_REAL_BESOCKS__
  if (!__libnet_internal__seek_section (fp, "besocks")) {
    while (!__libnet_internal__get_setting (fp, &option, &value)) {
      if (!strcmp (option, "disable")) {
        disable_driver = (atoi (value) || (value[0] == 'y'));
      }
    }
  }
#endif

  if (drv->load_conn_config) drv->load_conn_config (drv, fp, "internet");
}


#endif  /* __USE_REAL_SOCKS__ || __USE_REAL_WSOCK_WIN__ */


/* Prepare for dummy drivers */

#include <stdio.h>
#include <stdlib.h>

#include "libnet.h"
#include "internal.h"

static int dummy_detect (void) { return NET_DETECT_NO; }
static void dummy_load_config (NET_DRIVER *drv, FILE *fp) { }

#define MAKE_REAL_INET_DRIVER(drv,name) \
	NET_DRIVER drv = { \
		name, \
		"Internet driver", \
		NET_CLASS_INET, \
		\
		detect, \
		init, \
		drv_exit, \
		\
		inet_prepareaddress, inet_poll_prepareaddress, \
		\
		init_channel, \
		destroy_channel, \
		\
		update_target, \
		drv_send, \
		drv_recv, \
		query, \
		\
		NULL, NULL, \
		NULL, NULL, NULL, NULL, \
		NULL, NULL, NULL, NULL, NULL, \
		\
		load_config, \
		NULL, NULL \
	}

#define MAKE_DUMMY_INET_DRIVER(drv,name) \
	NET_DRIVER drv = { \
		name, "Internet driver", \
		NET_CLASS_INET, \
		dummy_detect, \
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, \
		NULL, NULL, NULL, NULL, NULL, NULL, \
		NULL, NULL, NULL, NULL, NULL, \
		dummy_load_config, \
		NULL, NULL \
	}



#ifdef __USE_REAL_SOCKS__
	MAKE_REAL_INET_DRIVER  (net_driver_sockets, "Berkeley Sockets");
#else
	MAKE_DUMMY_INET_DRIVER (net_driver_sockets, "Berkeley Sockets");
#endif


#ifdef __USE_REAL_WSOCK_WIN__
	MAKE_REAL_INET_DRIVER  (net_driver_wsock_win, "Winsock from Windows");
#else
	MAKE_DUMMY_INET_DRIVER (net_driver_wsock_win, "Winsock from Windows");
#endif


#ifdef __USE_REAL_BESOCKS__
	MAKE_REAL_INET_DRIVER  (net_driver_besocks, "BeOS Sockets");
#else
	MAKE_DUMMY_INET_DRIVER (net_driver_besocks, "BeOS Sockets");
#endif

