/*----------------------------------------------------------------
 * threads.c - module to help the library be thread-safe
 *----------------------------------------------------------------
 *  libnet is (c) Copyright Chad Catlett and George Foot 1997-1999
 *
 *  Please look in `docs' for details, documentation and
 *  distribution conditions.
 */

#include <stdlib.h>
#include <libnet.h>
#include "threads.h"


void (*__libnet_internal__mutex_create) (void **mutex);
void (*__libnet_internal__mutex_destroy) (void *mutex);
void (*__libnet_internal__mutex_lock) (volatile void *mutex);
void (*__libnet_internal__mutex_unlock) (volatile void *mutex);


static void default_mutex_create (void **mutex)
{
	*mutex = malloc (sizeof (int));
	*(int *)*mutex = 0;
}

static void default_mutex_destroy (void *mutex)
{
	free (mutex);
}

static void default_mutex_lock (volatile void *param)
{
	volatile int *mutex = param;
	do {
		while (*mutex);
		(*mutex)++;
		if (*mutex == 1) break;
		(*mutex)--;
	} while (1);
}

static void default_mutex_unlock (volatile void *mutex)
{
	(*(int *)mutex)--;
}


/* net_set_mutex_funcs:
 *  Installs user-provided mutex create/destroy/lock/unlock 
 *  functions.  They may use the integer pointer parameter for 
 *  coordination such that a `lock' call only terminates if no 
 *  other thread is between calling `lock' and `unlock'.  
 *  `create' will be called once beforehand to create the mutex,
 *  and `destroy' will be called later on to destroy it.
 */
void net_set_mutex_funcs (
	void (*create)  (void **),
	void (*destroy) (void *),
	void (*lock)    (volatile void *),
	void (*unlock)  (volatile void *)
)
{
	if (create) {
		__libnet_internal__mutex_create = create;
		__libnet_internal__mutex_destroy = destroy;
		__libnet_internal__mutex_lock = lock;
		__libnet_internal__mutex_unlock = unlock;
	} else {
		__libnet_internal__mutex_create = default_mutex_create;
		__libnet_internal__mutex_destroy = default_mutex_destroy;
		__libnet_internal__mutex_lock = default_mutex_lock;
		__libnet_internal__mutex_unlock = default_mutex_unlock;
	}
}


