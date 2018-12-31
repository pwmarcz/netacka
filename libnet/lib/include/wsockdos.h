/*----------------------------------------------------------------
 * wsock.h - declarations for the Winsock driver
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Individual sections of this file are copyrighted by their
 *  authors.
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

/*
 *  This information came from various sources, notably:
 *
 *    - `winsock.h' header file I found on www.stardust.com (general
 *      constants and data types)
 *      URL: http://www.stardust.com/
 *
 *    - `wsock.h' header file I found somewhere (vxd indices)
 *
 *    - Dan Hedlund's WSOCK library (farptrx functions)
 *      URL: http://www.rangenet.com/markiv/wsock.html
 *
 *  Please send comments/queries/complaints about any of this to me:
 *
 *    George Foot <george.foot@merton.oxford.ac.uk>
 */


#ifndef libnet_included_file_wsock_h
#define libnet_included_file_wsock_h


/*----------------------------------------------------------------*
 * sort out a few things so the winsock.h stuff can be read
 *----------------------------------------------------------------*/

#define FAR


/*----------------------------------------------------------------*
 * winsock.h (abridged)
 *----------------------------------------------------------------*/

/* WINSOCK.H--definitions to be used with the WINSOCK.DLL
 *
 * This header file corresponds to version 1.1 of the Windows Sockets specification.
 *
 * This file includes parts which are Copyright (c) 1982-1986 Regents
 * of the University of California.  All rights reserved.  The
 * Berkeley Software License Agreement specifies the terms and
 * conditions for redistribution.
 */

/*
 * Basic system type definitions, taken from the BSD file sys/types.h.
 */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

/*
 * The new type to be used in all
 * instances which refer to sockets.
 */
typedef u_int           SOCKET;

/*
 * Commands for ioctlsocket(),  taken from the BSD file fcntl.h.
 *
 *
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_VOID        0x20000000      /* no parameters */
#define IOC_OUT         0x40000000      /* copy out parameters */
#define IOC_IN          0x80000000      /* copy in parameters */
#define IOC_INOUT       (IOC_IN|IOC_OUT)
					/* 0x20000000 distinguishes new &

					   old ioctl's */
#define _IO(x,y)        (IOC_VOID|(x<<8)|y)

#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|(x<<8)|y)

#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|(x<<8)|y)

#define FIONREAD    _IOR('f', 127, u_long) /* get # bytes to read */
#define FIONBIO     _IOW('f', 126, u_long) /* set/clear non-blocking i/o */
#define FIOASYNC    _IOW('f', 125, u_long) /* set/clear async i/o */

/* Socket I/O Controls */
#define SIOCSHIWAT  _IOW('s',  0, u_long)  /* set high watermark */

#define SIOCGHIWAT  _IOR('s',  1, u_long)  /* get high watermark */
#define SIOCSLOWAT  _IOW('s',  2, u_long)  /* set low watermark */
#define SIOCGLOWAT  _IOR('s',  3, u_long)  /* get low watermark */
#define SIOCATMARK  _IOR('s',  7, u_long)  /* at oob mark? */

/*
 * Structures returned by network data base library, taken from the
 * BSD file netdb.h.  All addresses are supplied in host order, and
 * returned in network order (suitable for use in system calls).
 */

struct  hostent {
	char    FAR * h_name;           /* official name of host */
	char    FAR * FAR * h_aliases;  /* alias list */
	short   h_addrtype;             /* host address type */
	short   h_length;               /* length of address */
	char    FAR * FAR * h_addr_list; /* list of addresses */

#define h_addr  h_addr_list[0]          /* address, for backward compat */
};

/*
 * It is assumed here that a network number
 * fits in 32 bits.
 */
struct  netent {
	char    FAR * n_name;           /* official name of net */
	char    FAR * FAR * n_aliases;  /* alias list */
	short   n_addrtype;             /* net address type */
	u_long  n_net;                  /* network # */
};

struct  servent {
	char    FAR * s_name;           /* official service name */
	char    FAR * FAR * s_aliases;  /* alias list */
	short   s_port;                 /* port # */
	char    FAR * s_proto;          /* protocol to use */

};

struct  protoent {
	char    FAR * p_name;           /* official protocol name */
	char    FAR * FAR * p_aliases;  /* alias list */
	short   p_proto;                /* protocol # */
};

/*
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981, taken from the BSD file netinet/in.h.
 */

/*
 * Protocols
 */
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_ICMP            1               /* control message protocol */
#define IPPROTO_GGP             2               /* gateway^2 (deprecated) */
#define IPPROTO_TCP             6               /* tcp */

#define IPPROTO_PUP             12              /* pup */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_IDP             22              /* xns idp */
#define IPPROTO_ND              77              /* UNOFFICIAL net disk proto */

#define IPPROTO_RAW             255             /* raw IP packet */
#define IPPROTO_MAX             256

/*
 * Internet address (old style... should be updated)
 */
struct in_addr {
	union {
		struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { u_short s_w1,s_w2; } S_un_w;

		u_long S_addr;
	} S_un;
#define s_addr  S_un.S_addr
				/* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2
				/* host on imp */
#define s_net   S_un.S_un_b.s_b1
				/* network */
#define s_imp   S_un.S_un_w.s_w2
				/* imp */
#define s_impno S_un.S_un_b.s_b4
				/* imp # */
#define s_lh    S_un.S_un_b.s_b3
				/* logical host */

};

#define INADDR_ANY              (u_long)0x00000000
#define INADDR_LOOPBACK         0x7f000001
#define INADDR_BROADCAST        (u_long)0xffffffff 
#define INADDR_NONE             0xffffffff

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	short   sin_family;
	u_short sin_port;
	struct  in_addr sin_addr;
	char    sin_zero[8];
};

/*
 * Options for use with [gs]etsockopt at the IP level.
 */
#define IP_OPTIONS      1               /* set/get IP per-packet options */

/*
 * Definitions related to sockets: types, address families, options,
 * taken from the BSD file sys/socket.h.
 */

/*
 * This is used instead of -1, since the
 * SOCKET type is unsigned.
 */
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

/*
 * Types
 */
#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */
#define SOCK_RAW        3               /* raw-protocol interface */
#define SOCK_RDM        4               /* reliably-delivered message */
#define SOCK_SEQPACKET  5               /* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define SO_DEBUG        0x0001          /* turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002          /* socket has had listen() */
#define SO_REUSEADDR    0x0004          /* allow local address reuse */

#define SO_KEEPALIVE    0x0008          /* keep connections alive */
#define SO_DONTROUTE    0x0010          /* just use interface addresses */
#define SO_BROADCAST    0x0020          /* permit sending of broadcast msgs */
#define SO_USELOOPBACK  0x0040          /* bypass hardware when possible */
#define SO_LINGER       0x0080          /* linger on close if data present */
#define SO_OOBINLINE    0x0100          /* leave received OOB data in line */

#define SO_DONTLINGER   (u_int)(~SO_LINGER)

/*
 * Additional options.
 */
#define SO_SNDBUF       0x1001          /* send buffer size */
#define SO_RCVBUF       0x1002          /* receive buffer size */
#define SO_SNDLOWAT     0x1003          /* send low-water mark */

#define SO_RCVLOWAT     0x1004          /* receive low-water mark */
#define SO_SNDTIMEO     0x1005          /* send timeout */
#define SO_RCVTIMEO     0x1006          /* receive timeout */
#define SO_ERROR        0x1007          /* get error status and clear */
#define SO_TYPE         0x1008          /* get socket type */

/*
 * TCP options.
 */
#define TCP_NODELAY     0x0001

/*
 * Address families.
 */
#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */

#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */

#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */

#define AF_MAX          18

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	u_short sa_family;              /* address family */
	char    sa_data[14];            /* up to 14 bytes of direct address */

};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	u_short sp_family;              /* address family */
	u_short sp_protocol;            /* protocol */
};

/*
 * Protocol families, same as address families for now.
 */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_ISO          AF_ISO
#define PF_OSI          AF_OSI
#define PF_ECMA         AF_ECMA
#define PF_DATAKIT      AF_DATAKIT

#define PF_CCITT        AF_CCITT
#define PF_SNA          AF_SNA
#define PF_DECnet       AF_DECnet
#define PF_DLI          AF_DLI
#define PF_LAT          AF_LAT
#define PF_HYLINK       AF_HYLINK
#define PF_APPLETALK    AF_APPLETALK

#define PF_MAX          AF_MAX

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define SOL_SOCKET      0xffff          /* options for socket level */


#define MSG_OOB         0x1             /* process out-of-band data */
#define MSG_PEEK        0x2             /* peek at incoming message */
#define MSG_DONTROUTE   0x4             /* send without using routing tables */

#define MSG_MAXIOVLEN   16

/*
 * All Windows Sockets error constants are biased by WSABASEERR from
 * the "normal"
 */
#define WSABASEERR              10000

/*
 * Windows Sockets definitions of regular Microsoft C error constants
 */
#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)

/*
 * Windows Sockets definitions of regular Berkeley error constants
 */
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)

#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)

#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)

/*
 * Extended Windows Sockets error constant definitions
 */
#define WSASYSNOTREADY          (WSABASEERR+91)
#define WSAVERNOTSUPPORTED      (WSABASEERR+92)

#define WSANOTINITIALISED       (WSABASEERR+93)


/*----------------------------------------------------------------*
 * Definitions of VxD command indices.
 *
 * What follows was taken from towards the end of wsock.h.
 *
 * Most of wsock.h doesn't seem to compile; this is all I needed.
 *----------------------------------------------------------------*/

#define WSOCK_FIRST_CMD             0x00000100

#define WSOCK_ACCEPT_CMD            (WSOCK_FIRST_CMD + 0x0000)
#define WSOCK_BIND_CMD              (WSOCK_FIRST_CMD + 0x0001)
#define WSOCK_CLOSESOCKET_CMD       (WSOCK_FIRST_CMD + 0x0002)
#define WSOCK_CONNECT_CMD           (WSOCK_FIRST_CMD + 0x0003)
#define WSOCK_GETPEERNAME_CMD       (WSOCK_FIRST_CMD + 0x0004)
#define WSOCK_GETSOCKNAME_CMD       (WSOCK_FIRST_CMD + 0x0005)
#define WSOCK_GETSOCKOPT_CMD        (WSOCK_FIRST_CMD + 0x0006)
#define WSOCK_IOCTLSOCKET_CMD       (WSOCK_FIRST_CMD + 0x0007)
#define WSOCK_LISTEN_CMD            (WSOCK_FIRST_CMD + 0x0008)
#define WSOCK_RECV_CMD              (WSOCK_FIRST_CMD + 0x0009)
#define WSOCK_SELECT_SETUP_CMD      (WSOCK_FIRST_CMD + 0x000a)
#define WSOCK_SELECT_CLEANUP_CMD    (WSOCK_FIRST_CMD + 0x000b)
#define WSOCK_ASYNC_SELECT_CMD      (WSOCK_FIRST_CMD + 0x000c)
#define WSOCK_SEND_CMD              (WSOCK_FIRST_CMD + 0x000d)
#define WSOCK_SETSOCKOPT_CMD        (WSOCK_FIRST_CMD + 0x000e)
#define WSOCK_SHUTDOWN_CMD          (WSOCK_FIRST_CMD + 0x000f)
#define WSOCK_SOCKET_CMD            (WSOCK_FIRST_CMD + 0x0010)

#define WSOCK_CREATE_CMD            (WSOCK_FIRST_CMD + 0x0011)
#define WSOCK_CREATE_MULTIPLE_CMD   (WSOCK_FIRST_CMD + 0x0012)
#define WSOCK_DESTROY_CMD           (WSOCK_FIRST_CMD + 0x0013)
#define WSOCK_DESTROY_BY_SOCKET_CMD (WSOCK_FIRST_CMD + 0x0014)
#define WSOCK_DESTROY_BY_THREAD_CMD (WSOCK_FIRST_CMD + 0x0015)
#define WSOCK_SIGNAL_CMD            (WSOCK_FIRST_CMD + 0x0016)
#define WSOCK_SIGNAL_ALL_CMD        (WSOCK_FIRST_CMD + 0x0017)

#define WSOCK_CONTROL_CMD           (WSOCK_FIRST_CMD + 0x0018)

#define WSOCK_REGISTER_POSTMSG_CMD  (WSOCK_FIRST_CMD + 0x0019)

#define WSOCK_ARECV_CMD             (WSOCK_FIRST_CMD + 0x001a)
#define WSOCK_ASEND_CMD             (WSOCK_FIRST_CMD + 0x001b)

#ifdef CHICAGO
#define WSOCK_LAST_CMD              WSOCK_ASEND_CMD
#else
#define WSOCK_LAST_CMD              WSOCK_REGISTER_POSTMSG_CMD
#endif


/*----------------------------------------------------------------*
 * farptrx functions, by Dan Hedlund.
 *----------------------------------------------------------------*/
void _farpokex (unsigned short selector, unsigned long offset, const void *x, int len);
void _farpeekx (unsigned short selector, unsigned long offset, void *x, int len);

extern __inline__ void _farpokex (unsigned short selector, unsigned long offset, const void *x, int len)
{
  int dummy1, dummy2;
  __asm__ __volatile__ ("pushl %%es\n"
			"movw %w2, %%es\n"
			"rep\n"
			"movsb\n"
			"popl %%es"
	  : "=S" (dummy1), "=D" (dummy2)
	  : "rm" (selector), "S" (x), "D" (offset), "c" (len)
  );
}

extern __inline__ void _farpeekx (unsigned short selector, unsigned long offset, void *x, int len)
{
  int dummy1, dummy2;
  __asm__ __volatile__ ("pushl %%ds\n"
			"movw %w2,%%ds\n"
			"rep\n"
			"movsb\n"
			"popl %%ds"
	  : "=S" (dummy1), "=D" (dummy2)
	  : "rm" (selector), "S" (offset), "D" (x), "c" (len)
  );
}


/*----------------------------------------------------------------*
 * end of file
 *----------------------------------------------------------------*/

#endif

