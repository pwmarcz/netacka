/* $Header: /cvsroot/libnet/libnet/lib/include/inetaddr.h,v 1.9 2001/08/12 16:55:30 gfoot Exp $
 *  Address utility functions
 */

#ifndef libnet_included_inetaddr_h
#define libnet_included_inetaddr_h

#include "inetdefs.h"

#define write_address            __libnet_internal__write_address
#define get_address              __libnet_internal__get_address
#define parse_address            __libnet_internal__parse_address
#define inet_prepareaddress      __libnet_internal__inet_prepareaddress
#define inet_poll_prepareaddress __libnet_internal__inet_poll_prepareaddress

#include "platdefs.h"

#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_WSOCK_WIN__ || defined __USE_REAL_WSOCK_DOS__ || defined __USE_REAL_BESOCKS__
	char *write_address (char *p, struct sockaddr_in *addr);
	int get_address (const char *str, int *address);
	int parse_address (const char *str, int *addr, int *port);

	#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_WSOCK_WIN__ || defined __USE_REAL_BESOCKS__
		int inet_prepareaddress (NET_PREPADDR_DATA *d);
		int inet_poll_prepareaddress (NET_PREPADDR_DATA *d);
	#endif
#endif

#endif

