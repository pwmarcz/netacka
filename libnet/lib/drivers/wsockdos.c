/*----------------------------------------------------------------
 * wsock.c - Winsock in DOS driver for libnet
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1998
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include "platdefs.h"

/* If we can use the Winsock from DOS, do so.  Otherwise, dummy driver...
 */
#ifdef __USE_REAL_WSOCK_DOS__

#include <assert.h>
#include <ctype.h>
#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/farptr.h>
#include <sys/segments.h>
#include <time.h>
#include <netinet/in.h>

#include "wsockdos.h"

extern int __libnet_internal_wsockdos__dns_lookup (const char *, int, void *);

/*------------------------------------------------ VxD interface code */

#define VxdLdr_DeviceID 0x0027
#define VxdLdr_LoadDevice 1
#define VxdLdr_UnLoadDevice 2

static int VxdLdrEntry [2] = {0, 0};  /* VxD loader/unloader 16 bit PM entry */

static void VxdGetEntry (int *Entry, int ID)
{
	int dummy;
	asm ("\
		pushl   %%es            \n\
		movw    %%di, %%es      \n\
		int     $0x2f           \n\
		movl    $0, %%ecx       \n\
		movw    %%es, %%cx      \n\
		popl    %%es            "
	:
		"=a" (dummy),
		"=c" (Entry [1]),
		"=D" (Entry [0])
	:
		"a" (0x1684),
		"b" (ID),
		"D" (0)
	:
		"%edx"
	);
}

static void VxdLdrInit (void)
{
	VxdGetEntry (VxdLdrEntry, VxdLdr_DeviceID);
}

static int VxdLdrLoadDevice (char Device [])
{
	int Error;
	__dpmi_meminfo _dev;
	int dev;

	if (VxdLdrEntry [1] == 0) VxdLdrInit ();
	if (VxdLdrEntry [1] == 0) return -1;

	_dev.handle = 0;
	_dev.size = strlen (Device) + 1;
	_dev.address = 0;

	__dpmi_allocate_memory (&_dev);
	dev = __dpmi_allocate_ldt_descriptors (1);
	__dpmi_set_segment_base_address (dev, _dev.address);
	__dpmi_set_segment_limit (dev, _dev.size);

	_farpokex (dev, 0, Device, _dev.size);

	asm ("\
		pushl   %%ds               \n\
		movw    %%cx, %%ds         \n\
		lcall  *%%cs:_VxdLdrEntry  \n\
		setc    %%al               \n\
		movzbl  %%al, %%eax        \n\
		popl    %%ds"
	:
		"=a" (Error)
	:
		"a" (VxdLdr_LoadDevice),
		"c" (dev),
		"d" (0)
	);

	__dpmi_free_ldt_descriptor (dev);
	__dpmi_free_memory (_dev.handle);

	return Error;
}

static int VxdLdrUnLoadDevice (char Device [])
{
	int Error;
	__dpmi_meminfo _dev;
	int dev;

	if (VxdLdrEntry [1] == 0) VxdLdrInit ();
	if (VxdLdrEntry [1] == 0) return -1;

	_dev.handle = 0;
	_dev.size = strlen (Device) + 1;
	_dev.address = 0;

	__dpmi_allocate_memory (&_dev);
	dev = __dpmi_allocate_ldt_descriptors (1);
	__dpmi_set_segment_base_address (dev, _dev.address);
	__dpmi_set_segment_limit (dev, _dev.size);

	_farpokex (dev, 0, Device, _dev.size);

	asm ("\
		pushl   %%ds               \n\
		movw    %%cx, %%ds         \n\
		lcall  *%%cs:_VxdLdrEntry  \n\
		setc    %%al               \n\
		movzbl  %%al, %%eax        \n\
		popl    %%ds"
	:
		"=a" (Error)
	:
		"a" (VxdLdr_UnLoadDevice),
		"c" (dev),
		"d" (0)
	);

	__dpmi_free_ldt_descriptor (dev);
	__dpmi_free_memory (_dev.handle);

	return Error;
}


/*------------------------------------------------ Interface to WSOCK.VXD */

static __dpmi_meminfo _SocketP;         /* 64k selector for parameters */
static int SocketP;
static __dpmi_meminfo _SocketD;         /* 64k selector for data */
static int SocketD;
static int last_error = 0;              /* last error code from VxD function */
static int WSockEntry  [2] = {0, 0};    /* wsock.vxd 16 bit PM entry */


static int call_vxd(int func)
{
	int error;

	asm ("\
		pushl   %%es                    \n\
		pushl   %%ecx                   \n\
		popl    %%es                    \n\
		lcall  *_WSockEntry             \n\
		andl    $0x0000ffff, %%eax      \n\
		popl    %%es"
	:
		"=a" (error)
	:
		"a" (func),
		"b" (0),
		"c" (SocketP)
	);

	return error;
}

static int handle_error(int error)
{
	if (error && (error != 65535)) {
		last_error = error;
		return SOCKET_ERROR;
	} else
		return 0;
}


/* Semi-standard socket functions (some winsock-style rather than Berkeley) */

static SOCKET accept (SOCKET s, struct sockaddr *addr, int *addrlen)
{
	int error;
	int AddressLength,AddressPointer;
	SOCKET t;

	if (addrlen)
		AddressLength = *addrlen;
	else
		AddressLength = 0;

	if (addr)
		AddressPointer = (SocketP << 16) + (7 * 4);
	else
		AddressPointer = 0;

	_farpokel (SocketP, 0 * 4, AddressPointer);             /* Address */
	_farpokel (SocketP, 1 * 4, s);                          /* Listening Socket */
	_farpokel (SocketP, 2 * 4, 0);                          /* Connected Socket */
	_farpokel (SocketP, 3 * 4, AddressLength);              /* Address Length */
	_farpokel (SocketP, 4 * 4, 0);                          /* Connected Socket Handle */
	_farpokel (SocketP, 5 * 4, 0);                          /* Apc Routing */
	_farpokel (SocketP, 6 * 4, 0);                          /* Apc Context */

	error = call_vxd(WSOCK_ACCEPT_CMD);

	if (addr&&addrlen)
		_farpeekx (SocketP, 7 * 4, addr, *addrlen);            // should also update *addrlen

	t = _farpeekl (SocketP, 2 * 4);

	if (!error)
		return t;
	else {
		handle_error(error);
		return INVALID_SOCKET;
	}
}

static int bind (SOCKET s, const struct sockaddr *name, int namelen)
{
	int error;

	_farpokel (SocketP, 0 * 4, (SocketP << 16) + (5 * 4));      /* Address */
	_farpokel (SocketP, 1 * 4, s);                              /* Socket */
	_farpokel (SocketP, 2 * 4, namelen);                        /* Address Length */
	_farpokel (SocketP, 3 * 4, 0);                              /* Apc Routine */
	_farpokel (SocketP, 4 * 4, 0);                              /* Apc Context */

	_farpokex (SocketP, 5 * 4, name, namelen);

	error = call_vxd(WSOCK_BIND_CMD);

	return handle_error(error);
}

static int closesocket (SOCKET s)
{
	int error;

	_farpokel (SocketP, 0 * 4, s);
	error = call_vxd(WSOCK_CLOSESOCKET_CMD);

	return handle_error(error);
}

static int connect (SOCKET s, const struct sockaddr *name, int namelen)
{
	int error;

	_farpokel (SocketP, 0 * 4, (SocketP << 16) + (5 * 4));      // Address
	_farpokel (SocketP, 1 * 4, s);                              // Socket
	_farpokel (SocketP, 2 * 4, namelen);                        // Address Length
	_farpokel (SocketP, 3 * 4, 0);                              // Apc Routine
	_farpokel (SocketP, 4 * 4, 0);                              // Apc Context

	_farpokex (SocketP, 5 * 4, name, namelen);

	error = call_vxd(WSOCK_CONNECT_CMD);

	return handle_error(error);
}

static int getpeername (SOCKET s, struct sockaddr *name, int *namelen)
{
	int error;

	_farpokel (SocketP, 0 * 4, (SocketP << 16) + (3 * 4));      /* Address */
	_farpokel (SocketP, 1 * 4, s);                              /* Socket */
	_farpokel (SocketP, 2 * 4, *namelen);                       /* Address Length */

	error = call_vxd(WSOCK_GETPEERNAME_CMD);

	_farpeekx (SocketP, 3 * 4, name, *namelen);

	return handle_error(error);
}

static int getsockname (SOCKET s, struct sockaddr *name, int *namelen)
{
	int error;

	_farpokel (SocketP, 0 * 4, (SocketP << 16) + (3 * 4));      /* Address */
	_farpokel (SocketP, 1 * 4, s);                              /* Socket */
	_farpokel (SocketP, 2 * 4, *namelen);                       /* Address Length */

	error = call_vxd(WSOCK_GETSOCKNAME_CMD);

	_farpeekx (SocketP, 3 * 4, name, *namelen);

	return handle_error(error);
}

static int getsockopt (SOCKET s, int level, int optname, char *optval, int *optlen)
{
	int error;

	_farpokel (SocketP, 0 * 4, (SocketP << 16) + (6 * 4));      /* Value */
	_farpokel (SocketP, 1 * 4, s);                              /* Socket */
	_farpokel (SocketP, 2 * 4, level);                          /* Option Level */
	_farpokel (SocketP, 3 * 4, optname);                        /* Option Name */
	_farpokel (SocketP, 4 * 4, *optlen);                        /* Value Length */
	_farpokel (SocketP, 5 * 4, 0);                              /* Int Value */

	error = call_vxd (WSOCK_GETSOCKOPT_CMD);

	if (*optlen == 4)
		*((int *) optval) = _farpeekl (SocketP, 5 * 4);
	else
		_farpeekx (SocketP, 6 * 4, optval, *optlen);

	return handle_error(error);
}

static unsigned long inet_addr (const char *cp) {
	unsigned int a[4] = { 0 };
	int i = 0, error = 0;
	const char *s = cp;

	do {
		if (!isdigit (*s))
			error = 1;
		else {
			do {
				a[i] = a[i] * 10 + (*s - '0');
				s++;
			} while (isdigit (*s));
			if (*s == '.') {
				if ((i == 3) || (a[i] > 255)) error = 1;
				s++;
				i++;
			}
		}
	} while ((*s) && (!error));

	if (error) return INADDR_NONE;

	switch (i) {
		case 0:
			return a[0];
			break;
		case 1:
			if (a[1] >= (1<<24)) return INADDR_NONE;
			return (a[0] << 24) + a[1];
			break;
		case 2:
			if (a[2] >= (1<<16)) return INADDR_NONE;
			return (a[0] << 24) + (a[1] << 16) + a[2];
			break;
		case 3:
			if (a[3] >= (1<<8)) return INADDR_NONE;
			return (a[0] << 24) + (a[1] << 16) + (a[2] << 8) + a[3];
			break;
	}
	return INADDR_NONE;
}

static char inet_ntoa_buffer[32] = { 0 };

static void write_number (unsigned int num, char **where) {
	if (num >= 10) write_number (num/10, where);
	*((*where)++) = '0' + (num % 10);
}

static char *inet_ntoa (struct in_addr in) {
	char *chp = inet_ntoa_buffer;
	unsigned char *v = (unsigned char *)&in.s_addr;
	write_number (v[3], &chp);
	*chp++ = '.';
	write_number (v[2], &chp);
	*chp++ = '.';
	write_number (v[1], &chp);
	*chp++ = '.';
	write_number (v[0], &chp);
	*chp++ = 0;
	return inet_ntoa_buffer;
}

static int ioctlsocket (SOCKET s, long cmd, u_long *argp)
{
	int error;
	int size;

	_farpokel (SocketP, 0 * 4, s);                        /* Socket */
	_farpokel (SocketP, 1 * 4, cmd);                      /* Command */
	_farpokel (SocketP, 2 * 4, *argp);                    /* Parameter */

	error = call_vxd (WSOCK_IOCTLSOCKET_CMD);

	switch (cmd) {
		case FIONBIO:
		case FIONREAD:
			size=4;
			break;
		case SIOCATMARK:
			size=1;
			break;
		default:
			size=0;
	}

	_farpeekx (SocketP, 2 * 4, argp, size);

	return handle_error(error);
}

static int listen(SOCKET s, int backlog)
{
	_farpokel (SocketP, 0 * 4, s);                      /* Socket */
	_farpokel (SocketP, 1 * 4, backlog);                /* Backlog */

	return handle_error(call_vxd(WSOCK_LISTEN_CMD));
}

static int recvfrom (SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
	int error;
	int AddressPointer, AddressLength;

	if (from) {
		if (fromlen)
			AddressLength=*fromlen;
		else
			AddressLength=0;
		AddressPointer = (SocketP << 16) + (10 * 4);
	} else {
		AddressPointer = 0;
		AddressLength  = 0;
	}

	_farpokel (SocketP, 0 * 4, SocketD << 16);              /* Buffer */
	_farpokel (SocketP, 1 * 4, AddressPointer);             /* Address */
	_farpokel (SocketP, 2 * 4, s);                          /* Socket */
	_farpokel (SocketP, 3 * 4, len);                        /* Buffer Length */
	_farpokel (SocketP, 4 * 4, flags);                      /* Flags */
	_farpokel (SocketP, 5 * 4, AddressLength);              /* Address Length */
	_farpokel (SocketP, 6 * 4, 0);                          /* Bytes Received */
	_farpokel (SocketP, 7 * 4, 0);                          /* Apc Routine */
	_farpokel (SocketP, 8 * 4, 0);                          /* Apc Context */
	_farpokel (SocketP, 9 * 4, 0);                          /* Timeout */

	error = call_vxd (WSOCK_RECV_CMD);

	if (from) _farpeekx (SocketP, 10 * 4, from, AddressLength);

	_farpeekx (SocketD, 0, buf, len);

	if (error==0)
		return _farpeekl (SocketP, 6 * 4);
	else
		return handle_error(error);
}

static int recv (SOCKET s, char *buf, int len, int flags)
{
	return recvfrom(s,buf,len,flags,0,0);
}

static int sendto (SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
	int error;
	int AddressPointer, AddressLength;

	if (to) {
		AddressPointer = (SocketP << 16) + (10 * 4);
		AddressLength  = tolen;
	} else {
		AddressPointer = 0;
		AddressLength  = 0;
	}

	_farpokel (SocketP, 0 * 4, SocketD << 16);                  /* Buffer */
	_farpokel (SocketP, 1 * 4, AddressPointer);                 /* Address */
	_farpokel (SocketP, 2 * 4, s);                              /* Socket */
	_farpokel (SocketP, 3 * 4, len);                            /* Buffer Length */
	_farpokel (SocketP, 4 * 4, flags);                          /* Flags */
	_farpokel (SocketP, 5 * 4, AddressLength);                  /* Address Length */
	_farpokel (SocketP, 6 * 4, 0);                              /* Bytes Sent */
	_farpokel (SocketP, 7 * 4, 0);                              /* Apc Routine */
	_farpokel (SocketP, 8 * 4, 0);                              /* Apc Context */
	_farpokel (SocketP, 9 * 4, 0);                              /* Timeout */

	if (to) _farpokex (SocketP, 10 * 4, to, tolen);

	_farpokex (SocketD, 0, buf, len);

	error = call_vxd (WSOCK_SEND_CMD);

	if (error==0)
		return _farpeekl (SocketP, 6 * 4);
	else
		return handle_error(error);
}

static int send (SOCKET s, const char *buf, int len, int flags)
{
	return sendto(s,buf,len,flags,0,0);
}

static int setsockopt (SOCKET s, int level, int optname, const char *optval, int optlen)
{
	int error;

	_farpokel (SocketP, 0 * 4, (SocketP << 16) + (6 * 4));      /* Value */
	_farpokel (SocketP, 1 * 4, s);                              /* Socket */
	_farpokel (SocketP, 2 * 4, level);                          /* Option Level */
	_farpokel (SocketP, 3 * 4, optname);                        /* Option Name */
	_farpokel (SocketP, 4 * 4, optlen);                         /* Value Length */
	_farpokel (SocketP, 5 * 4, *((int *) optval));              /* Int Value */

	_farpokex (SocketP, 6 * 4, optval, optlen);

	error = call_vxd (WSOCK_SETSOCKOPT_CMD);

	return handle_error(error);
}

static int shutdown (SOCKET s, int how)
{
	_farpokel (SocketP, 0 * 4, s);                              /* Socket */
	_farpokel (SocketP, 1 * 4, how);                            /* How */

	return handle_error(call_vxd(WSOCK_SHUTDOWN_CMD));
}

static SOCKET socket (int af, int type, int protocol)
{
	SOCKET s;
	int error;

	_farpokel (SocketP, 0 * 4, af);                             /* Address Family */
	_farpokel (SocketP, 1 * 4, type);                           /* Socket Type */
	_farpokel (SocketP, 2 * 4, protocol);                       /* Protocol */
	_farpokel (SocketP, 3 * 4, 0);                              /* New Socket */
	_farpokel (SocketP, 4 * 4, 0);                              /* New Socket Handle */

	error = call_vxd (WSOCK_SOCKET_CMD);

	s = _farpeekl (SocketP, 3 * 4);

	if (error==0)
		return s;
	else {
		handle_error(error);
		return INVALID_SOCKET;
	}
}


/* Initialisation of the VxD and interface */

static int i_loaded_wsock_vxd = 0;
static int i_loaded_wsock_386 = 0;
static int vxd_loaded = 0;

static int load_wsock_vxd (void)
{
	if (vxd_loaded) return 0;

	if (VxdLdrLoadDevice ("WSOCK.VXD") == 0) i_loaded_wsock_vxd = 1;
	if (VxdLdrLoadDevice ("WSOCK.386") == 0) i_loaded_wsock_386 = 1;
	VxdGetEntry (WSockEntry, 0x003e);

	vxd_loaded = (WSockEntry[1] != 0);      /* CS == 0 => wsock.vxd not present */

	return vxd_loaded ? 0 : 1;
}

static int unload_wsock_vxd (void) {
	int retval = 0;
	if (i_loaded_wsock_vxd) {
		i_loaded_wsock_vxd = 0;
		if (VxdLdrUnLoadDevice ("WSOCK.VXD")) retval = 1;
	}
	if (i_loaded_wsock_386) {
		i_loaded_wsock_386 = 0;
		if (VxdLdrUnLoadDevice ("WSOCK.386")) retval = 1;
	}
	vxd_loaded = 0;
	return retval;
}

static int winsock_detect (void) {
	int retval;
	retval = load_wsock_vxd();
	unload_wsock_vxd();
	return retval;
}

#define ERRORLEVEL(xxx) do { retval = xxx; goto error_level_##xxx; } while (0)

static int winsock_init (void) {
	int retval = -1;

	if (load_wsock_vxd()) ERRORLEVEL(1);

	_SocketP.handle = 0;
	_SocketP.size = 65536;
	_SocketP.address = 0;
	if (__dpmi_allocate_memory (&_SocketP) == -1) ERRORLEVEL(2);

	_SocketD.handle = 0;
	_SocketD.size = 65536;
	_SocketD.address = 0;
	if (__dpmi_allocate_memory (&_SocketD) == -1) ERRORLEVEL(3);

	if ((SocketP = __dpmi_allocate_ldt_descriptors (1)) == -1) ERRORLEVEL(4);
	if ((SocketD = __dpmi_allocate_ldt_descriptors (1)) == -1) ERRORLEVEL(5);

	if ((__dpmi_set_segment_base_address (SocketP, _SocketP.address) == -1)
	 || (__dpmi_set_segment_base_address (SocketD, _SocketD.address) == -1)
	 || (__dpmi_set_segment_limit (SocketP, _SocketP.size) == -1)
	 || (__dpmi_set_segment_limit (SocketD, _SocketD.size) == -1))
		ERRORLEVEL(6);

	return 0;

	error_level_6:
		__dpmi_free_ldt_descriptor (SocketD);
	error_level_5:
		__dpmi_free_ldt_descriptor (SocketP);
	error_level_4:
		__dpmi_free_memory (_SocketD.handle);
	error_level_3:
		__dpmi_free_memory (_SocketP.handle);
	error_level_2:
		unload_wsock_vxd();
	error_level_1:
		return retval;
}

static int winsock_exit (void) {
	int retval = 0;
	retval |= __dpmi_free_ldt_descriptor (SocketD);
	retval |= __dpmi_free_ldt_descriptor (SocketP);
	retval |= __dpmi_free_memory (_SocketD.handle);
	retval |= __dpmi_free_memory (_SocketP.handle);
	retval |= unload_wsock_vxd();
	return retval;
}

/*------------------------------------------------ DNS lookup code */

#include "dns.h"

#define MAX_NAMESERVERS 4
static int num_nameservers = 0;
static int nameserver[MAX_NAMESERVERS] = { 0 };


static unsigned char data_block[512];
static int data_size = 0;



/* Data putting routines -- these put data into the send/receive buffer.
 * It's all big-endian. */

static void put_1 (int *offset, int x) {
 if (offset && (*offset >= 0) && (*offset < 512))
  data_block [(*offset)++] = x;
 else
  if (offset) *offset = -1;

 if (*offset > data_size) data_size = *offset;
}

static void put_2 (int *offset, int x) {
 put_1 (offset, (x & 0xff00) >> 8);
 put_1 (offset, (x & 0x00ff));
}

static void put_4 (int *offset, int x) {
 put_2 (offset, (x & 0xffff0000) >> 16);
 put_2 (offset, (x & 0x0000ffff));
}

static void put_hostname (int *offset, char *hostname) {
 char *p, *q;

 if ((!offset) || (*offset < 0)) return;

 if (*hostname == 0) {
  if (*offset >= 512) {
   *offset = -1;
   return;
  }
  data_block[(*offset)++] = 0;
  return;
 }

 if (*offset + 1 + strlen (hostname) >= 512) {
  *offset = -1;
  return;
 }

 strcpy (data_block + *offset + 1, hostname);

 p = data_block + *offset;
 q = p + 1;
 do {
  while (*q && (*q != '.')) q++;
  *p = q - (p + 1);
  p = q++;
 } while (*p);

 *offset = ((unsigned char *)q) - data_block;
 if (*offset > data_size) data_size = *offset;
}


/* Data getting routines -- these get data from the buffer used for sends
 * and receives.  Big-endian again. */

static int do_get_1 (int *offset) {
 return (offset && (*offset >= 0) && (*offset < data_size)) ? data_block[(*offset)++] : ((offset ? *offset = -1 : 0), 0);
}

static int get_1 (int *offset) {
 int x = do_get_1 (offset);
 return x;
}

static int get_2 (int *offset) {
 int ret = get_1(offset);
 return (ret << 8) + get_1(offset);
}

static int get_4 (int *offset) {
 int ret = get_2(offset);
 return (ret<<16) + get_2(offset);
}

static char *get_char_string (int *offset, char *buffer) {
	int ch,i;
	ch = get_1 (offset);
	for (i = 0; i < ch; i++) buffer[i] = get_1 (offset);
	buffer[i] = 0;
	return i ? buffer : NULL;
}

static char *get_hostname (int *offset, char *buffer) {
	static int depth = 0;
	int ch;

	#define MAX_DEPTH 20
	#define RETURN(xxx) do { char *__fubar = (xxx); depth--; return (__fubar); } while (0)

	if (++depth > MAX_DEPTH) RETURN (NULL);

	ch = get_1 (offset);    /* read the label length byte */

	if (ch == 0) {      /* If it's zero, this is the end of the hostname */
		*buffer = 0;
		RETURN (buffer);
	}

	switch (ch >> 6) {
		case 0:     /* It's a label */
			{
				int i;
				for (i = 0; i < ch; i++) buffer[i] = get_1 (offset);
				buffer[i] = '.';
				get_hostname (offset, buffer+i+1); 
				if (!buffer[i+1]) buffer[i] = 0;
			}
			break;
		case 3:     /* It's a pointer to somewhere else in the data */
			{
				int i = ((ch & 0x3f) << 8) + get_1 (offset);
				get_hostname (&i, buffer);
			}
			break;
		default:        /* Undefined; this should never occur */
			*buffer = 0;
			RETURN (NULL);
	}

	RETURN (buffer);

	#undef MAX_DEPTH
	#undef RETURN
}



/* clear_datablock:
 *  Clears the packet buffer, zeroing its size.
 */
static void clear_datablock(void) {
 memset (data_block, 0, 512);
 data_size = 0;
}




/* These are utility routines to translate the dns_query and dns_packet
   structs into the RFC1035 format. */

static void put_query (int *offset, struct dns_query *q) {
	put_hostname (offset, q->qname);
	put_2        (offset, q->qtype);
	put_2        (offset, q->qclass);
}

static void get_query (int *offset, struct dns_query *q) {
	char buffer[512];
	char *qname;

	qname = get_hostname (offset, buffer);
	q->qname = qname ? strdup (qname) : NULL;
	q->qtype  = get_2 (offset);
	q->qclass = get_2 (offset);
}

static void get_rr (int *offset, struct dns_rr *rr) {
	char buffer[512];
	char *name;
	int rdata,i;

	name = get_hostname (offset, buffer);
	rr->name = name ? strdup (name) : NULL;
	rr->type = get_2 (offset);
	rr->_class = get_2 (offset);
	rr->ttl = get_4 (offset);
	rr->rdlength = get_2 (offset);
	rr->rdata = (char *) malloc (rr->rdlength);

	rdata = offset ? *offset : -1;
	for (i = 0; i < rr->rdlength; i++) rr->rdata[i] = get_1 (offset);

	switch (rr->type) {
		case DNS_TYPE_A:
			rr->data.a.address = get_4 (&rdata);
			break;
		case DNS_TYPE_CNAME:
			rr->data.cname.cname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_HINFO:
			rr->data.hinfo.cpu = get_char_string (&rdata, buffer) ? strdup (buffer) : NULL;
			rr->data.hinfo.os  = get_char_string (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MB:
			rr->data.mb.madname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MD:
			rr->data.md.madname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MF:
			rr->data.mf.madname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MG:
			rr->data.mg.mgmname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MINFO:
			rr->data.minfo.rmailbx = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			rr->data.minfo.emailbx = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MR:
			rr->data.mr.newname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_MX:
			rr->data.mx.preference = get_2 (&rdata);
			rr->data.mx.exchange = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_NULL:
			rr->data.null.data = (char *) malloc (rr->rdlength);
			memcpy (rr->data.null.data, rr->rdata, rr->rdlength);
			break;
		case DNS_TYPE_NS:
			rr->data.ns.nsdname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_PTR:
			rr->data.ptr.ptrdname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			break;
		case DNS_TYPE_SOA:
			rr->data.soa.mname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			rr->data.soa.rname = get_hostname (&rdata, buffer) ? strdup (buffer) : NULL;
			rr->data.soa.serial = get_4 (&rdata);
			rr->data.soa.refresh = get_4 (&rdata);
			rr->data.soa.retry = get_4 (&rdata);
			rr->data.soa.expire = get_4 (&rdata);
			rr->data.soa.minimum = get_4 (&rdata);
			break;
		case DNS_TYPE_TXT:
			{
				int n = 1, m, start = rdata;
				while (get_char_string (&rdata, buffer)) n++;
				rr->data.txt.txt_data = (char **) malloc (sizeof (char *) * n);
				for (m = 0; m < n; m++) rr->data.txt.txt_data[m] = get_char_string (&start, buffer) ? strdup (buffer) : NULL;
			}
			break;
		case DNS_TYPE_WKS:
			rr->data.wks.address = get_4 (&rdata);
			rr->data.wks.protocol = get_1 (&rdata);
			rr->data.wks.bitmapsize = (*offset) - rdata;
			rr->data.wks.bitmap = (char *) malloc (rr->data.wks.bitmapsize);
			for (i = 0; i < rr->data.wks.bitmapsize; i++) rr->data.wks.bitmap[i] = get_1 (&rdata);
			break;
	}
}

static void put_packet (struct dns_packet *q) {
	int offset = 0, i;

	clear_datablock();

	put_2 (&offset, q->id);
	put_2 (&offset, q->flags.i);
	put_2 (&offset, q->qdcount);
	put_2 (&offset, 0);             /* q->ancount */
	put_2 (&offset, 0);             /* q->nscount */
	put_2 (&offset, 0);             /* q->arcount */

	for (i = 0; i < q->qdcount; i++) put_query (&offset, q->questions   + i);
}

static void get_packet (struct dns_packet *a) {
	int offset = 0, i;

	a->id      = get_2 (&offset);
	a->flags.i = get_2 (&offset);
	a->qdcount = get_2 (&offset);
	a->ancount = get_2 (&offset);
	a->nscount = get_2 (&offset);
	a->arcount = get_2 (&offset);

	a->questions   = a->qdcount ? (struct dns_query *) malloc (sizeof (struct dns_query) * a->qdcount) : NULL;
	a->answers     = a->ancount ? (struct dns_rr *)    malloc (sizeof (struct dns_rr)    * a->ancount) : NULL;
	a->authorities = a->nscount ? (struct dns_rr *)    malloc (sizeof (struct dns_rr)    * a->nscount) : NULL;
	a->additionals = a->arcount ? (struct dns_rr *)    malloc (sizeof (struct dns_rr)    * a->arcount) : NULL;

	for (i = 0; i < a->qdcount; i++) get_query (&offset, a->questions   + i);
	for (i = 0; i < a->ancount; i++) get_rr    (&offset, a->answers     + i);
	for (i = 0; i < a->nscount; i++) get_rr    (&offset, a->authorities + i);
	for (i = 0; i < a->arcount; i++) get_rr    (&offset, a->additionals + i);
}

static void free_query (struct dns_query *q) {
	free (q->qname);
}

static void free_rr (struct dns_rr *rr) {
	free (rr->name);
	free (rr->rdata);
	switch (rr->type) {
		case DNS_TYPE_CNAME:
			free (rr->data.cname.cname);
			break;
		case DNS_TYPE_HINFO:
			free (rr->data.hinfo.cpu);
			free (rr->data.hinfo.os);
			break;
		case DNS_TYPE_MB:
			free (rr->data.mb.madname);
			break;
		case DNS_TYPE_MD:
			free (rr->data.md.madname);
			break;
		case DNS_TYPE_MF:
			free (rr->data.mf.madname);
			break;
		case DNS_TYPE_MG:
			free (rr->data.mg.mgmname);
			break;
		case DNS_TYPE_MINFO:
			free (rr->data.minfo.rmailbx);
			free (rr->data.minfo.emailbx);
			break;
		case DNS_TYPE_MR:
			free (rr->data.mr.newname);
			break;
		case DNS_TYPE_MX:
			free (rr->data.mx.exchange);
			break;
		case DNS_TYPE_NULL:
			free (rr->data.null.data);
			break;
		case DNS_TYPE_NS:
			free (rr->data.ns.nsdname);
			break;
		case DNS_TYPE_PTR:
			free (rr->data.ptr.ptrdname);
			break;
		case DNS_TYPE_SOA:
			free (rr->data.soa.mname);
			free (rr->data.soa.rname);
			break;
		case DNS_TYPE_TXT:
			{
				char **chpp = rr->data.txt.txt_data;
				while (*chpp) {
					free (*chpp);
					chpp++;
				}
				free (rr->data.txt.txt_data);
			}
			break;
		case DNS_TYPE_WKS:
			free (rr->data.wks.bitmap);
			break;
	}
}

static void free_packet (struct dns_packet *p) {
	int i;

	for (i = 0; i < p->qdcount; i++) free_query (p->questions   + i);
	for (i = 0; i < p->ancount; i++) free_rr    (p->answers     + i);
	for (i = 0; i < p->nscount; i++) free_rr    (p->authorities + i);
	for (i = 0; i < p->arcount; i++) free_rr    (p->additionals + i);

	free (p->questions);
	free (p->answers);
	free (p->authorities);
	free (p->additionals);
}



/* Now for the main resolver */


static struct wait_list_t {
	struct wait_list_t *next, *prev;

	int nameserver;             /* who was asked */
	char *hostname;             /* what was looked up */
	int id;                     /* the packet's ID */
	struct dns_packet *reply;   /* the reply, when it arrives */
} *waiting_list = NULL;

static struct done_list_t {
	struct done_list_t *next;

	int nameserver;             /* who was asked */
	char *hostname;             /* what was looked up */
} *done_list = NULL;

static SOCKET dns_socket = INVALID_SOCKET;
static int dns_pkt_id = 0;

static int create_lists (void) {
	waiting_list = (struct wait_list_t *) malloc (sizeof (struct wait_list_t));
	if (!waiting_list) return 1;
	waiting_list->next = waiting_list->prev = NULL;
	waiting_list->nameserver = 0xdeadc0de;
	waiting_list->hostname = NULL;
	waiting_list->id = -1;
	waiting_list->reply = NULL;

	done_list = (struct done_list_t *) malloc (sizeof (struct done_list_t));
	if (!done_list) { free (waiting_list); return 1; }
	done_list->next = NULL;
	done_list->nameserver = 0xdeadc0de;
	done_list->hostname = NULL;

	return 0;
}

static void destroy_lists (void) {
	struct wait_list_t *wptr;
	struct done_list_t *dptr;

	while (waiting_list) {
		wptr = waiting_list->next;
		free (waiting_list);
		waiting_list = wptr;
	}

	dptr = done_list;
	done_list = done_list->next;
	free (dptr);

	while (done_list) {
		dptr = done_list->next;
		free (done_list);
		done_list = dptr;
	}
}

static int create_socket(void) {
	int one = 1;

	dns_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (dns_socket == INVALID_SOCKET) return 1;
	if (ioctlsocket (dns_socket, FIONBIO, (u_long *)&one)) {
		closesocket (dns_socket);
		return 1;
	}

	return 0;
}

static void destroy_socket(void) {
	closesocket (dns_socket);
}

static void send_request (int nameserver, char *hostname) {
	int packetid = ++dns_pkt_id;

	{
		struct done_list_t *ptr1 = done_list;
		struct wait_list_t *ptr2 = waiting_list;

		while (ptr1->next) {
			ptr1 = ptr1->next;
			if ((ptr1->nameserver == nameserver) && !stricmp (ptr1->hostname, hostname))
				return;     /* already done this query */
		}

		while (ptr2->next) ptr2 = ptr2->next;

		ptr1->next = (struct done_list_t *) malloc (sizeof (struct done_list_t));
		if (!ptr1->next) return;

		ptr2->next = (struct wait_list_t *) malloc (sizeof (struct wait_list_t));
		if (!ptr2->next) {
			free (ptr1->next);
			return;
		}

		ptr1->next->next = NULL; 
		ptr1 = ptr1->next;

		ptr2->next->prev = ptr2;
		ptr2->next->next = NULL;
		ptr2 = ptr2->next;

		ptr2->nameserver = ptr1->nameserver = nameserver;
		ptr2->hostname   = ptr1->hostname   = strdup (hostname);
		ptr2->id                            = packetid;
		ptr2->reply                         = NULL;
	}

	{
		struct dns_packet packet;
		struct dns_query query;

		packet.nameserver  = nameserver;
		packet.id          = packetid;
		packet.flags.i     = 0;
		packet.qdcount     = 1;
		packet.ancount     = 0;
		packet.nscount     = 0;
		packet.arcount     = 0;
		packet.questions   = &query;
		packet.answers     = NULL;
		packet.authorities = NULL;
		packet.additionals = NULL;

		query.qname  = hostname;
		query.qtype  = DNS_TYPE_A;
		query.qclass = DNS_CLASS_IN;

		put_packet (&packet);
	}

	{
		struct sockaddr_in addr;

		addr.sin_family      = AF_INET;
		addr.sin_port        = htons (IPPORT_DNS);
		addr.sin_addr.s_addr = htonl (nameserver);

		sendto (dns_socket, data_block, data_size, 0, (struct sockaddr *)&addr, sizeof (addr));
	}
}

static struct wait_list_t *get_reply(void) {
	struct dns_packet *data;
	struct wait_list_t *x;

	data_size = recvfrom (dns_socket, data_block, 512, 0, NULL, NULL);
	if ((data_size == 0) || (data_size == SOCKET_ERROR)) return 0;

	/* Got some data; now decode it */
	data = (struct dns_packet *) malloc (sizeof (struct dns_packet));
	if (!data) return 0;
	get_packet (data);

	/* Find the right entry in the waiting list */
	for (x = waiting_list->next; x && (x->id != data->id); x = x->next);

	if (!x) {                   /* not in list */
		free_packet (data);
		free (data);
		return 0;
	}

	if (x->next) x->next->prev = x->prev;
	if (x->prev) x->prev->next = x->next;
	x->reply = data;

	return x;
}


/* dns_lookup:
 *  Performs a DNS lookup.  If successful, the IP address is stored in
 *  `address' in host format; otherwise INADDR_NONE (255.255.255.255) is
 *  put there.
 *
 *  Parameters:
 *      `what' is a string containing a hostname
 *      `timeout' is a timeout value in seconds
 *      `address' is a pointer to somewhere to store the resulting IP
 *
 *  Return value:
 *      0   :   hostname resolved successfully
 *      1   :   internal failure (e.g. memory problem)
 *      2   :   host not found (non-authoritative)
 *      3   :   authoritative `host not found'
 */
int __libnet_internal_wsockdos__dns_lookup (const char *what, int timeout, void *address) {
	int *addr = address;            /* `int *' is easier to deal with */
	int retval,i,j;
	char buffer[1024];              /* this always holds the name we're currently looking up (may not be `what' due to CNAMEs) */
	int time_limit = clock() + timeout * CLOCKS_PER_SEC;

	dns_pkt_id = 0;

	if (strlen (what) >= 1024) return 1;
	if (!addr) return 1;
	if (create_socket()) return 1;
	if (create_lists()) { destroy_socket(); return 1; }

	*addr = INADDR_NONE;
	strcpy (buffer, what);

	for (i = 0; i < num_nameservers; i++)
		send_request (nameserver[i], buffer);

	retval = 2;

	while ((clock() < time_limit) && (waiting_list->next) && (retval == 2)) {
		struct wait_list_t *x = get_reply();

		if (x) {
			if (x->reply->flags.f.rcode == 3) { /* auth. host-not-found */
				*addr = INADDR_NONE;
				retval = 3;
			} else {
				/* check for address answer */
				for (i = 0; i < x->reply->ancount; i++)
					if ((x->reply->answers[i].type == DNS_TYPE_A)
					 && (x->reply->answers[i]._class == DNS_CLASS_IN)) {
						*addr = x->reply->answers[i].data.a.address;
						retval = 0;
						goto near_end_of_loop;
					}

				/* check for CNAME answer */
				for (i = 0; i < x->reply->ancount; i++)
					if ((x->reply->answers[i].type == DNS_TYPE_CNAME)
					 && (x->reply->answers[i]._class == DNS_CLASS_IN)) {
						strcpy (buffer, x->reply->answers[i].data.cname.cname);
					}

				/* ask the nameserver again in case it was a cname */
				send_request (x->nameserver,buffer);

				/* ask all the authoritative nameservers */
				for (i = 0; i < x->reply->nscount; i++)
					if (x->reply->authorities[i].type == DNS_TYPE_NS) {
						int y = 1;
						for (j = 0; (j < x->reply->arcount) && y; j++)
							if (x->reply->additionals[j].type == DNS_TYPE_A)
								if (!stricmp (x->reply->additionals[j].name, x->reply->authorities[i].data.ns.nsdname)) {
									y = 0;
									send_request (x->reply->additionals[j].data.a.address, buffer);
								}
					}
			}

			near_end_of_loop:

			free_packet (x->reply);
			free (x->reply);
			free (x);
		}
	}

	destroy_lists();
	destroy_socket();

	return retval;
}


/*------------------------------------------------ libnet interface */

#include "libnet.h"
#include "internal.h"
#include "config.h"

static char desc[80] = "Internet via Winsock from DOS";


static int default_port = 24785;    /* default port */
static int forceport = 0;           /* should we reuse non-vacant ports? */
static int ip_address = 0;          /* IP address */
int __libnet_internal_wsockdos__dns_timeout = 10;  /* seconds */
static int auto_get_ip = 0;         /* should we try to guess our IP? */


struct channel_data_t {
	SOCKET sock;
	struct sockaddr_in target_address;
	int address_length;
};


static int disable_driver = 0;

static int detect(void)
{
	if (disable_driver) return NET_DETECT_NO;
	return winsock_detect() ? NET_DETECT_NO : NET_DETECT_YES;
}

static int get_my_ip (int server, int port, int timelimit) 
{
	SOCKET sock;
	struct sockaddr_in addr;
	u_long one = 1;
	int addrlength;
	int clock_limit = clock() + timelimit * CLOCKS_PER_SEC;
	int clockdiff;

	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) return 0;

	if (ioctlsocket (sock, FIONBIO, &one)) {
		closesocket (sock);
		return 0;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	addr.sin_addr.s_addr = INADDR_ANY;
	addrlength = sizeof (addr);
	if (bind (sock, (struct sockaddr *)&addr, addrlength)) {
		closesocket (sock);
		return 0;
	}

	addr.sin_addr.s_addr = htonl (server);
	addr.sin_port = htons (port);
	addr.sin_family = AF_INET;
	if (connect (sock, (struct sockaddr *)&addr, addrlength) && (last_error != WSAEWOULDBLOCK)) {
		closesocket (sock);
		return 0;
	}

	clockdiff = 1;

	do {
		{
			int clockstop = clock() + (clockdiff *= 2);
			while (clock() < clockstop);
		} 
		addrlength = sizeof (addr);
		getsockname (sock, (struct sockaddr *) &addr, &addrlength);
	} while ((addr.sin_addr.s_addr == 0) && (clock() < clock_limit));

	closesocket (sock);

	return ntohl(addr.sin_addr.s_addr);
}

static int init(void) 
{
	if (winsock_init()) return -1;
	if (auto_get_ip) {
		int i = 0;
		while ((i < num_nameservers) && !ip_address)
			ip_address = get_my_ip (nameserver[i++], 53, 5);
	}
	return 0;
}

static int drv_exit(void)
{
	return winsock_exit();
}

#include "inetaddr.h"

static int do_init_channel (NET_CHANNEL *chan, int addr, int port, int forceport) 
{
	struct channel_data_t *data;
	int addrlength;
	struct sockaddr_in sock_addr;
	u_long a;

	data = (struct channel_data_t *) malloc (sizeof (struct channel_data_t));
	if (!data) return 1;
	chan->data = data;

	data->sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (data->sock==INVALID_SOCKET) { free(data); return 2; }
	a = 1;
	if (ioctlsocket (data->sock, FIONBIO, &a)) {
		closesocket(data->sock);
		free(data);
		return 3;
	}

	/* if forceport is set, tell socket not to fail its bind operation if port 
	 * is already in use */
	setsockopt (data->sock, SOL_SOCKET, SO_REUSEADDR, (char *)&forceport, sizeof (forceport));

	sock_addr.sin_family      = AF_INET;
	sock_addr.sin_port        = htons(port);
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

static int init_channel (NET_CHANNEL *chan, const char *addr) {
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

	if (parse_address (chan->target_addr, &addr, &port)) return 1;
	if (port == 0) port = default_port;
	data->target_address.sin_family = AF_INET;
	data->target_address.sin_port = htons(port);
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
	int x;
	if (!size) return 1;    /* filter out empty packets */
	x = sendto (data->sock, (char *) buf, size, 0, (struct sockaddr *) &data->target_address, data->address_length);
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
	} else
		return -1;
}

static int query (NET_CHANNEL *chan) 
{
	struct channel_data_t *data = chan->data;
	u_long x = 0;

	ioctlsocket (data->sock, FIONREAD, &x);
	return !!x;
}


static void load_config (NET_DRIVER *drv, FILE *fp) {
  char *option, *value;
  
  if (!__libnet_internal__seek_section (fp, "internet")) {
    while (__libnet_internal__get_setting (fp, &option, &value) == 0) {
      
      if (!stricmp (option, "port")) {
	
	int x = atoi (value);
	if (x) default_port = x;
	
      } else if (!stricmp (option, "forceport")) {
	
	forceport = atoi (value);
	
      } else if (!stricmp (option, "ip")) {
	
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
  
  if (!__libnet_internal__seek_section (fp, "winsock_dos")) {
    while (__libnet_internal__get_setting (fp, &option, &value) == 0) {
      
      if (!stricmp (option, "nameserver")) {
	
	char *chp = value;
	int addr = 0, i;
	
	if (num_nameservers < MAX_NAMESERVERS) {
	  num_nameservers++;
	  for (i = 0; i < 4; i++) {
	    while ((*chp) && (*chp != '.')) chp++;
	    if (*chp) *chp++ = 0;
	    addr = (addr << 8) + atoi (value);
	    value = chp;
	  }
	  nameserver[num_nameservers-1] = addr;
	} 
	
      } else if (!stricmp (option, "dns_timeout")) {
	
	int x = atoi (value);
	if (x) __libnet_internal_wsockdos__dns_timeout = x;
	
      } else if (!stricmp (option, "auto_get_ip")) {
	
	auto_get_ip = atoi (value);
	
      } else if (!strcmp (option, "disable")) {
        disable_driver = (atoi (value) || (value[0] == 'y'));
      }

    }
  }
  
  if (drv->load_conn_config) drv->load_conn_config (drv, fp, "internet");
}


NET_DRIVER net_driver_wsock_dos = {
	"Winsock from DOS",
	desc,
	NET_CLASS_INET,

	detect,
	init,
	drv_exit,

	NULL, NULL,
	
	init_channel,
	destroy_channel,
	
	update_target,
	drv_send,
	drv_recv,
	query,
	
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	
	load_config,
	NULL, NULL
};

#else /* __USE_REAL_WSOCK_DOS__ */

/* Can't use the Winsock DOS-style on this platform, so put in a dummy
 * driver. */

#include <stdlib.h>
#include <stdio.h>

#include "libnet.h"
#include "internal.h"

static int detect (void) { return NET_DETECT_NO; }
static void load_config (NET_DRIVER *drv, FILE *fp) { }

NET_DRIVER net_driver_wsock_dos = {
	"Winsock from DOS", "Dummy driver", NET_CLASS_INET,
	detect,
	NULL, NULL,
	NULL, NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	load_config,
	NULL, NULL
};

#endif
