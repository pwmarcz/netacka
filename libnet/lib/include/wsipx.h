/* This wsipx.h fragment is included in Libnet in case your Windows compiler
 * does not have its own wsipx.h.  This is not a complete wsipx.h, just the 
 * bits Libnet needs.
 */

/* WSIPX.H -- Winsock 2 Extensions for IPX networks
 *
 * This file contains IPX/SPX specific information for use by
 * Winsock 2 compabable applications. Include this file below
 * WINSOCK.H to enable IPX features in your application.
 *
 * Rev 0.3, Feb 23, 1995
 */

#ifndef _WSIPX_
#define _WSIPX_

#pragma pack(1)

/* Transparant sockaddr definition for address family AF_IPX */
struct sockaddr_ipx {
	u_short sa_family;
	u_char sa_netnum[4];
	u_char sa_nodenum[6];
	unsigned short sa_socket;
};

#endif /* _WSIPX_ */

