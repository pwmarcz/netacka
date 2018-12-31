/* $Header: /cvsroot/libnet/libnet/lib/include/inetdefs.h,v 1.3 2001/08/12 16:55:30 gfoot Exp $
 *
 *  Platform-specific internet definitions
 *
 *  For the most part, this means that Berkeley sockets emulates Winsock.
 */

#ifndef libnet_included_inetdefs_h
#define libnet_included_inetdefs_h

#include "platdefs.h"

/*---------------------------------------- Berkeley sockets includes (Unix) */
#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_BESOCKS__
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET int
#define INVALID_SOCKET ((SOCKET)-1)
#define SOCKET_ERROR (-1)
#define ioctlsocket ioctl
#define closesocket close
#endif

/*---------------------------------------- Windows Winsock includes */
#ifdef __USE_REAL_WSOCK_WIN__
#include <winsock.h>
#endif

/*---------------------------------------- DOS Winsock includes */
#ifdef __USE_REAL_WSOCK_DOS__
#include "wsockdos.h"
#endif

#endif

