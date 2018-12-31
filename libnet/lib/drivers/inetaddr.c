/* $Header: /cvsroot/libnet/libnet/lib/drivers/inetaddr.c,v 1.8 2001/09/30 03:22:02 tjaden Exp $
 *  Address utility functions
 */

#include "platdefs.h"

/* conditional for whole file */
#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_WSOCK_WIN__ || defined __USE_REAL_WSOCK_DOS__ || defined __USE_REAL_BESOCKS__

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "libnet.h"
#include "internal.h"
#include "inetdefs.h"
#include "inetaddr.h"

#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_BESOCKS__
#include <netdb.h>
#endif

#if defined __USE_REAL_WSOCK_DOS__
#include <netinet/in.h>
#endif


char *write_address (char *p, struct sockaddr_in *addr)
{
	unsigned char *q;
	q = (unsigned char *) &(addr->sin_addr.s_addr);
	*p = '\0';
	if (addr->sin_addr.s_addr != INADDR_ANY)
		sprintf (p,"%d.%d.%d.%d",q[0],q[1],q[2],q[3]);
	sprintf (strchr (p, '\0'), ":%d", ntohs(addr->sin_port));
	return p;
}

int get_address (const char *str, int *address)
{
	const char *s;
	unsigned char *a = (unsigned char *)address;
	int isnumerical = 1;
	int i = 3;
  
	*address = 0;
  
	if (!strcmp (str, "*")) {
		*address = INADDR_BROADCAST;
		return 0;
	}
  
	s = str;
	while ((*s) && (isnumerical)) {
		if (isdigit ((unsigned char)*s))
			a[i] = a[i] * 10 + (*s - '0');
		else if (*s == '.') {
			i--;
			if (i == -1) isnumerical = 0;
		} else {
			isnumerical = 0;
		}
		s++;
	}

	if (isnumerical && i) return -1;  /* Not enough parts */
	if (isnumerical) return 0;        /* OK, nice numerical address */

	/* Not numerical -- need a DNS lookup */
	#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_WSOCK_WIN__ || defined __USE_REAL_BESOCKS__
	{
		int i;
		struct hostent *he = gethostbyname (str);
		if (!he) return -1;

		for (i = 0; i < 4; i++)
			a[i] = he->h_addr[3-i];

		return 0;
	}
	#elif defined __USE_REAL_WSOCK_DOS__
	{
		/* Ugly as hell!  Maybe the DNS resolver should move into 
		 * this file, for DOS sockets. */
		extern int __libnet_internal_wsockdos__dns_lookup (const char *, int, void *);
		extern int __libnet_internal_wsockdos__dns_timeout;
		return __libnet_internal_wsockdos__dns_lookup (str, __libnet_internal_wsockdos__dns_timeout, a);
	}
	#else
		#error configuration problem
	#endif
}

int parse_address (const char *str, int *addr, int *port)
{
	unsigned char a[4] = {0};
	int i = 0;
	char s[NET_MAX_ADDRESS_LENGTH], *t;
	strncpy (s, str, NET_MAX_ADDRESS_LENGTH);
	s[NET_MAX_ADDRESS_LENGTH-1] = '\0';
	t = strchr (s, ':');
	*port = t ? *t = 0, atoi (t+1) : 0;
	if (*s) return get_address (s, addr);
	*addr = INADDR_ANY;
	return 0;
}


/* conditional for prepareaddress routines */
#if defined __USE_REAL_SOCKS__ || defined __USE_REAL_WSOCK_WIN__ || defined __USE_REAL_BESOCKS__

#if defined __USE_REAL_SOCKS__

#include <pthread.h>

struct prep_driver_data {
	pthread_t thread;
	int done;
};

#define THREADDECLARE void *
#define THREADRETURN return NULL

#elif defined __USE_REAL_WSOCK_WIN__

#include <process.h>

struct prep_driver_data {
   HANDLE thread;
   int done;
};

#define THREADDECLARE void
#define THREADRETURN return

#elif defined __USE_REAL_BESOCKS__

#include "scheduler.h"

struct prep_driver_data {
 thread_id thread;
 int done;
};

#define THREADDECLARE int32
#define THREADRETURN return 0

#else

#error configuration problem

#endif


static THREADDECLARE ns_resolver (void *data)
{
	struct NET_PREPADDR_DATA *d = data;
	struct prep_driver_data *dd = d->driver_data;

	char *sp, *dp = d->dest;

	for (sp = d->src; *sp && !isdigit(*sp) && !isalpha(*sp); sp++)
		*dp++ = *sp;
	
	if (!*sp) {
		dd->done = -1;
		THREADRETURN;
	}

	if (isdigit(*sp)) {
		strncpy (d->dest, d->src, NET_MAX_ADDRESS_LENGTH);
		dd->done = 1;
		THREADRETURN;
	}

	{
		struct hostent *he;
		char name[NET_MAX_ADDRESS_LENGTH];
		char *end;
		strncpy (name, sp, NET_MAX_ADDRESS_LENGTH);
		end = strchr (name, ':');
		if (end) *end = '\0';
		he = gethostbyname (name);
		if (!he) {
			dd->done = -1;
			THREADRETURN;
		}
	
		sprintf (dp, "%d.%d.%d.%d",
			(unsigned char)he->h_addr[0],
			(unsigned char)he->h_addr[1],
			(unsigned char)he->h_addr[2],
			(unsigned char)he->h_addr[3]);
	}

	while (*sp && *sp != ':') sp++;
	dp = strchr (dp, '\0');
	while (*sp && dp-d->dest < NET_MAX_ADDRESS_LENGTH-1) *dp++ = *sp++;
	*dp = '\0';

	dd->done = 1;
	THREADRETURN;
}

int inet_prepareaddress (NET_PREPADDR_DATA *d)
{
	int result;
	struct prep_driver_data *dd = malloc (sizeof (struct prep_driver_data));
	if (!dd) return -1;
	d->driver_data = dd;
	dd->done = 0;

	#if defined __USE_REAL_SOCKS__

	result = pthread_create (&dd->thread, NULL, ns_resolver, d);
	if (!result) pthread_detach (dd->thread);
	return result;

	#elif defined __USE_REAL_WSOCK_WIN__

	dd->thread = (HANDLE)_beginthread (ns_resolver, 0, d);
	if (dd->thread) {
		CloseHandle (dd->thread);
		return 0;
	} else {
		return -1;
	}

	#elif defined __USE_REAL_BESOCKS__

	dd->thread = spawn_thread(ns_resolver, "ns_resolver", B_NORMAL_PRIORITY, d);
	if (dd->thread >= B_OK) {
		resume_thread(dd->thread);
		return 0;
	} else {
		return -1;
	}

	#else

	#error configuration problem

	#endif
}

int inet_poll_prepareaddress (NET_PREPADDR_DATA *d)
{
	int done = ((struct prep_driver_data *)d->driver_data)->done;
	if (done) free (d->driver_data);
	return done;
}

#endif /* conditional for prepareaddress routines */


#endif /* conditional for whole file */

